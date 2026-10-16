using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace DevDialogue.Api.Migrations
{
    /// <inheritdoc />
    public partial class AddNewTables : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.EnsureSchema(
                name: "public");

            migrationBuilder.CreateTable(
                name: "users",
                schema: "public",
                columns: table => new
                {
                    id = table.Column<Guid>(type: "uuid", nullable: false),
                    email = table.Column<string>(type: "text", nullable: false),
                    full_name = table.Column<string>(type: "text", nullable: true),
                    avatar_url = table.Column<string>(type: "text", nullable: true),
                    role = table.Column<string>(type: "text", nullable: false, defaultValue: "user"),
                    daily_interviews_count = table.Column<int>(type: "integer", nullable: false, defaultValue: 0),
                    last_interview_date = table.Column<DateOnly>(type: "date", nullable: true),
                    created_at = table.Column<DateTime>(type: "timestamp with time zone", nullable: false),
                    updated_at = table.Column<DateTime>(type: "timestamp with time zone", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_users", x => x.id);
                });

            migrationBuilder.CreateTable(
                name: "interviews",
                schema: "public",
                columns: table => new
                {
                    id = table.Column<Guid>(type: "uuid", nullable: false),
                    user_id = table.Column<Guid>(type: "uuid", nullable: false),
                    direction = table.Column<string>(type: "text", nullable: false),
                    level = table.Column<string>(type: "text", nullable: false),
                    language = table.Column<string>(type: "text", nullable: false, defaultValue: "uk"),
                    stage = table.Column<string>(type: "text", nullable: false, defaultValue: "hard_skills"),
                    status = table.Column<string>(type: "text", nullable: false, defaultValue: "active"),
                    score = table.Column<int>(type: "integer", nullable: true),
                    transcript = table.Column<string>(type: "jsonb", nullable: false, defaultValue: "[]"),
                    feedback = table.Column<string>(type: "jsonb", nullable: true),
                    question_count = table.Column<int>(type: "integer", nullable: false),
                    created_at = table.Column<DateTime>(type: "timestamp with time zone", nullable: false),
                    updated_at = table.Column<DateTime>(type: "timestamp with time zone", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_interviews", x => x.id);
                    table.ForeignKey(
                        name: "FK_interviews_users_user_id",
                        column: x => x.user_id,
                        principalSchema: "public",
                        principalTable: "users",
                        principalColumn: "id",
                        onDelete: ReferentialAction.Cascade);
                });

            migrationBuilder.CreateTable(
                name: "questions_bank",
                schema: "public",
                columns: table => new
                {
                    id = table.Column<Guid>(type: "uuid", nullable: false),
                    direction = table.Column<string>(type: "text", nullable: false),
                    stage = table.Column<string>(type: "text", nullable: false),
                    level = table.Column<string>(type: "text", nullable: false),
                    topic = table.Column<string>(type: "text", nullable: false),
                    question_text = table.Column<string>(type: "text", nullable: false),
                    is_active = table.Column<bool>(type: "boolean", nullable: false, defaultValue: true),
                    created_by = table.Column<Guid>(type: "uuid", nullable: true),
                    created_at = table.Column<DateTime>(type: "timestamp with time zone", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_questions_bank", x => x.id);
                    table.ForeignKey(
                        name: "FK_questions_bank_users_created_by",
                        column: x => x.created_by,
                        principalSchema: "public",
                        principalTable: "users",
                        principalColumn: "id",
                        onDelete: ReferentialAction.SetNull);
                });

            migrationBuilder.CreateIndex(
                name: "idx_interviews_created_at",
                schema: "public",
                table: "interviews",
                column: "created_at");

            migrationBuilder.CreateIndex(
                name: "idx_interviews_user_id",
                schema: "public",
                table: "interviews",
                column: "user_id");

            migrationBuilder.CreateIndex(
                name: "idx_questions_direction_level",
                schema: "public",
                table: "questions_bank",
                columns: new[] { "direction", "level" });

            migrationBuilder.CreateIndex(
                name: "IX_questions_bank_created_by",
                schema: "public",
                table: "questions_bank",
                column: "created_by");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "interviews",
                schema: "public");

            migrationBuilder.DropTable(
                name: "questions_bank",
                schema: "public");

            migrationBuilder.DropTable(
                name: "users",
                schema: "public");
        }
    }
}
