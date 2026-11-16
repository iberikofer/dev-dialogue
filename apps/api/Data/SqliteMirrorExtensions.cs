using System.Security.Cryptography;
using System.Text;
using Microsoft.Data.Sqlite;
using Microsoft.EntityFrameworkCore;

namespace DevDialogue.Api.Data;

/// <summary>
/// Runtime database provider state allowing instant switching between Default (Postgres/InMemory) and encrypted SQLite.
/// </summary>
public sealed class DatabaseModeState
{
    private volatile bool _useSqlite;
    public string SqliteFilePath { get; }
    public string SqliteConnectionString { get; }

    public DatabaseModeState(bool initialUseSqlite, string sqliteFilePath, string sqliteConnectionString)
    {
        _useSqlite = initialUseSqlite;
        SqliteFilePath = sqliteFilePath;
        SqliteConnectionString = sqliteConnectionString;
    }

    public bool UseSqlite
    {
        get => _useSqlite;
        set => _useSqlite = value;
    }

    public string ActiveMode => _useSqlite ? "Sqlite" : "Default";
}

/// <summary>
/// Encrypted SQLite (SQLCipher) copy &amp; runtime provider-switching helpers built on the existing AppDbContext.
/// </summary>
public static class SqliteMirrorExtensions
{
    public const string DefaultSqliteFileName = "dev_dialogue_copy.db";
    private const string DefaultKeyFileName = "dev_dialogue.key";
    private static int _batteriesInitialized;

    public static void EnsureSqlCipherInitialized()
    {
        if (Interlocked.Exchange(ref _batteriesInitialized, 1) == 0)
        {
            SQLitePCL.Batteries_V2.Init();
        }
    }

    public static string GetSqliteCopyPath(string contentRootPath)
        => Path.Combine(contentRootPath, "Data", DefaultSqliteFileName);

    public static bool ShouldUseSqlite(IConfiguration configuration, string? connectionString)
    {
        var provider = configuration["Database:Provider"] ?? Environment.GetEnvironmentVariable("DATABASE_PROVIDER");
        if (string.Equals(provider, "Sqlite", StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }
        if (string.Equals(provider, "InMemory", StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }
        if (!string.IsNullOrWhiteSpace(connectionString)
            && connectionString.Contains("Data Source=", StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        // Auto-fallback: if no reachable cloud/PostgreSQL database is connected, use encrypted local SQLite.
        return !CanConnectPostgres(connectionString);
    }

    public static bool CanConnectPostgres(string? connectionString)
    {
        if (string.IsNullOrWhiteSpace(connectionString)
            || (!connectionString.Contains("Host=", StringComparison.OrdinalIgnoreCase)
                && !connectionString.Contains("Server=", StringComparison.OrdinalIgnoreCase)))
        {
            return false;
        }

        try
        {
            var builder = new Npgsql.NpgsqlConnectionStringBuilder(connectionString)
            {
                Timeout = 3
            };
            using var conn = new Npgsql.NpgsqlConnection(builder.ConnectionString);
            conn.Open();
            return true;
        }
        catch
        {
            return false;
        }
    }

    public static string BuildEncryptedSqliteConnectionString(
        IConfiguration configuration,
        string contentRootPath,
        string? existingConnectionString = null)
    {
        EnsureSqlCipherInitialized();

        var csb = !string.IsNullOrWhiteSpace(existingConnectionString)
                  && existingConnectionString.Contains("Data Source=", StringComparison.OrdinalIgnoreCase)
            ? new SqliteConnectionStringBuilder(existingConnectionString)
            : new SqliteConnectionStringBuilder { DataSource = GetSqliteCopyPath(contentRootPath) };

        if (string.IsNullOrWhiteSpace(csb.Password))
        {
            csb.Password = GetOrCreateEncryptionKey(configuration, contentRootPath);
        }

        return csb.ToString();
    }

    public static async Task<SqliteSyncResult> SyncSqliteCopyAsync(
        AppDbContext sourceDb,
        string sqliteConnectionString,
        string sqliteFilePath,
        CancellationToken cancellationToken = default)
    {
        EnsureSqlCipherInitialized();
        Directory.CreateDirectory(Path.GetDirectoryName(sqliteFilePath)!);

        var options = new DbContextOptionsBuilder<AppDbContext>()
            .UseSqlite(sqliteConnectionString)
            .Options;

        await using var sqliteDb = new AppDbContext(options);
        await sqliteDb.Database.EnsureCreatedAsync(cancellationToken);

        var isAlreadySqlite = sourceDb.Database.ProviderName?.Contains("Sqlite", StringComparison.OrdinalIgnoreCase) == true;
        if (!isAlreadySqlite)
        {
            await SyncEntitiesAsync(
                await sourceDb.Users.AsNoTracking().ToListAsync(cancellationToken),
                sqliteDb.Users,
                u => u.Id,
                cancellationToken);

            await SyncEntitiesAsync(
                await sourceDb.QuestionsBank.AsNoTracking().ToListAsync(cancellationToken),
                sqliteDb.QuestionsBank,
                q => q.Id,
                cancellationToken);

            await SyncEntitiesAsync(
                await sourceDb.Interviews.AsNoTracking().ToListAsync(cancellationToken),
                sqliteDb.Interviews,
                i => i.Id,
                cancellationToken);

            await sqliteDb.SaveChangesAsync(cancellationToken);
        }

        return new SqliteSyncResult(
            sqliteFilePath,
            await sqliteDb.Users.CountAsync(cancellationToken),
            await sqliteDb.QuestionsBank.CountAsync(cancellationToken),
            await sqliteDb.Interviews.CountAsync(cancellationToken)
        );
    }

    public static IEndpointRouteBuilder MapSqliteCopyEndpoints(this IEndpointRouteBuilder app)
    {
        app.MapGet("/api/health/sqlite-copy", async (
            AppDbContext db,
            DatabaseModeState modeState,
            CancellationToken ct) =>
        {
            var result = await SyncSqliteCopyAsync(db, modeState.SqliteConnectionString, modeState.SqliteFilePath, ct);
            return Results.Ok(new
            {
                status = "Encrypted SQLite copy ready",
                encrypted = true,
                cipher = "SQLCipher (AES-256)",
                file = result.SqlitePath,
                tables = new
                {
                    users = result.UsersCount,
                    questions_bank = result.QuestionsCount,
                    interviews = result.InterviewsCount
                },
                timestamp = DateTime.UtcNow
            });
        })
        .WithTags("Health");

        app.MapGet("/api/health/db-mode", (AppDbContext db, DatabaseModeState modeState) =>
        {
            return Results.Ok(new
            {
                mode = modeState.ActiveMode,
                useSqlite = modeState.UseSqlite,
                encrypted = modeState.UseSqlite,
                cipher = modeState.UseSqlite ? "SQLCipher (AES-256)" : null,
                database = db.Database.ProviderName,
                sqliteFile = modeState.SqliteFilePath,
                timestamp = DateTime.UtcNow
            });
        })
        .WithTags("Health");

        app.MapPost("/api/health/db-mode", async (
            HttpRequest req,
            AppDbContext db,
            DatabaseModeState modeState,
            CancellationToken ct) =>
        {
            string? requested = req.Query["provider"].FirstOrDefault();
            if (string.IsNullOrWhiteSpace(requested) && req.ContentLength > 0)
            {
                var body = await req.ReadFromJsonAsync<DbModeSwitchRequest>(ct);
                requested = body?.Provider;
            }

            bool targetSqlite = string.Equals(requested, "sqlite", StringComparison.OrdinalIgnoreCase)
                || (string.IsNullOrWhiteSpace(requested) && !modeState.UseSqlite);

            if (targetSqlite && !modeState.UseSqlite)
            {
                await SyncSqliteCopyAsync(db, modeState.SqliteConnectionString, modeState.SqliteFilePath, ct);
            }

            modeState.UseSqlite = targetSqlite;

            return Results.Ok(new
            {
                mode = modeState.ActiveMode,
                useSqlite = modeState.UseSqlite,
                encrypted = targetSqlite,
                activeProviderAfterSwitch = targetSqlite ? "Microsoft.EntityFrameworkCore.Sqlite (SQLCipher)" : "Default (Postgres/InMemory)",
                timestamp = DateTime.UtcNow
            });
        })
        .WithTags("Health");

        app.MapGet("/db", () => Results.Content(DbSwitchPageHtml, "text/html; charset=utf-8"))
            .ExcludeFromDescription();

        return app;
    }

    private static string GetOrCreateEncryptionKey(IConfiguration configuration, string contentRootPath)
    {
        var configured = configuration["Database:SqliteKey"] ?? Environment.GetEnvironmentVariable("SQLITE_ENCRYPTION_KEY");
        if (!string.IsNullOrWhiteSpace(configured))
        {
            return configured.Trim();
        }

        var keyPath = Path.Combine(contentRootPath, "Data", DefaultKeyFileName);
        Directory.CreateDirectory(Path.GetDirectoryName(keyPath)!);

        if (File.Exists(keyPath))
        {
            var raw = File.ReadAllBytes(keyPath);
            if (OperatingSystem.IsWindows())
            {
                var unprotected = ProtectedData.Unprotect(raw, null, DataProtectionScope.CurrentUser);
                return Encoding.UTF8.GetString(unprotected);
            }
            return Encoding.UTF8.GetString(raw).Trim();
        }

        var key = Convert.ToBase64String(RandomNumberGenerator.GetBytes(32));
        var keyBytes = Encoding.UTF8.GetBytes(key);
        if (OperatingSystem.IsWindows())
        {
            var protectedBytes = ProtectedData.Protect(keyBytes, null, DataProtectionScope.CurrentUser);
            File.WriteAllBytes(keyPath, protectedBytes);
        }
        else
        {
            File.WriteAllBytes(keyPath, keyBytes);
            File.SetUnixFileMode(keyPath, UnixFileMode.UserRead | UnixFileMode.UserWrite);
        }

        return key;
    }

    private static async Task SyncEntitiesAsync<TEntity, TKey>(
        List<TEntity> sourceItems,
        DbSet<TEntity> targetSet,
        Func<TEntity, TKey> keySelector,
        CancellationToken cancellationToken) where TEntity : class where TKey : notnull
    {
        var existingIds = (await targetSet.AsNoTracking().ToListAsync(cancellationToken))
            .Select(keySelector)
            .ToHashSet();

        foreach (var item in sourceItems)
        {
            if (existingIds.Contains(keySelector(item)))
            {
                targetSet.Update(item);
            }
            else
            {
                await targetSet.AddAsync(item, cancellationToken);
            }
        }
    }

    private const string DbSwitchPageHtml = """
        <!doctype html>
        <html lang="uk">
        <head>
          <meta charset="utf-8">
          <title>DevDialogue — Database Switch</title>
          <style>
            body { font-family: system-ui, sans-serif; background: #0f172a; color: #e2e8f0; display: flex; justify-content: center; padding: 48px 16px; }
            .card { background: #1e293b; border: 1px solid #334155; border-radius: 14px; padding: 24px; max-width: 580px; width: 100%; }
            h2 { margin: 0 0 12px; font-size: 20px; }
            .row { display: flex; align-items: center; justify-content: space-between; gap: 12px; margin: 16px 0; padding: 14px; background: #0f172a; border-radius: 10px; }
            .badge { padding: 4px 10px; border-radius: 999px; font-size: 13px; font-weight: 600; background: #334155; }
            .badge.sqlite { background: #065f46; color: #a7f3d0; }
            .badge.default { background: #1e40af; color: #bfdbfe; }
            button { border: 0; border-radius: 8px; padding: 10px 16px; font-size: 14px; font-weight: 600; cursor: pointer; background: #3b82f6; color: #fff; }
            button:hover { background: #2563eb; }
            pre { background: #020617; padding: 12px; border-radius: 8px; overflow-x: auto; font-size: 12px; color: #94a3b8; }
          </style>
        </head>
        <body>
          <div class="card">
            <h2>Перемикач бази даних (DevDialogue API)</h2>
            <div class="row">
              <div>
                <div>Поточний режим: <span id="mode-badge" class="badge">...</span></div>
                <small id="provider-name" style="color:#94a3b8"></small>
              </div>
              <button id="toggle-btn" onclick="toggleMode()">Перемкнути</button>
            </div>
            <pre id="status-json">Завантаження...</pre>
          </div>
          <script>
            let currentSqlite = false;
            async function loadStatus() {
              const [dbRes, modeRes] = await Promise.all([
                fetch('/api/health/db').then(r => r.json()),
                fetch('/api/health/db-mode').then(r => r.json())
              ]);
              currentSqlite = Boolean(modeRes.useSqlite);
              const badge = document.getElementById('mode-badge');
              badge.textContent = currentSqlite ? 'SQLite (Encrypted)' : 'Default (InMemory / Postgres)';
              badge.className = 'badge ' + (currentSqlite ? 'sqlite' : 'default');
              document.getElementById('provider-name').textContent = (dbRes.database || '') + (modeRes.cipher ? ' • ' + modeRes.cipher : '');
              document.getElementById('toggle-btn').textContent = currentSqlite ? 'Перемкнути на Default' : 'Перемкнути на SQLite';
              document.getElementById('status-json').textContent = JSON.stringify({ healthDb: dbRes, dbMode: modeRes }, null, 2);
            }
            async function toggleMode() {
              const next = currentSqlite ? 'default' : 'sqlite';
              await fetch('/api/health/db-mode?provider=' + next, { method: 'POST' });
              await loadStatus();
            }
            loadStatus();
          </script>
        </body>
        </html>
        """;
}

public record DbModeSwitchRequest(string? Provider);
public record SqliteSyncResult(string SqlitePath, int UsersCount, int QuestionsCount, int InterviewsCount);
