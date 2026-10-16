using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace DevDialogue.Api.Models.Entities;

[Table("questions_bank", Schema = "public")]
public class QuestionBank
{
    [Key]
    [Column("id")]
    public Guid Id { get; set; } = Guid.NewGuid();

    [Required]
    [Column("direction")]
    public string Direction { get; set; } = "frontend";

    [Required]
    [Column("stage")]
    public string Stage { get; set; } = "hard_skills";

    [Required]
    [Column("level")]
    public string Level { get; set; } = "junior";

    [Required]
    [Column("topic")]
    public string Topic { get; set; } = string.Empty;

    [Required]
    [Column("question_text")]
    public string QuestionText { get; set; } = string.Empty;

    [Column("is_active")]
    public bool IsActive { get; set; } = true;

    [Column("created_by")]
    public Guid? CreatedBy { get; set; }

    [ForeignKey(nameof(CreatedBy))]
    public User? Creator { get; set; }

    [Column("created_at")]
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}
