#include "viewmodels/NavigationViewModel.hpp"

namespace devdialogue::viewmodels {

NavigationViewModel::NavigationViewModel(QObject *parent)
    : QObject(parent), m_currentViewIndex(ViewIndex::Landing) {}

int NavigationViewModel::currentViewIndex() const { return m_currentViewIndex; }

void NavigationViewModel::setCurrentViewIndex(int index) {
  if (m_currentViewIndex != index) {
    m_currentViewIndex = index;
    emit currentViewIndexChanged(m_currentViewIndex);
  }
}

QString NavigationViewModel::activeInterviewId() const {
  return m_activeInterviewId;
}

void NavigationViewModel::navigateToLanding() {
  setCurrentViewIndex(ViewIndex::Landing);
}

void NavigationViewModel::navigateToDashboard() {
  setCurrentViewIndex(ViewIndex::Dashboard);
}

void NavigationViewModel::navigateToSetup() {
  setCurrentViewIndex(ViewIndex::Setup);
}

void NavigationViewModel::navigateToChat(const QString &interviewId) {
  m_activeInterviewId = interviewId;
  emit activeInterviewIdChanged(m_activeInterviewId);
  setCurrentViewIndex(ViewIndex::ChatRoom);
}

void NavigationViewModel::navigateToReport(const QString &interviewId) {
  m_activeInterviewId = interviewId;
  emit activeInterviewIdChanged(m_activeInterviewId);
  setCurrentViewIndex(ViewIndex::Report);
}

void NavigationViewModel::navigateToAdmin() {
  setCurrentViewIndex(ViewIndex::Admin);
}

} // namespace devdialogue::viewmodels
