using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace DevDialogue.Api.Models.Entities;

[Table("interviews", Schema = "public")]
public class Interview
{
    [Key]
    [Column("id")]
    public Guid Id { get; set; } = Guid.NewGuid();

    [Required]
    [Column("user_id")]
    public Guid UserId { get; set; }

    [ForeignKey(nameof(UserId))]
    public User? User { get; set; }

    [Required]
    [Column("direction")]
    public string Direction { get; set; } = "frontend"; // 'frontend', 'backend', 'devops', 'pm', 'qa'

    [Required]
    [Column("level")]
    public string Level { get; set; } = "junior"; // 'trainee', 'junior', 'middle'

    [Required]
    [Column("language")]
    public string Language { get; set; } = "uk"; // 'uk', 'en'

    [Required]
    [Column("stage")]
    public string Stage { get; set; } = "hard_skills"; // 'hard_skills', 'soft_skills', 'english'

    [Required]
    [Column("status")]
    public string Status { get; set; } = "active"; // 'active', 'completed', 'abandoned'

    [Column("score")]
    public int? Score { get; set; } // 0 - 100

    [Column("transcript", TypeName = "jsonb")]
    public string Transcript { get; set; } = "[]";

    [Column("feedback", TypeName = "jsonb")]
    public string? Feedback { get; set; }

    [Column("question_count")]
    public int QuestionCount { get; set; } = 0;

    [Column("created_at")]
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;

    [Column("updated_at")]
    public DateTime UpdatedAt { get; set; } = DateTime.UtcNow;
}
