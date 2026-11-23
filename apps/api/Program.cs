using System.Text;
using System.Security.Claims;
using DevDialogue.Api.Data;
using DevDialogue.Api.Models.Entities;
using DevDialogue.Api.Services.AI;
using Microsoft.AspNetCore.Authentication.JwtBearer;
using Microsoft.EntityFrameworkCore;
using Microsoft.IdentityModel.Tokens;
using Microsoft.OpenApi.Models;
using Microsoft.AspNetCore.Mvc;

var builder = WebApplication.CreateBuilder(args);

// Optional local dev configuration override (gitignored)
builder.Configuration.AddJsonFile("appsettings.Local.json", optional: true, reloadOnChange: true);

// Load .env if present (checks apps/api/.env and workspace root .env)
var envCandidates = new[]
{
    Path.Combine(builder.Environment.ContentRootPath, ".env"),
    Path.Combine(builder.Environment.ContentRootPath, "..", "..", ".env")
};
foreach (var envPath in envCandidates)
{
    if (File.Exists(envPath))
    {
        foreach (var line in File.ReadAllLines(envPath))
        {
            var trimmed = line.Trim();
            if (string.IsNullOrWhiteSpace(trimmed) || trimmed.StartsWith("#")) continue;
            var parts = trimmed.Split('=', 2);
            if (parts.Length == 2 && string.IsNullOrEmpty(Environment.GetEnvironmentVariable(parts[0].Trim())))
            {
                Environment.SetEnvironmentVariable(parts[0].Trim(), parts[1].Trim().Trim('"'));
            }
        }
        break;
    }
}

// Dynamic PORT binding for cloud container environments
var renderPort = Environment.GetEnvironmentVariable("PORT");
if (!string.IsNullOrWhiteSpace(renderPort))
{
    builder.WebHost.UseUrls($"http://0.0.0.0:{renderPort}");
}

// Database Configuration (PostgreSQL with in-memory fallback, plus runtime encrypted SQLite switch)
SqliteMirrorExtensions.EnsureSqlCipherInitialized();
var connectionString = builder.Configuration.GetConnectionString("DefaultConnection")
                    ?? Environment.GetEnvironmentVariable("CONNECTION_STRING");

var sqliteFilePath = SqliteMirrorExtensions.GetSqliteCopyPath(builder.Environment.ContentRootPath);
var initialSqliteConn = SqliteMirrorExtensions.BuildEncryptedSqliteConnectionString(
    builder.Configuration,
    builder.Environment.ContentRootPath,
    connectionString);

builder.Services.AddSingleton(new DatabaseModeState(
    SqliteMirrorExtensions.ShouldUseSqlite(builder.Configuration, connectionString),
    sqliteFilePath,
    initialSqliteConn));

builder.Services.AddDbContext<AppDbContext>((sp, options) =>
{
    var modeState = sp.GetRequiredService<DatabaseModeState>();
    if (modeState.UseSqlite)
    {
        options.UseSqlite(modeState.SqliteConnectionString);
        return;
    }

    var isPostgresConfigured = !string.IsNullOrWhiteSpace(connectionString)
                            && (connectionString.Contains("Host=", StringComparison.OrdinalIgnoreCase)
                             || connectionString.Contains("Server=", StringComparison.OrdinalIgnoreCase));

    if (isPostgresConfigured)
    {
        options.UseNpgsql(connectionString);
    }
    else
    {
        options.UseInMemoryDatabase("DevDialogueDevDb");
    }
});

// AI Service Registration (Cloud, Local Ollama, or Mock)
builder.Services.AddSingleton<LocalModelManager>();
builder.Services.AddSingleton<AvatarSpeechService>();
builder.Services.AddSingleton<ParakeetSpeechToTextService>();
builder.Services.AddSingleton<VisualSessionAggregator>();
builder.Services.AddHttpClient<LocalVisionService>();
var aiProvider = builder.Configuration["AI:Provider"] ?? "Mock";
var aiApiKey = builder.Configuration["AI:ApiKey"] ?? Environment.GetEnvironmentVariable("AI_API_KEY");

if (string.Equals(aiProvider, "Cloud", StringComparison.OrdinalIgnoreCase) && !string.IsNullOrWhiteSpace(aiApiKey))
{
    builder.Services.AddHttpClient<IAiService, CloudAiService>();
}
else if (string.Equals(aiProvider, "Ollama", StringComparison.OrdinalIgnoreCase))
{
    builder.Services.AddHttpClient<IAiService, OllamaAiService>();
}
else
{
    builder.Services.AddSingleton<IAiService, MockAiService>();
}

// Supabase JWT Authentication
var supabaseUrl = builder.Configuration["Supabase:Url"] 
                  ?? Environment.GetEnvironmentVariable("SUPABASE_URL");

if (string.IsNullOrWhiteSpace(supabaseUrl))
{
    throw new Exception("Supabase:Url is missing in configuration.");
}

builder.Services.AddAuthentication(options =>
    {
        options.DefaultAuthenticateScheme = JwtBearerDefaults.AuthenticationScheme;
        options.DefaultChallengeScheme = JwtBearerDefaults.AuthenticationScheme;
    })
    .AddJwtBearer(options =>
    {
        options.RequireHttpsMetadata = false;
        options.SaveToken = true;
    
        // Automatically fetch public keys (JWKS) from Supabase
        options.Authority = $"{supabaseUrl}/auth/v1";
    
        options.TokenValidationParameters = new TokenValidationParameters
        {
            ValidateIssuer = true,
            ValidIssuer = $"{supabaseUrl}/auth/v1",
            ValidateAudience = true,
            ValidAudience = "authenticated", // Standard Supabase audience
            ValidateIssuerSigningKey = true,
            ValidateLifetime = true,
            ClockSkew = TimeSpan.Zero
        };

        if (builder.Environment.IsDevelopment())
        {
            options.Events = new JwtBearerEvents
            {
                OnMessageReceived = context =>
                {
                    var authHeader = context.Request.Headers.Authorization.ToString();
                    // Дозволяємо тестування в Postman без живого Supabase токена
                    if (string.IsNullOrEmpty(authHeader) ||
                        authHeader.StartsWith("Bearer dev", StringComparison.OrdinalIgnoreCase) ||
                        authHeader.StartsWith("Bearer test", StringComparison.OrdinalIgnoreCase) ||
                        authHeader.Contains("mock-dev-jwt-token") ||
                        authHeader.Contains("DYX1I2"))
                    {
                        var claims = new[]
                        {
                            new Claim(ClaimTypes.NameIdentifier, "00000000-0000-0000-0000-000000000001"),
                            new Claim("sub", "00000000-0000-0000-0000-000000000001"),
                            new Claim(ClaimTypes.Email, "demo@devdialogue.io"),
                            new Claim("email", "demo@devdialogue.io"),
                            new Claim(ClaimTypes.Role, "admin"),
                            new Claim("role", "admin"),
                            new Claim("aud", "authenticated")
                        };
                        var identity = new ClaimsIdentity(claims, "DevMock");
                        context.Principal = new ClaimsPrincipal(identity);
                        context.Success();
                    }
                    return Task.CompletedTask;
                }
            };
        }
    });

builder.Services.AddAuthorization();

// CORS Policy
var allowedOrigins = (builder.Configuration["Cors:AllowedOrigins"] ?? "http://localhost:3000")
    .Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);

builder.Services.AddCors(options =>
{
    options.AddPolicy("AllowFrontend", policy =>
    {
        var configuredFrontend = Environment.GetEnvironmentVariable("FRONTEND_URL");
        var origins = new List<string>
        {
            "http://localhost:3000",
            "http://127.0.0.1:3000",
            "https://*.vercel.app",
            "https://*.onrender.com"
        };

        if (!string.IsNullOrWhiteSpace(configuredFrontend))
        {
            origins.Add(configuredFrontend.TrimEnd('/'));
        }

        policy.WithOrigins(origins.ToArray())
              .SetIsOriginAllowedToAllowWildcardSubdomains()
              .AllowAnyHeader()
              .AllowAnyMethod()
              .AllowCredentials();
    });
});

// Controllers & Swagger
builder.Services.AddControllers();

// Custom validation error response format
builder.Services.Configure<ApiBehaviorOptions>(options =>
{
    options.InvalidModelStateResponseFactory = context =>
    {
        var errors = context.ModelState
            .Where(e => e.Value?.Errors.Count > 0)
            .ToDictionary(
                kvp => kvp.Key,
                kvp => kvp.Value!.Errors.Select(er => er.ErrorMessage).ToArray()
            );

        return new BadRequestObjectResult(new
        {
            error = "Validation failed",
            code = "VALIDATION_ERROR",
            errors
        });
    };
});

builder.Services.AddEndpointsApiExplorer();

builder.Services.AddSwaggerGen(c =>
{
    c.SwaggerDoc("v1", new OpenApiInfo
    {
        Title = "Dev Dialogue API",
        Version = "v1",
        Description = "AI Interview Simulator Backend API for Dev Dialogue"
    });

    c.AddSecurityDefinition("Bearer", new OpenApiSecurityScheme
    {
        Description = "JWT Authorization header using the Bearer scheme. Example: 'Bearer {token}'",
        Name = "Authorization",
        In = ParameterLocation.Header,
        Type = SecuritySchemeType.Http,
        Scheme = "Bearer"
    });

    c.AddSecurityRequirement(new OpenApiSecurityRequirement
    {
        {
            new OpenApiSecurityScheme
            {
                Reference = new OpenApiReference
                {
                    Type = ReferenceType.SecurityScheme,
                    Id = "Bearer"
                }
            },
            Array.Empty<string>()
        }
    });
});

var app = builder.Build();

// Middleware Pipeline
app.UseExceptionHandler(errorApp =>
{
    errorApp.Run(async context =>
    {
        context.Response.StatusCode = StatusCodes.Status500InternalServerError;
        context.Response.ContentType = "application/json";
        await context.Response.WriteAsJsonAsync(new { error = "Internal server error", code = "INTERNAL_ERROR" });
    });
});

if (app.Environment.IsDevelopment() || builder.Configuration.GetValue<bool>("Swagger:Enabled"))
{
    app.UseSwagger();
    app.UseSwaggerUI(c =>
    {
        c.SwaggerEndpoint("/swagger/v1/swagger.json", "Dev Dialogue API v1");
        c.RoutePrefix = "swagger";
    });
}

app.UseCors("AllowFrontend");

// Serve embedded 3D Avatar assets from "3D Face Import/" at /avatar when present
var avatarRoot = app.Services.GetRequiredService<AvatarSpeechService>().AvatarRootPath;
if (!string.IsNullOrEmpty(avatarRoot) && Directory.Exists(avatarRoot))
{
    var avatarMimeTypes = new Microsoft.AspNetCore.StaticFiles.FileExtensionContentTypeProvider();
    avatarMimeTypes.Mappings[".glb"] = "model/gltf-binary";
    avatarMimeTypes.Mappings[".gltf"] = "model/gltf+json";
    app.UseStaticFiles(new StaticFileOptions
    {
        FileProvider = new Microsoft.Extensions.FileProviders.PhysicalFileProvider(avatarRoot),
        RequestPath = "/avatar",
        ContentTypeProvider = avatarMimeTypes,
        ServeUnknownFileTypes = false
    });
}

app.UseAuthentication();
app.UseAuthorization();

app.MapControllers();
app.MapSqliteCopyEndpoints();

// Seed Initial Data (if QuestionBank is empty)
using (var scope = app.Services.CreateScope())
{
    var modeState = scope.ServiceProvider.GetRequiredService<DatabaseModeState>();
    var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
    if (modeState.UseSqlite)
    {
        await db.Database.EnsureCreatedAsync();
        await SqliteMirrorExtensions.EnsureSqliteSchemaUpgradedAsync(db);
        app.Logger.LogInformation("Database mode: Encrypted SQLite ({SqliteFile})", modeState.SqliteFilePath);
    }

    var existingQuestions = await db.QuestionsBank.ToListAsync();
    var existingTexts = existingQuestions
        .Select(q => q.QuestionText)
        .ToHashSet(StringComparer.OrdinalIgnoreCase);

    var seedPool = DevDialogue.Api.Services.AI.QuestionBankSelector.BuildDefaultSeedQuestions();
    var addedAny = false;
    foreach (var seed in seedPool)
    {
        var match = existingQuestions.FirstOrDefault(q =>
            string.Equals(q.QuestionText, seed.QuestionText, StringComparison.OrdinalIgnoreCase));
        if (match == null)
        {
            db.QuestionsBank.Add(seed);
            addedAny = true;
        }
        else if (string.IsNullOrWhiteSpace(match.ExpectedAnswer))
        {
            match.ExpectedAnswer = seed.ExpectedAnswer;
            addedAny = true;
        }
    }

    if (addedAny)
    {
        await db.SaveChangesAsync();
    }

    await SqliteMirrorExtensions.SyncSqliteCopyAsync(db, modeState.SqliteConnectionString, modeState.SqliteFilePath);
}

app.Run();
