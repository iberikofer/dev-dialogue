using DevDialogue.Api.Models.Entities;
using Microsoft.EntityFrameworkCore;

namespace DevDialogue.Api.Data;

public class AppDbContext : DbContext
{
    public AppDbContext(DbContextOptions<AppDbContext> options) : base(options)
    {
    }

    public DbSet<User> Users => Set<User>();
    public DbSet<Interview> Interviews => Set<Interview>();
    public DbSet<QuestionBank> QuestionsBank => Set<QuestionBank>();

    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        base.OnModelCreating(modelBuilder);

        // Users mapping
        modelBuilder.Entity<User>(entity =>
        {
            entity.ToTable("users", "public");
            entity.HasKey(u => u.Id);
            entity.Property(u => u.Email).IsRequired();
            entity.Property(u => u.Role).HasDefaultValue("user");
            entity.Property(u => u.DailyInterviewsCount).HasDefaultValue(0);
        });

        // Interviews mapping
        modelBuilder.Entity<Interview>(entity =>
        {
            entity.ToTable("interviews", "public");
            entity.HasKey(i => i.Id);
            entity.Property(i => i.Transcript).HasColumnType("jsonb").HasDefaultValue("[]");
            entity.Property(i => i.Feedback).HasColumnType("jsonb");
            entity.Property(i => i.Language).HasDefaultValue("uk");
            entity.Property(i => i.Stage).HasDefaultValue("hard_skills");
            entity.Property(i => i.Status).HasDefaultValue("active");

            entity.HasOne(i => i.User)
                  .WithMany(u => u.Interviews)
                  .HasForeignKey(i => i.UserId)
                  .OnDelete(DeleteBehavior.Cascade);

            entity.HasIndex(i => i.UserId).HasDatabaseName("idx_interviews_user_id");
            entity.HasIndex(i => i.CreatedAt).HasDatabaseName("idx_interviews_created_at");
        });

        // QuestionsBank mapping
        modelBuilder.Entity<QuestionBank>(entity =>
        {
            entity.ToTable("questions_bank", "public");
            entity.HasKey(q => q.Id);
            entity.Property(q => q.IsActive).HasDefaultValue(true);

            entity.HasOne(q => q.Creator)
                  .WithMany()
                  .HasForeignKey(q => q.CreatedBy)
                  .OnDelete(DeleteBehavior.SetNull);

            entity.HasIndex(q => new { q.Direction, q.Level }).HasDatabaseName("idx_questions_direction_level");
        });
    }
}
