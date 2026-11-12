#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <optional>

namespace devdialogue::models {
class QuestionListModel;
}

namespace devdialogue::services {
class AdminService;
}

namespace devdialogue::viewmodels {

/// Backs the admin question-bank screen: filtering, pagination and CRUD.
class AdminViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(models::QuestionListModel *questionsModel READ questionsModel
                 CONSTANT)
  Q_PROPERTY(QString direction READ direction WRITE setDirection NOTIFY
                 filtersChanged)
  Q_PROPERTY(QString level READ level WRITE setLevel NOTIFY filtersChanged)
  Q_PROPERTY(QString stage READ stage WRITE setStage NOTIFY filtersChanged)
  Q_PROPERTY(QString activeFilter READ activeFilter WRITE setActiveFilter
                 NOTIFY filtersChanged)
  Q_PROPERTY(int page READ page NOTIFY questionsChanged)
  Q_PROPERTY(int pageSize READ pageSize NOTIFY questionsChanged)
  Q_PROPERTY(int totalCount READ totalCount NOTIFY questionsChanged)
  Q_PROPERTY(int totalPages READ totalPages NOTIFY questionsChanged)
  Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
  Q_PROPERTY(QString successMessage READ successMessage NOTIFY
                 successMessageChanged)

public:
  explicit AdminViewModel(std::shared_ptr<services::AdminService> adminService,
                          QObject *parent = nullptr);

  models::QuestionListModel *questionsModel() const {
    return m_questionsModel;
  }

  QString direction() const { return m_direction; }
  void setDirection(const QString &direction);

  QString level() const { return m_level; }
  void setLevel(const QString &level);

  QString stage() const { return m_stage; }
  void setStage(const QString &stage);

  QString activeFilter() const { return m_activeFilter; }
  void setActiveFilter(const QString &filter);

  int page() const { return m_page; }
  int pageSize() const { return m_pageSize; }
  int totalCount() const { return m_totalCount; }
  int totalPages() const { return m_totalPages; }
  bool isBusy() const { return m_isBusy; }
  QString errorMessage() const { return m_errorMessage; }
  QString successMessage() const { return m_successMessage; }

  Q_INVOKABLE void refresh();
  Q_INVOKABLE void applyFilters(const QString &direction, const QString &level,
                                const QString &stage,
                                const QString &activeFilter);
  Q_INVOKABLE void clearFilters();
  Q_INVOKABLE void nextPage();
  Q_INVOKABLE void previousPage();

  Q_INVOKABLE void createQuestion(const QString &direction,
                                  const QString &stage, const QString &level,
                                  const QString &topic,
                                  const QString &questionText);
  Q_INVOKABLE void setQuestionActive(const QString &id, bool active);
  Q_INVOKABLE void deleteQuestion(const QString &id);
  Q_INVOKABLE void clearError();
  Q_INVOKABLE void clearSuccess();

signals:
  void questionsChanged();
  void filtersChanged();
  void busyChanged();
  void errorMessageChanged();
  void successMessageChanged();

private:
  static bool parseActiveFilter(const QString &filter,
                                std::optional<bool> &result);

  void setBusy(bool busy);
  void setError(const QString &error);
  void setSuccess(const QString &message);

  std::shared_ptr<services::AdminService> m_adminService;
  models::QuestionListModel *m_questionsModel;

  QString m_direction;
  QString m_level;
  QString m_stage;
  QString m_activeFilter{QStringLiteral("all")};
  int m_page{1};
  int m_pageSize{20};
  int m_totalCount{0};
  int m_totalPages{0};
  bool m_isBusy{false};
  QString m_errorMessage;
  QString m_successMessage;
};

} // namespace devdialogue::viewmodels
