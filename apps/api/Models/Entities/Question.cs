namespace DevDialogue.Models;

public class Question
{
    public int Id { get; set; }
    public string Content { get; set; } = "";
    public string ExpectedAnswer { get; set; } = "";
    public string Category { get; set; } = "";
    public int DifficultyLevel { get; set; }
    public string? Role { get; set; }
    public string? Level { get; set; }
    public string? Language { get; set; }
    public string? Framework { get; set; }
}