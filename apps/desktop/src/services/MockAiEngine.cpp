#include "services/MockAiEngine.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace devdialogue::services {

namespace {

std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) {
                   return static_cast<char>(std::tolower(c));
                 });
  return value;
}

bool contains(const std::string &haystack, const std::string &needle) {
  return haystack.find(needle) != std::string::npos;
}

} // namespace

MockAiEngine::MockAiEngine() { initializeQuestionBank(); }

int MockAiEngine::directionKey(models::InterviewDirection direction) {
  return static_cast<int>(direction);
}

int MockAiEngine::levelKey(models::InterviewLevel level) {
  return static_cast<int>(level);
}

void MockAiEngine::addPool(models::InterviewDirection direction,
                           models::InterviewLevel level,
                           std::vector<std::string> uk,
                           std::vector<std::string> en) {
  QuestionPool pool;
  pool.questionsUk = std::move(uk);
  pool.questionsEn = std::move(en);
  m_questionBank[directionKey(direction)][levelKey(level)] = std::move(pool);
}

const MockAiEngine::QuestionPool &
MockAiEngine::poolFor(models::InterviewDirection direction,
                      models::InterviewLevel level) const {
  static const QuestionPool empty;
  auto directionIt = m_questionBank.find(directionKey(direction));
  if (directionIt == m_questionBank.end())
    return empty;
  auto levelIt = directionIt->second.find(levelKey(level));
  if (levelIt == directionIt->second.end())
    return empty;
  return levelIt->second;
}

std::string
MockAiEngine::generateFirstQuestion(models::InterviewDirection direction,
                                    models::InterviewLevel level,
                                    models::InterviewLanguage language) const {
  const QuestionPool &pool = poolFor(direction, level);
  const std::vector<std::string> &questions =
      language == models::InterviewLanguage::Uk ? pool.questionsUk
                                                : pool.questionsEn;
  if (questions.empty()) {
    return language == models::InterviewLanguage::Uk
               ? "Вітаю на співбесіді! Розкажіть коротко про себе та свій досвід."
               : "Welcome to the interview! Tell me briefly about yourself and "
                 "your experience.";
  }
  return questions.front();
}

std::string MockAiEngine::generateNextQuestion(
    models::InterviewDirection direction, models::InterviewLevel level,
    models::InterviewLanguage language, int currentQuestionCount,
    const std::string &candidateAnswer) const {
  const QuestionPool &pool = poolFor(direction, level);
  const std::vector<std::string> &questions =
      language == models::InterviewLanguage::Uk ? pool.questionsUk
                                                : pool.questionsEn;

  const std::size_t nextIndex =
      static_cast<std::size_t>(std::max(currentQuestionCount, 0));
  if (nextIndex < questions.size())
    return questions[nextIndex];

  (void)candidateAnswer;
  return language == models::InterviewLanguage::Uk
             ? "Дякую за розгорнуту відповідь! Усі питання цієї сесії "
               "вичерпано. Натисніть «Завершити співбесіду», щоб отримати "
               "детальний звіт."
             : "Thank you for the detailed answers! All questions for this "
               "session are complete. Press “Complete Interview” to see your "
               "detailed scorecard.";
}

int MockAiEngine::calculateAnswerScore(const std::string &answer) {
  if (answer.empty())
    return 0;

  const std::string lower = toLower(answer);
  const std::size_t length = answer.size();

  if (length < 20)
    return 45;
  if (length < 80)
    return 65;

  int score = 75;
  if (contains(lower, "because") || contains(lower, "тому що") ||
      contains(lower, "оскільки"))
    score += 5;
  if (contains(lower, "for example") || contains(lower, "наприклад"))
    score += 5;
  if (contains(lower, "trade-off") || contains(lower, "компроміс") ||
      contains(lower, "переваг"))
    score += 4;
  if (length > 200)
    score += 8;

  return std::min(score, 98);
}

models::InterviewFeedbackReport MockAiEngine::evaluateInterview(
    models::InterviewDirection direction, models::InterviewLevel /*level*/,
    models::InterviewLanguage language,
    const std::vector<models::InterviewMessage> &transcript) const {
  models::InterviewFeedbackReport report;
  std::vector<models::QuestionScoreItem> questionScores;
  int totalScore = 0;

  for (std::size_t i = 0; i < transcript.size(); ++i) {
    if (transcript[i].role != "assistant")
      continue;

    std::string answer;
    if (i + 1 < transcript.size() && transcript[i + 1].role == "user")
      answer = transcript[i + 1].content;

    models::QuestionScoreItem item;
    item.question = transcript[i].content;
    item.score = calculateAnswerScore(answer);
    totalScore += item.score;

    if (item.score >= 85) {
      item.comment =
          language == models::InterviewLanguage::Uk
              ? "Сильна, структурована відповідь із доречними прикладами та "
                "розумінням компромісів."
              : "Strong, well-structured answer with relevant examples and a "
                "clear grasp of trade-offs.";
    } else if (item.score >= 70) {
      item.comment =
          language == models::InterviewLanguage::Uk
              ? "Відповідь логічна, продемонстровано базове володіння "
                "концепціями, але бракує глибини."
              : "Logical answer showing basic command of the concepts, though "
                "it lacks depth.";
    } else {
      item.comment =
          language == models::InterviewLanguage::Uk
              ? "Відповідь надто коротка або поверховa: варто розкрити "
                "причини та навести приклад."
              : "The answer is too short or superficial: explain your reasoning "
                "and provide an example.";
    }

    questionScores.push_back(std::move(item));
  }

  const int finalScore = questionScores.empty()
                             ? 70
                             : totalScore / static_cast<int>(questionScores.size());

  report.score = finalScore;
  report.questionScores = questionScores;

  const bool uk = language == models::InterviewLanguage::Uk;

  if (uk) {
    if (finalScore >= 85)
      report.summary =
          "Кандидат продемонстрував впевнене володіння матеріалом, чітко "
          "пояснював архітектурні рішення та їхні компроміси. Рівень "
          "відповідей відповідає очікуванням для обраної позиції.";
    else if (finalScore >= 70)
      report.summary =
          "Кандидат загалом правильно розуміє ключові концепції, але частина "
          "відповідей потребує більшої конкретики та прикладів із практики.";
    else
      report.summary =
          "Базове розуміння теми присутнє, проте відповіді часто надто "
          "короткі. Рекомендовано систематизувати знання та більше "
          "практикуватися на реальних сценаріях.";

    report.strengths = {
        "Розуміння базових концепцій напрямку",
        "Здатність пояснювати власні рішення",
        "Готовність міркувати про компроміси"};
    report.weaknesses = {
        "Бракує конкретних прикладів із практики",
        "Варто глибше дослідити профілювання та оптимізацію",
        "Звернути увагу на крайові сценарії та обробку помилок"};
    report.recommendations = {
        {"System Design Primer",
         "https://github.com/donnemartin/system-design-primer"},
        {"Use The Index, Luke", "https://use-the-index-luke.com"}};
  } else {
    if (finalScore >= 85)
      report.summary =
          "The candidate demonstrated confident command of the material, "
          "clearly explaining architectural decisions and their trade-offs. "
          "The answer quality matches expectations for the selected role.";
    else if (finalScore >= 70)
      report.summary =
          "The candidate understands the core concepts well overall, but "
          "several answers need more specificity and practical examples.";
    else
      report.summary =
          "A basic understanding is present, but answers are often too brief. "
          "Structuring the knowledge and practising real-world scenarios is "
          "recommended.";

    report.strengths = {
        "Grasp of the direction's core concepts",
        "Ability to explain own decisions",
        "Willingness to reason about trade-offs"};
    report.weaknesses = {
        "Lacks concrete practical examples",
        "Deeper study of profiling and optimisation is advised",
        "Pay attention to edge cases and error handling"};
    report.recommendations = {
        {"System Design Primer",
         "https://github.com/donnemartin/system-design-primer"},
        {"Use The Index, Luke", "https://use-the-index-luke.com"}};
  }

  (void)direction;
  return report;
}

void MockAiEngine::initializeQuestionBank() {
  using D = models::InterviewDirection;
  using L = models::InterviewLevel;

  addPool(
      D::Frontend, L::Trainee,
      {"Вітаю на співбесіді Frontend (Trainee)! Розкажіть, що вас зацікавило у "
       "веброзробці.",
       "Чим HTML-елемент відрізняється від CSS-властивості? Наведіть простий "
       "приклад.",
       "Що таке змінна в JavaScript і чим let відрізняється від const?",
       "Як ви користуєтесь інструментами розробника в браузері, коли щось не "
       "працює?"},
      {"Welcome to the Frontend (Trainee) interview! What got you interested in "
       "web development?",
       "How does an HTML element differ from a CSS property? Give a simple "
       "example.",
       "What is a variable in JavaScript, and how does let differ from const?",
       "How do you use browser developer tools when something breaks?"});

  addPool(
      D::Frontend, L::Junior,
      {"Вітаю на співбесіді Frontend (Junior)! Розкажіть коротко про себе та "
       "ваш досвід із JavaScript/TypeScript.",
       "Як працює Virtual DOM у React і в чому його перевага над прямими "
       "маніпуляціями з DOM?",
       "Поясніть різницю між useMemo та useCallback. Коли їхнє використання є "
       "надлишковим?",
       "Які способи оптимізації рендерингу великих списків у браузері ви "
       "знаєте?"},
      {"Welcome to the Frontend (Junior) interview! Briefly introduce yourself "
       "and your JavaScript/TypeScript experience.",
       "How does the Virtual DOM work in React, and what benefits does it "
       "provide over direct DOM mutations?",
       "Explain the distinction between useMemo and useCallback. When does "
       "memoization add unnecessary overhead?",
       "What techniques do you apply to optimise rendering of large data lists "
       "in modern browsers?"});

  addPool(
      D::Frontend, L::Middle,
      {"Вітаю на співбесіді Frontend (Middle)! Опишіть ваш найскладніший "
       "проєкт і вашу роль у ньому.",
       "Як ви проєктуєте стан великого React-застосунку? Порівняйте "
       "Context, Redux Toolkit і серверний стан.",
       "Що таке Core Web Vitals і як ви діагностуєте проблеми з LCP та CLS?",
       "Як організувати code splitting і кешування так, щоб не зламати "
       "інвалідацію даних?",
       "Які компроміси між SSR, SSG та CSR ви обираєте для продуктових "
       "сторінок?"},
      {"Welcome to the Frontend (Middle) interview! Describe your most complex "
       "project and your role in it.",
       "How do you design state management for a large React application? "
       "Compare Context, Redux Toolkit, and server state.",
       "What are Core Web Vitals, and how do you diagnose LCP and CLS issues?",
       "How would you organise code splitting and caching without breaking "
       "data invalidation?",
       "What trade-offs between SSR, SSG, and CSR do you choose for product "
       "pages?"});

  addPool(
      D::Backend, L::Trainee,
      {"Вітаю на співбесіді Backend (Trainee)! Що вас приваблює в серверній "
       "розробці?",
       "Що таке HTTP-запит і з яких частин він складається?",
       "Чим відрізняється GET від POST?",
       "Що таке база даних і навіщо потрібні таблиці?"},
      {"Welcome to the Backend (Trainee) interview! What attracts you to "
       "server-side development?",
       "What is an HTTP request, and what parts does it consist of?",
       "How does GET differ from POST?",
       "What is a database, and why do we need tables?"});

  addPool(
      D::Backend, L::Junior,
      {"Вітаю на співбесіді Backend (Junior)! Розкажіть про свій досвід із "
       "C#/.NET або іншим серверним стеком.",
       "Що таке REST і які принципи роблять API ресурсно-орієнтованим?",
       "Як працює індексація в реляційній базі даних і коли індекс шкодить?",
       "Поясніть різницю між автентифікацією та авторизацією. Як працює JWT?"},
      {"Welcome to the Backend (Junior) interview! Tell me about your "
       "experience with C#/.NET or another server stack.",
       "What is REST, and which principles make an API resource-oriented?",
       "How does indexing work in a relational database, and when does an "
       "index hurt?",
       "Explain the difference between authentication and authorisation. How "
       "does JWT work?"});

  addPool(
      D::Backend, L::Middle,
      {"Вітаю на співбесіді Backend (Middle)! Опишіть архітектуру сервісу, "
       "який ви проєктували або супроводжували.",
       "Як ви забезпечуєте ідемпотентність операцій під час повторних "
       "запитів і ретраїв?",
       "Порівняйте optimistic та pessimistic locking. Наведіть сценарій для "
       "кожного.",
       "Як організувати обробку фонових задач і гарантувати доставку подій?",
       "Що ви робите для спостережуваності: метрики, логи, трейсинг?"},
      {"Welcome to the Backend (Middle) interview! Describe the architecture of "
       "a service you designed or maintained.",
       "How do you make operations idempotent when clients retry requests?",
       "Compare optimistic and pessimistic locking. Give a scenario for each.",
       "How do you organise background job processing and guarantee event "
       "delivery?",
       "What do you do for observability: metrics, logs, and tracing?"});

  addPool(
      D::DevOps, L::Trainee,
      {"Вітаю на співбесіді DevOps (Trainee)! Що для вас означає DevOps?",
       "Що таке CI/CD і навіщо автоматизувати збірку?",
       "Чим відрізняється контейнер від віртуальної машини?",
       "Як ви перевірите, що сервіс запустився після деплою?"},
      {"Welcome to the DevOps (Trainee) interview! What does DevOps mean to "
       "you?",
       "What is CI/CD, and why automate builds?",
       "How does a container differ from a virtual machine?",
       "How would you verify that a service started after deployment?"});

  addPool(
      D::DevOps, L::Junior,
      {"Вітаю на співбесіді DevOps (Junior)! Розкажіть, які інструменти ви "
       "використовували для автоматизації.",
       "Що описує Dockerfile і як зменшити розмір образу?",
       "Як працює Kubernetes Service і чим він відрізняється від Ingress?",
       "Які метрики ви збираєте для моніторингу здоров'я сервісу?"},
      {"Welcome to the DevOps (Junior) interview! Which automation tools have "
       "you used?",
       "What does a Dockerfile describe, and how do you reduce image size?",
       "How does a Kubernetes Service work, and how does it differ from an "
       "Ingress?",
       "Which metrics do you collect to monitor service health?"});

  addPool(
      D::DevOps, L::Middle,
      {"Вітаю на співбесіді DevOps (Middle)! Опишіть інфраструктуру, яку ви "
       "розгортали, і ключові рішення.",
       "Як ви проєктуєте zero-downtime deployment для stateful-сервісу?",
       "Порівняйте Terraform та Helm для керування інфраструктурою.",
       "Як організувати секрети в CI/CD і уникнути їх витоку?",
       "Що таке SLO та error budget і як вони впливають на релізи?"},
      {"Welcome to the DevOps (Middle) interview! Describe the infrastructure "
       "you deployed and your key decisions.",
       "How do you design a zero-downtime deployment for a stateful service?",
       "Compare Terraform and Helm for infrastructure management.",
       "How do you manage secrets in CI/CD and prevent leaks?",
       "What are SLOs and error budgets, and how do they affect releases?"});

  addPool(
      D::Qa, L::Trainee,
      {"Вітаю на співбесіді QA (Trainee)! Що вас зацікавило в тестуванні?",
       "Чим відрізняється ручне тестування від автоматизованого?",
       "Що таке тест-кейс і які його обов'язкові частини?",
       "Як ви опишете баг, щоб розробник швидко його відтворив?"},
      {"Welcome to the QA (Trainee) interview! What attracted you to testing?",
       "How does manual testing differ from automated testing?",
       "What is a test case, and what are its essential parts?",
       "How do you report a bug so a developer can reproduce it quickly?"});

  addPool(
      D::Qa, L::Junior,
      {"Вітаю на співбесіді QA (Junior)! Розкажіть про свій досвід "
       "тестування вебзастосунків.",
       "Як ви обираєте, що автоматизувати в першу чергу?",
       "Що таке регресійне тестування і як його зробити ефективним?",
       "Як перевірити API без UI? Наведіть приклад перевірок."},
      {"Welcome to the QA (Junior) interview! Tell me about your experience "
       "testing web applications.",
       "How do you decide what to automate first?",
       "What is regression testing, and how do you make it efficient?",
       "How do you test an API without a UI? Give examples of checks."});

  addPool(
      D::Qa, L::Middle,
      {"Вітаю на співбесіді QA (Middle)! Опишіть стратегію тестування "
       "продукту, який ви супроводжували.",
       "Як ви будуєте піраміду тестів і які метрики якості відстежуєте?",
       "Що таке flaky-тест і як ви системно зменшуєте нестабільність?",
       "Як організувати тестові дані та ізоляцію тестів у CI?",
       "Як ви тестуєте продуктивність і де знаходите вузькі місця?"},
      {"Welcome to the QA (Middle) interview! Describe the testing strategy for "
       "a product you supported.",
       "How do you build a test pyramid, and which quality metrics do you "
       "track?",
       "What is a flaky test, and how do you systematically reduce flakiness?",
       "How do you organise test data and test isolation in CI?",
       "How do you approach performance testing and find bottlenecks?"});

  addPool(
      D::Pm, L::Trainee,
      {"Вітаю на співбесіді Product Manager (Trainee)! Що для вас означає "
       "роль продакт-менеджера?",
       "Чим відрізняється фіча від гіпотези?",
       "Як ви зрозумієте, що користувачам потрібна нова функція?",
       "Що таке MVP і навіщо його робити?"},
      {"Welcome to the Product Manager (Trainee) interview! What does the "
       "product manager role mean to you?",
       "How does a feature differ from a hypothesis?",
       "How would you learn that users need a new capability?",
       "What is an MVP, and why build one?"});

  addPool(
      D::Pm, L::Junior,
      {"Вітаю на співбесіді Product Manager (Junior)! Розкажіть про продукт, "
       "яким ви користуєтесь щодня, і як би ви його покращили.",
       "Як ви пріоритезуєте беклог? Порівняйте RICE та MoSCoW.",
       "Які метрики ви обрали б для SaaS-продукту на ранній стадії?",
       "Як ви проводите інтерв'ю з користувачами, щоб не отримати упереджені "
       "відповіді?"},
      {"Welcome to the Product Manager (Junior) interview! Describe a product "
       "you use daily and how you would improve it.",
       "How do you prioritise a backlog? Compare RICE and MoSCoW.",
       "Which metrics would you choose for an early-stage SaaS product?",
       "How do you run user interviews without collecting biased answers?"});

  addPool(
      D::Pm, L::Middle,
      {"Вітаю на співбесіді Product Manager (Middle)! Опишіть продукт, за "
       "який ви відповідали, і його ключові метрики.",
       "Як ви ухвалюєте рішення за браком даних? Наведіть приклад.",
       "Як узгодити roadmap між інженерією, дизайном і бізнесом?",
       "Що таке product-market fit і як ви його вимірюєте?",
       "Як ви вирішуєте, коли зупинити розвиток функції та видалити її?"},
      {"Welcome to the Product Manager (Middle) interview! Describe a product "
       "you owned and its key metrics.",
       "How do you make decisions with insufficient data? Give an example.",
       "How do you align a roadmap across engineering, design, and business?",
       "What is product-market fit, and how do you measure it?",
       "How do you decide when to stop developing a feature and remove it?"});
}

} // namespace devdialogue::services
