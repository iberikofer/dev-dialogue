# Етап збірки (використовуємо .NET 9 SDK)
FROM mcr.microsoft.com/dotnet/sdk:9.0 AS build
WORKDIR /src

# Копіюємо файл проєкту і відновлюємо залежності
COPY ["DevDialogue.Api.csproj", "./"]
RUN dotnet restore "DevDialogue.Api.csproj"

# Копіюємо весь інший код і збираємо проєкт
COPY . .
RUN dotnet publish "DevDialogue.Api.csproj" -c Release -o /app/publish /p:UseAppHost=false

# Етап виконання (використовуємо легкий ASP.NET Runtime)
FROM mcr.microsoft.com/dotnet/aspnet:9.0 AS final
WORKDIR /app
EXPOSE 8080
COPY --from=build /app/publish .

# Вказуємо порт, який Render буде використовувати
ENV ASPNETCORE_HTTP_PORTS=8080

ENTRYPOINT ["dotnet", "DevDialogue.Api.dll"]