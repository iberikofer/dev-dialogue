FROM mcr.microsoft.com/dotnet/sdk:9.0 AS build
WORKDIR /src

COPY . .

WORKDIR /src/apps/api

RUN dotnet restore "DevDialogue.Api.csproj"
RUN dotnet publish "DevDialogue.Api.csproj" -c Release -o /app/publish /p:UseAppHost=false

FROM mcr.microsoft.com/dotnet/aspnet:9.0 AS final
WORKDIR /app
EXPOSE 8080
COPY --from=build /app/publish .

ENV ASPNETCORE_HTTP_PORTS=8080

ENTRYPOINT ["dotnet", "DevDialogue.Api.dll"]