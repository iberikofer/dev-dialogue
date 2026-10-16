# Dev Dialogue

<p align="center">
  <b>🌐 Language | Мова:</b><br>
  <a href="#-dev-dialogue-ai-interview-simulator"><b>English</b></a> | <a href="#-dev-dialogue-симулятор-співбесід-українська"><b>Українська</b></a>
</p>

---

## 🇬🇧 Dev Dialogue (AI Interview Simulator)

An interactive technical, behavioral, and language interview simulation platform powered by an adaptive AI interviewer. Designed for the student software engineering olympiad at VNTU (IPZ FITKI).

### 🚀 Technology Stack

| Layer                | Technology                               | Purpose                                                                         |
| :------------------- | :--------------------------------------- | :------------------------------------------------------------------------------ |
| **Monorepo**         | **Turborepo** + **pnpm v12**             | Fast caching, script orchestration, and workspace isolation                     |
| **Frontend**         | **Next.js 14** (App Router) + TypeScript | Responsive UI, interactive chat room, scorecard dashboard                       |
| **Design System**    | **Tailwind CSS** + **Geist Font**        | High-craft UI based on David Sanders Taste & Anthropic Frontend standards       |
| **State Management** | **Zustand**                              | Session persistence, client tokens, and instantaneous Admin/User mode toggle    |
| **Backend**          | **ASP.NET Core (.NET 9)** + C#           | High-performance REST API, session state machine, rate limits, security         |
| **Database & Auth**  | **Supabase** (PostgreSQL 16 + JWT)       | Relational storage for users, question banks, and JSONB transcripts             |
| **AI Layer**         | `IAiService` (Mock + Cloud API)          | Modular abstraction: offline zero-latency mock or cloud LLM (Groq / OpenRouter) |
| **Code Formatting**  | **Prettier**                             | Unified formatting rules across `.ts`, `.tsx`, `.md`, and `.json`               |
| **Deployment**       | **Vercel** (Web) + **Render** (API)      | Automated GitHub CI/CD pipelines and cloud hosting                              |

---

### 📁 Monorepo Structure

```text
dev-dialogue/
├── apps/
│   ├── web/                    # Next.js 14 frontend application
│   │   ├── src/app/            # App Router (/, /dashboard, /interview/*, /admin)
│   │   ├── src/components/     # UI components (Navbar, ChatRoom, PerformanceReport, etc.)
│   │   ├── src/lib/            # Typed API client, Supabase client, utilities
│   │   └── src/store/          # Zustand auth & dynamic view-mode store
│   └── api/                    # ASP.NET Core Web API
│       ├── Controllers/        # Auth, Interviews, AdminQuestions, Health
│       ├── Data/               # AppDbContext (PostgreSQL / In-Memory fallback)
│       ├── Models/             # Entities (User, Interview, QuestionBank) & DTOs
│       ├── Security/           # JWT validation & [RequireRole("admin")] attribute
│       └── Services/AI/        # IAiService (MockAiService + CloudAiService)
│
├── packages/
│   └── api-contracts/          # Shared TypeScript interfaces & DTO contracts
│
├── .agents/                    # Workspace agent customizations, skills, & rules
├── docker-compose.yml          # Containerized API + PostgreSQL local environment
├── turbo.json                  # Turborepo task pipeline configuration
├── pnpm-workspace.yaml         # pnpm workspace definition
└── package.json                # Monorepo scripts & dependencies
```

---

### 🛠 Quick Start for Development

#### 1. Install Dependencies

```bash
pnpm install
```

#### 2. Start the Development Server

```bash
# Starts Next.js (http://localhost:3000) and .NET API (http://localhost:5238) concurrently
pnpm dev
```

You can also run applications individually:

```bash
# Frontend only
pnpm --filter @dev-dialogue/web dev

# Backend only
cd apps/api && dotnet watch run
```

#### 3. Format Code (Prettier)

```bash
pnpm format
```

#### 4. Build the Monorepo (Production Check)

```bash
pnpm build
```

#### 5. Docker Local Setup (Optional)

If you prefer running the backend and PostgreSQL in isolated containers without installing local SDKs:

```bash
docker compose up --build
```

---

### 🧠 Flexible AI Architecture (`IAiService`)

The AI subsystem is engineered for zero-dependency offline prototyping and seamless production switching:

1. **`MockAiService` (Default & Offline):** Generates domain-tailored technical questions for Frontend, Backend, DevOps, QA, and PM tracks across Trainee, Junior, and Middle levels in Ukrainian and English. Produces detailed analytical scorecards with scores, strengths, and study plans.
2. **`CloudAiService`:** Ready for Groq (Llama 3.1 8B), OpenRouter (Qwen 2.5 72B), or any OpenAI-compatible API. Switchable via `appsettings.json` or environment variables:
   ```json
   "AI": {
     "Provider": "Cloud",
     "BaseUrl": "https://api.groq.com/openai/v1",
     "ApiKey": "gsk_...",
     "Model": "llama-3.1-8b-instant"
   }
   ```

---

### 🛡 Roles & Admin Mode Switcher

- Users have either `user` (candidate) or `admin` roles.
- For administrators, a dedicated toggle button **"🛡 Admin View / 👤 Candidate View"** appears in the top Navbar.
- The toggle dynamically switches client tool visibility via Zustand without invalidating the active session, allowing administrators and olympiad judges to preview the candidate experience instantly.
- Question bank management endpoints are enforced server-side via the `[RequireRole("admin")]` attribute.

---

---

## 🇺🇦 Dev Dialogue (Симулятор Співбесід) — Українська

Платформа інтерактивної підготовки до технічних, поведінкових та англомовних IT-співбесід з адаптивним AI-інтерв'юером, розроблена для студентської олімпіади ВНТУ (ІПЗ ФІТКІ).

[⬆ Повернутися до англійської версії](#-dev-dialogue-ai-interview-simulator)

### 🚀 Технологічний стек

| Шар                  | Технологія                                   | Призначення                                                           |
| :------------------- | :------------------------------------------- | :-------------------------------------------------------------------- |
| **Монорепозиторій**  | **Turborepo** + **pnpm v12**                 | Кешування, оркестрація збірок, лінтингу та воркспейсів                |
| **Frontend**         | **Next.js 14** (App Router) + TypeScript     | Користувацький інтерфейс, інтерактивний чат, дашборд                  |
| **Дизайн-система**   | **Tailwind CSS** + **Шрифт Geist**           | High-craft дизайн на базі стандартів David Sanders Taste та Anthropic |
| **State Management** | **Zustand**                                  | Стан сесії, збереження токенів, миттєвий перемикач режимів адміна     |
| **Backend**          | **ASP.NET Core (.NET 9)** + C#               | REST API, життєвий цикл сесій, ліміти, рольова безпека                |
| **Database & Auth**  | **Supabase** (PostgreSQL 16 + JWT)           | Збереження користувачів, банку питань, транскриптів (JSONB)           |
| **AI Layer**         | `IAiService` (Mock + Cloud API)              | Модульна абстракція: миттєвий локальний мок або хмарні LLM            |
| **Форматування**     | **Prettier**                                 | Єдиний стиль коду для `.ts`, `.tsx`, `.md` та `.json`                 |
| **Деплой**           | **Vercel** (Frontend) + **Render** (Backend) | Автоматичний CI/CD деплой з GitHub                                    |

---

### 📁 Структура монорепозиторію

```text
dev-dialogue/
├── apps/
│   ├── web/                    # Next.js 14 фронтенд додаток
│   │   ├── src/app/            # App Router (/, /dashboard, /interview/*, /admin)
│   │   ├── src/components/     # UI компоненти (Navbar, ChatRoom, PerformanceReport, тощо)
│   │   ├── src/lib/            # Typed API client, Supabase клієнт, утиліти
│   │   └── src/store/          # Zustand auth & view-mode store
│   └── api/                    # ASP.NET Core Web API
│       ├── Controllers/        # Auth, Interviews, AdminQuestions, Health
│       ├── Data/               # AppDbContext (PostgreSQL / In-Memory fallback)
│       ├── Models/             # Entities (User, Interview, QuestionBank) & DTOs
│       ├── Security/           # JWT валідація, [RequireRole("admin")] атрибут
│       └── Services/AI/        # IAiService (MockAiService + CloudAiService)
│
├── packages/
│   └── api-contracts/          # Спільні TypeScript типи та DTO контракти
│
├── .agents/                    # Скіли, конфігурації та правила ШІ-агентів
├── docker-compose.yml          # Локальний запуск API + PostgreSQL
├── turbo.json                  # Turborepo конфігурація пайплайну
├── pnpm-workspace.yaml         # Конфігурація воркспейсів
└── package.json                # Кореневий файл з командами
```

---

### 🛠 Швидкий старт для розробки

#### 1. Встановлення залежностей

```bash
pnpm install
```

#### 2. Запуск у режимі розробки

```bash
# Запуск Next.js (http://localhost:3000) та .NET API (http://localhost:5238) паралельно
pnpm dev
```

Також можна запускати сервіси окремо:

```bash
# Тільки фронтенд
pnpm --filter @dev-dialogue/web dev

# Тільки бекенд
cd apps/api && dotnet watch run
```

#### 3. Форматування коду (Prettier)

```bash
pnpm format
```

#### 4. Збірка всього монорепозиторію

```bash
pnpm build
```

#### 5. Запуск через Docker Compose (опціонально)

```bash
docker compose up --build
```

---

### 🧠 Гнучка AI-архітектура (`IAiService`)

Архітектура спроєктована так, щоб розробка, тестування та демонстрація не залежали від сторонніх ключів або балансу API:

1. **`MockAiService` (за замовчуванням):** Генерує реалістичні технічні питання для Frontend, Backend, DevOps, QA та PM на рівнях Trainee, Junior, Middle українською та англійською мовами, а також формує розгорнуті зведені звіти з оцінками.
2. **`CloudAiService`:** Повна реалізація для підключення Groq (Llama 3.1 8B), OpenRouter (Qwen 2.5 72B) або будь-якого сумісного OpenAI API через зміну налаштувань в `appsettings.json` або змінних середовища:
   ```json
   "AI": {
     "Provider": "Cloud",
     "BaseUrl": "https://api.groq.com/openai/v1",
     "ApiKey": "gsk_...",
     "Model": "llama-3.1-8b-instant"
   }
   ```

---

### 🛡 Ролі та режим адміністратора

- Користувачі мають ролі `user` або `admin`.
- Для адміністраторів у навбарі реалізовано кнопку перемикання **"🛡 Режим Адміна / 👤 Режим Кандидата"**.
- Перемикання змінює видимість адміністративних інструментів на клієнті через Zustand без створення нових сесій, дозволяючи тестувати інтерфейс з точки зору кандидата.
- Ендпоінти керування банком питань захищені серверним атрибутом `[RequireRole("admin")]`.
