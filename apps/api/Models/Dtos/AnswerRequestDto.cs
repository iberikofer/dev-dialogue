using System.ComponentModel.DataAnnotations;
using Microsoft.EntityFrameworkCore;

namespace DevDialogue.Api.Models.Dtos
{
    public class AnswerRequestDto
    {
        [Required(ErrorMessage = "Відповідь не може бути порожньою")]
        [MaxLength(3000, ErrorMessage = "Відповідь перевищує ліміт у 3000 символів")]
        public string UserAnswer { get; set; } = string.Empty;
        
        [Required]
        public Guid QuestionId { get; set; }
    }
}
