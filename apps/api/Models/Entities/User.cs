using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace DevDialogue.Api.Models.Entities;

[Table("users", Schema = "public")]
public class User
{
    [Key]
    [Column("id")]
    public Guid Id { get; set; }

    [Required]
    [Column("email")]
    public string Email { get; set; } = string.Empty;

    [Column("full_name")]
    public string? FullName { get; set; }

    [Column("avatar_url")]
    public string? AvatarUrl { get; set; }

    [Required]
    [Column("role")]
    public string Role { get; set; } = "user"; // "user" or "admin"

    [Column("daily_interviews_count")]
    public int DailyInterviewsCount { get; set; } = 0;

    [Column("last_interview_date")]
    public DateOnly? LastInterviewDate { get; set; }

    [Column("created_at")]
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;

    [Column("updated_at")]
    public DateTime UpdatedAt { get; set; } = DateTime.UtcNow;

    // Navigation
    public ICollection<Interview> Interviews { get; set; } = new List<Interview>();
}
