using System.Text;
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

// Database Configuration (PostgreSQL with in-memory fallback)
var connectionString = builder.Configuration.GetConnectionString("DefaultConnection")
                    ?? Environment.GetEnvironmentVariable("CONNECTION_STRING");

builder.Services.AddDbContext<AppDbContext>(options =>
{
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

// AI Service Registration (Cloud or Mock)
var aiProvider = builder.Configuration["AI:Provider"] ?? "Mock";
var aiApiKey = builder.Configuration["AI:ApiKey"] ?? Environment.GetEnvironmentVariable("AI_API_KEY");

if (string.Equals(aiProvider, "Cloud", StringComparison.OrdinalIgnoreCase) && !string.IsNullOrWhiteSpace(aiApiKey))
{
    builder.Services.AddHttpClient<IAiService, CloudAiService>();
}
else
{
    builder.Services.AddSingleton<IAiService, MockAiService>();
}

// Supabase JWT Authentication
var jwtSecret = builder.Configuration["Supabase:JwtSecret"]
             ?? Environment.GetEnvironmentVariable("SUPABASE_JWT_SECRET");

if (string.IsNullOrWhiteSpace(jwtSecret))
{
    jwtSecret = "super-secret-dev-dialogue-local-jwt-token-key-2026-min-32-chars!";
}

var keyBytes = Encoding.UTF8.GetBytes(jwtSecret);

builder.Services.AddAuthentication(options =>
{
    options.DefaultAuthenticateScheme = JwtBearerDefaults.AuthenticationScheme;
    options.DefaultChallengeScheme = JwtBearerDefaults.AuthenticationScheme;
})
.AddJwtBearer(options =>
{
    options.RequireHttpsMetadata = false;
    options.SaveToken = true;
    options.TokenValidationParameters = new TokenValidationParameters
    {
        ValidateIssuerSigningKey = true,
        IssuerSigningKey = new SymmetricSecurityKey(keyBytes),
        ValidateIssuer = false,
        ValidateAudience = false,
        ClockSkew = TimeSpan.Zero
    };
});

builder.Services.AddAuthorization();

// ==========================================
// 4. CORS Policy (Next.js Frontend & Local Dev)
// ==========================================
var allowedOrigins = (builder.Configuration["Cors:AllowedOrigins"] ?? "http://localhost:3000")
    .Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);

builder.Services.AddCors(options =>
{
    options.AddPolicy("AllowFrontend", policy =>
    {
        policy.WithOrigins(allowedOrigins)
            .SetIsOriginAllowed(origin => 
            {
                var host = new Uri(origin).Host;
                return host == "localhost" || host.EndsWith(".vercel.app");
            })
            .AllowAnyHeader()
            .AllowAnyMethod()
            .AllowCredentials();
    });
});

// Controllers & Swagger
builder.Services.AddControllers();

// Перевизначення формату помилок валідації (ModelState)
builder.Services.Configure<ApiBehaviorOptions>(options =>
{
    options.InvalidModelStateResponseFactory = context =>
        new BadRequestObjectResult(new { error = "Validation failed", code = "VALIDATION_ERROR" });
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

// ==========================================
// 6. Middleware Pipeline
// ==========================================
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

app.UseAuthentication();
app.UseAuthorization();

app.MapControllers();

// Seed Initial Data (if QuestionBank is empty)
using (var scope = app.Services.CreateScope())
{
    var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
    if (!db.QuestionsBank.Any())
    {
        db.QuestionsBank.AddRange(
            new QuestionBank
            {
                Direction = "frontend",
                Level = "junior",
                Stage = "hard_skills",
                Topic = "React Components & State",
                QuestionText = "Поясніть різницю між controlled та uncontrolled компонентами у формах."
            },
            new QuestionBank
            {
                Direction = "backend",
                Level = "junior",
                Stage = "hard_skills",
                Topic = "Networking & Protocols",
                QuestionText = "Поясніть різницю між TCP та UDP. У яких сценаріях UDP є кращим вибором?"
            },
            new QuestionBank
            {
                Direction = "backend",
                Level = "junior",
                Stage = "hard_skills",
                Topic = "Databases & Indexing",
                QuestionText = "Як працюють індекси в базах даних (зокрема B-Tree) та коли індекс може нашкодити продуктивності?"
            },
            new QuestionBank
            {
                Direction = "devops",
                Level = "junior",
                Stage = "hard_skills",
                Topic = "Docker Fundamentals",
                QuestionText = "Поясніть різницю між Docker контейнером та віртуальною машиною."
            }
        );
        db.SaveChanges();
    }
}

app.Run();
