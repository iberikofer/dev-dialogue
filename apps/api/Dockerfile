FROM mcr.microsoft.com/dotnet/sdk:9.0 AS build
WORKDIR /src

COPY ["apps/api/DevDialogue.Api.csproj", "apps/api/"]
RUN dotnet restore "apps/api/DevDialogue.Api.csproj"

COPY apps/api/ apps/api/
WORKDIR "/src/apps/api"
RUN dotnet publish "DevDialogue.Api.csproj" -c Release -o /app/publish /p:UseAppHost=false

FROM mcr.microsoft.com/dotnet/aspnet:9.0 AS final
WORKDIR /app
COPY --from=build /app/publish .

ENV ASPNETCORE_URLS=http://+:5000
EXPOSE 5000

ENTRYPOINT ["dotnet", "DevDialogue.Api.dll"]
