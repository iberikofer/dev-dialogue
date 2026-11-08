# Звіт з аудиту DTO, мережевої архітектури та плану реалізації
**Гілка:** `feature/desktop-network`  
**Відповідальний:** Даня (файли `02_data_models_and_networking.md` та `03_state_management_and_auth.md`)  
**Дата аудиту:** 06.11.2026  

---

## 1. Вступ та контекст задачі

Згідно з розподілом задач команди переведення проекту **Dev Dialogue** на C++ десктоп:
- **Даня (наша частина)** — файли **02** і **03** (моделі даних DTO, серіалізація `nlohmann::json`, асинхронний `ApiClient` на базі `QNetworkAccessManager`, безпечне сховище токенів `ISecureStorage`, сервіси `AuthService`, `InterviewService`, `AdminService`, а також `AuthViewModel` та `NavigationViewModel`).
- **Віталік** — файл **04** (UI, QML, екрани, компоненти).
- **Саня** — файли **05** і **06** (офлайн-рушій `MockAiEngine`, i18n/Linguist, тести та CI/CD).

Цей звіт містить вичерпні результати порівняння контрактів `packages/api-contracts`, реального API на ASP.NET Core (`apps/api`) та десктопних специфікацій із документації, а також фінальний технічний план реалізації перед початком написання коду.

---

## 2. Звірка DTO: `api-contracts` vs `apps/api` vs Doc 02

### 2.1. Перерахування (Enums)

| Enum | Значення в `packages/api-contracts` | Значення в `apps/api` (`SupportedOptions.cs`) | Статус та рішення для C++ |
| :--- | :--- | :--- | :--- |
| **`UserRole`** | `user`, `admin` | `user`, `admin` | Повний збіг. Додаємо `UserRole::User`, `UserRole::Admin`. |
| **`InterviewDirection`** | `frontend`, `backend`, `devops`, `pm`, `qa` | `frontend`, `backend`, `devops`, `qa`, `pm` | Повний збіг. |
| **`InterviewLevel`** | `trainee`, `junior`, `middle` | `junior`, `middle`, `senior` | **Невідповідність:** у бекенді з'явився рівень `senior`, а в TS був `trainee`. У C++ enum підтримуємо обидва: `Trainee`, `Junior`, `Middle`, `Senior`. |
| **`InterviewLanguage`** | `uk`, `en` | `uk`, `en` | Повний збіг. |
| **`InterviewStage`** | `hard_skills`, `soft_skills`, `english` | `hard_skills`, `soft_skills`, `english` | Повний збіг. |
| **`InterviewStatus`** | `active`, `completed`, `abandoned` | `active`, `completed` | Повний збіг. Додаємо `Active`, `Completed`, `Abandoned`. |

---

### 2.2. Моделі даних (Data Transfer Objects)

#### 1. `UserProfile` та `AuthMeResponse`
* **TypeScript:** `id`, `email`, `fullName` (string | null), `avatarUrl` (string | null), `role: UserRole`.
* **C# (`AuthDtos.cs`):** `Guid Id`, `string Email`, `string? FullName`, `string? AvatarUrl`, `string Role`.
* **Рішення для C++:**
  * `std::string id`, `std::string email`, `std::optional<std::string> fullName`, `std::optional<std::string> avatarUrl`, `UserRole role`.

#### 2. `InterviewMessage`
* **TypeScript:** `role` ("system" | "assistant" | "user"), `content`, `timestamp`.
* **C# (`InterviewDtos.cs`):** `string Role`, `string Content`, `DateTime Timestamp`.
* **Рішення для C++:** `std::string role`, `std::string content`, `std::string timestamp`.

#### 3. `InterviewFeedbackReport`
* **TypeScript:** `summary`, `score`, `strengths`, `weaknesses`, `recommendations`, `questionScores: QuestionScoreItem[]`.
* **C# (`InterviewDtos.cs`):** 
  ```csharp
  [property: JsonPropertyName("question_scores")] List<QuestionScoreDto> QuestionScores
  ```
* **Рішення для C++:**
  * **Критична вимога:** у C# задано явний атрибут `question_scores` (snake_case). 
  * У C++ десеріалізаторі `from_json` підтримуємо перевірку **обох ключів**:
    ```cpp
    if (j.contains("questionScores")) {
        j.at("questionScores").get_to(r.questionScores);
    } else if (j.contains("question_scores")) {
        j.at("question_scores").get_to(r.questionScores);
    }
    ```

#### 4. `InterviewSummary` та `InterviewDetail`
* **TypeScript:** `id`, `userId`, `direction`, `level`, `language`, `stage`, `status`, `score` (number | null), `questionCount`, `createdAt`, `updatedAt`. В `InterviewDetail` додаються `transcript` та `feedback` (null | object).
* **C# (`InterviewDtos.cs`):** `Guid Id`, `Guid UserId`, `int? Score`, тощо.
* **Рішення для C++:** `score` має тип `std::optional<int>`, `feedback` має тип `std::optional<InterviewFeedbackReport>`.

#### 5. `CreateInterviewRequest` та `CreateInterviewResponse`
* **Запит:** `direction`, `level`, `language`.
* **Відповідь:** `interviewId` (UUID string), `firstQuestion` (string).
* **Рішення для C++:** Типізовані поля `direction`, `level`, `language` з автоматичною серіалізацією рядкових значень enums.

#### 6. `SendMessageRequest` та `SendMessageResponse`
* **Запит:** `{ "content": "..." }`.
* **Відповідь:** `{ "message": "...", "questionCount": 2, "isCompleted": false }`.
* **Рішення для C++:** Повний збіг.

#### 7. `QuestionBankItem`, `CreateQuestionRequest`, `UpdateQuestionRequest`
* **TypeScript:** повний CRUD для адмінки.
* **C# (`AdminDtos.cs`):** валідація довжини рядків, необов'язкові поля в `UpdateQuestionDto`.
* **Рішення для C++:** У `UpdateQuestionRequest` усі поля робимо `std::optional<T>`, що дозволяє часткове оновлення (PATCH/PUT-семантика).

#### 8. `PagedResult<T>`
* **TypeScript & C#:** `{ "items": [...], "page": 1, "pageSize": 20, "totalCount": 4, "totalPages": 1 }`.
* **Рішення для C++:** Шаблонна структура `template<typename T> struct PagedResult` із власною серіалізацією.

#### 9. Підтримка `std::optional` у `nlohmann::json`
* **Проблема:** Стандартні макроси `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE` не підтримують `std::optional` без кастомного серіалізатора.
* **Рішення:** Реалізація `nlohmann::adl_serializer<std::optional<T>>` у просторі імен `nlohmann`:
  ```cpp
  namespace nlohmann {
  template <typename T>
  struct adl_serializer<std::optional<T>> {
      static void to_json(json& j, const std::optional<T>& opt) {
          if (opt.has_value()) j = *opt; else j = nullptr;
      }
      static void from_json(const json& j, std::optional<T>& opt) {
          if (j.is_null()) opt = std::nullopt; else opt = j.get<T>();
      }
  };
  }
  ```

---

## 3. Звірка ендпоінтів та протоколів взаємодії

### 3.1. Ендпоінти ASP.NET Core API
* `GET /api/health/ping` — перевірка працездатності бекенду (Doc 02 містив помилку з `/api/health`).
* `GET /api/auth/me` — повертає `AuthMeDto` за Bearer JWT токеном (створює профіль автоматично при першому вході).
* `GET /api/options` — повертає доступні напрямки, рівні, етапи та мови з локалізованими назвами.
* `POST /api/interviews` — створення сесії співбесіди (повертає 404, якщо мову не підтримує сервер).
* `GET /api/interviews?page=1&pageSize=20` — пагінований список сесій поточного користувача.
* `GET /api/interviews/{id}` — отримання транскрипту та результатів сесії.
* `POST /api/interviews/{id}/message` — надсилання повідомлення кандидатом (ліміт 3000 символів).
* `POST /api/interviews/{id}/complete` — генерація підсумкового звіту та оцінки через ШІ.
* `GET /api/interviews/stats` — показники KPI дашборду.
* `GET /api/admin/questions` — пагінований список питань (вимагає роль `admin`, інакше 403 Forbidden).
* `POST /api/admin/questions` — створення питання в банку.
* `PUT /api/admin/questions/{id}` — оновлення питання.
* `DELETE /api/admin/questions/{id}` — видалення питання.

### 3.2. Автентифікація через Supabase REST API
Оскільки бекенд не має ендпоінтів логіну, десктопний клієнт напряму викликає Supabase Auth API:
1. **Логін:** `POST {supabaseUrl}/auth/v1/token?grant_type=password` з JSON `{ "email": "...", "password": "..." }`.
2. **Реєстрація:** `POST {supabaseUrl}/auth/v1/signup` з JSON `{ "email": "...", "password": "...", "data": { "full_name": "..." } }`.
3. **Оновлення токена:** `POST {supabaseUrl}/auth/v1/token?grant_type=refresh_token` з JSON `{ "refresh_token": "..." }`.
4. **Заголовки:** `apikey: {supabaseAnonKey}`, `Content-Type: application/json`.
5. **Подальша синхронізація:** Одразу після отримання `access_token` викликається `GET /api/auth/me` на бекенді для збереження профілю користувача.

---

## 4. Архітектура підсистем та компоненти нашої частини

### 4.1. Безпечне сховище токенів (`ISecureStorage`)
* **`ISecureStorage` (інтерфейс):**
  * `virtual bool writeCredential(const std::string& key, const std::string& secret) = 0;`
  * `virtual std::optional<std::string> readCredential(const std::string& key) = 0;`
  * `virtual bool deleteCredential(const std::string& key) = 0;`
* **Ключі:**
  * `StorageKeyAccessToken` = `"jwt_token"`
  * `StorageKeyRefreshToken` = `"refresh_token"`
* **`QtKeychainSecureStorage`:**
  * Використовує асинхронні завдання `QKeychain::WritePasswordJob`, `ReadPasswordJob`, `DeletePasswordJob` з локальним циклом очікування для синхронного інтерфейсу сховища.
* **`InMemorySecureStorage`:**
  * Потокобезпечне сховище на базі `std::unordered_map` з `std::mutex`.
  * Використовується в тестах та як безпечний fallback у Release-збірках при відсутності keychain (**ніяких незашифрованих файлів на диску**).

### 4.2. Мережевий клієнт (`ApiClient`)
* **Таймаути:**
  * `ColdStartTimeoutMs = 90000;` (90 с) для перевірки здоров'я та першого запиту (враховує засинання Render).
  * `StandardTimeoutMs = 30000;` (30 с) для стандартних робочих запитів.
* **Властивість `isWakingUp`:**
  * Прапорець сигналізує UI (через `Q_PROPERTY`), що сервер наразі пробуджується, якщо запит виконується довше 3 секунд.
* **Політика повторних спроб (Retry Policy):**
  * Повторюються **лише ідемпотентні GET-запити** (до 2 спроб із затримкою).
  * Для запитів `POST`, `PUT`, `DELETE` повтори заблоковані, щоб уникнути дублювання транзакцій і повідомлень у чаті.
* **Запобігання дублюванню Refresh токена:**
  * При отриманні статусу `401 Unauthorized` виставляється прапорець `m_isRefreshing`.
  * Усі наступні запити стають у чергу очікування оновлення токена і не генерують повторних запитів до Supabase.
  * Якщо рефреш успішний — оригінальні запити повторюються з новим токеном.
  * Якщо рефреш не вдався — генерується сигнал `unauthorizedDetected()`, сесія очищається.

### 4.3. Сервісний шар
1. **`AuthService`**:
   * Керування сесією, логін, реєстрація, рефреш токенів через Supabase REST.
   * `restoreSession()` при старті застосунку.
   * `mockLogin(role)` доступний **виключно в Debug-збірках** (`#ifndef NDEBUG`) або при увімкненому офлайн-режимі.
2. **`InterviewService`**:
   * Покриває всі операції над співбесідами (`createInterview`, `getInterviews`, `getInterview`, `sendMessage`, `completeInterview`, `getStats`).
3. **`AdminService`**:
   * Покриває адміністративні операції над банком питань (`getQuestions`, `createQuestion`, `updateQuestion`, `deleteQuestion`).

### 4.4. ViewModels
* **`AuthViewModel`**:
  * Реалізація згідно з контрактом із Документу 03 без змін публічного API (`isAuthenticated`, `isAdmin`, `isAdminViewActive`, `setAdminViewActive`, `toggleAdminView`, `mockLogin`, `loginWithEmail`, `registerUser`, `logout`).
* **`NavigationViewModel`**:
  * Підтверджена повна сумісність із чинним `NavigationViewModel` та `Main.qml`.

---

## 5. Зміни в системі збірки (`CMakeLists.txt` та `vcpkg.json`)

1. **`vcpkg.json`**:
   * Додається залежність `"qtkeychain"`.
2. **`CMakeLists.txt`**:
   * Додається пошук пакета `find_package(Qt6Keychain QUIET)`.
   * Лінкування `qt6keychain` (за наявності) до цілі `DevDialogue`.
   * Додаються нові заголовки та вихідні файли в таргет `DevDialogue` (`ApiClient`, `AuthService`, `InterviewService`, `AdminService`, `SecureStorage`, `AuthViewModel`).

---

## 6. Чеклист файлів до створення / оновлення

- [ ] `apps/desktop/include/models/ApiContracts.hpp` (оновлення з повними DTO та adl_serializer)
- [ ] `apps/desktop/include/core/Constants.hpp` (таймаути 90с/30с, ключі сховища)
- [ ] `apps/desktop/include/core/AppConfig.hpp` та `src/core/AppConfig.cpp` (Supabase URL / Anon Key)
- [ ] `apps/desktop/include/network/NetworkEndpoints.hpp` (константи шляхів REST)
- [ ] `apps/desktop/include/network/ApiClient.hpp` та `src/network/ApiClient.cpp`
- [ ] `apps/desktop/include/services/ISecureStorage.hpp`
- [ ] `apps/desktop/include/services/QtKeychainSecureStorage.hpp` та `src/services/QtKeychainSecureStorage.cpp`
- [ ] `apps/desktop/include/services/InMemorySecureStorage.hpp` та `src/services/InMemorySecureStorage.cpp`
- [ ] `apps/desktop/include/services/AuthService.hpp` та `src/services/AuthService.cpp`
- [ ] `apps/desktop/include/services/InterviewService.hpp` та `src/services/InterviewService.cpp`
- [ ] `apps/desktop/include/services/AdminService.hpp` та `src/services/AdminService.cpp`
- [ ] `apps/desktop/include/viewmodels/AuthViewModel.hpp` та `src/viewmodels/AuthViewModel.cpp`
- [ ] `apps/desktop/CMakeLists.txt` та `apps/desktop/vcpkg.json` (мінімальні необхідні правки)

---

> **Очікується підтвердження від користувача для переходу до реалізації коду.**
