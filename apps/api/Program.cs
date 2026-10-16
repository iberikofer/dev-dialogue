using System.Text;
using DevDialogue.Api.Data;
using DevDialogue.Api.Models.Entities;
using DevDialogue.Api.Services.AI;
using Microsoft.AspNetCore.Authentication.JwtBearer;
using Microsoft.EntityFrameworkCore;
using Microsoft.IdentityModel.Tokens;
using Microsoft.OpenApi.Models;

var builder = WebApplication.CreateBuilder(args);

// ==========================================
// 1. Database Configuration (PostgreSQL / In-Memory fallback)
// ==========================================
var connectionString = builder.Configuration.GetConnectionString("DefaultConnection")
                    ?? Environment.GetEnvironmentVariable("CONNECTION_STRING");

builder.Services.AddDbContext<AppDbContext>(options =>
{
    if (!string.IsNullOrWhiteSpace(connectionString) && !connectionString.Contains("YOUR_POSTGRES"))
    {
        options.UseNpgsql(connectionString);
    }
    else
    {
        // Graceful in-memory fallback for immediate zero-config local development
        options.UseInMemoryDatabase("DevDialogueDevDb");
    }
});

// ==========================================
// 2. AI Service Registration (Cloud / Mock)
// ==========================================
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

// ==========================================
// 3. Supabase JWT Authentication
// ==========================================
var jwtSecret = builder.Configuration["Supabase:JwtSecret"]
             ?? Environment.GetEnvironmentVariable("SUPABASE_JWT_SECRET");

// Fallback secret for local development/testing if not yet set in Supabase
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
builder.Services.AddCors(options =>
{
    options.AddPolicy("AllowFrontend", policy =>
    {
        policy.WithOrigins(
                "http://localhost:3000",
                "http://127.0.0.1:3000",
                "https://*.vercel.app"
              )
              .SetIsOriginAllowedToAllowWildcardSubdomains()
              .AllowAnyHeader()
              .AllowAnyMethod()
              .AllowCredentials();
    });
});

// ==========================================
// 5. Controllers & Swagger Documentation
// ==========================================
builder.Services.AddControllers();
builder.Services.AddEndpointsApiExplorer();

builder.Services.AddSwaggerGen(c =>
{
    c.SwaggerDoc("v1", new OpenApiInfo
    {
        Title = "Dev Dialogue API",
        Version = "v1",
        Description = "AI Interview Simulator Backend API for Dev Dialogue"
    });

    // JWT Bearer documentation for Swagger UI
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
app.UseSwagger();
app.UseSwaggerUI(c =>
{
    c.SwaggerEndpoint("/swagger/v1/swagger.json", "Dev Dialogue API v1");
    c.RoutePrefix = "swagger";
});

app.UseCors("AllowFrontend");

app.UseAuthentication();
app.UseAuthorization();

app.MapControllers();

// ==========================================
// 7. Seed Initial Data (if QuestionBank is empty)
// ==========================================
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
