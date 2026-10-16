using DevDialogue.Api.Data;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.Filters;
using Microsoft.EntityFrameworkCore;

namespace DevDialogue.Api.Security;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Method)]
public class RequireRoleAttribute : TypeFilterAttribute
{
    public RequireRoleAttribute(string role) : base(typeof(RoleRequirementFilter))
    {
        Arguments = new object[] { role };
    }
}

public class RoleRequirementFilter : IAsyncAuthorizationFilter
{
    private readonly string _requiredRole;
    private readonly AppDbContext _dbContext;

    public RoleRequirementFilter(string requiredRole, AppDbContext dbContext)
    {
        _requiredRole = requiredRole;
        _dbContext = dbContext;
    }

    public async Task OnAuthorizationAsync(AuthorizationFilterContext context)
    {
        var user = context.HttpContext.User;
        if (user.Identity?.IsAuthenticated != true)
        {
            context.Result = new UnauthorizedResult();
            return;
        }

        try
        {
            var userId = user.GetUserId();
            var dbUser = await _dbContext.Users
                .AsNoTracking()
                .FirstOrDefaultAsync(u => u.Id == userId);

            if (dbUser == null || !string.Equals(dbUser.Role, _requiredRole, StringComparison.OrdinalIgnoreCase))
            {
                context.Result = new ForbidResult();
            }
        }
        catch
        {
            context.Result = new UnauthorizedResult();
        }
    }
}
