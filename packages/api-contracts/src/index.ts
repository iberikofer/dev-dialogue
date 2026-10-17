export type UserRole = 'user' | 'admin';

export interface UserProfile {
  id: string;
  email: string;
  fullName: string | null;
  avatarUrl: string | null;
  role: UserRole;
  dailyInterviewsCount: number;
  lastInterviewDate: string | null;
  createdAt: string;
}

export interface AuthMeResponse {
  id: string;
  email: string;
  fullName: string | null;
  avatarUrl: string | null;
  role: UserRole;
}

export type InterviewDirection = 'frontend' | 'backend' | 'devops' | 'pm' | 'qa';
export type InterviewLevel = 'trainee' | 'junior' | 'middle';
export type InterviewLanguage = 'uk' | 'en';
export type InterviewStage = 'hard_skills' | 'soft_skills' | 'english';
export type InterviewStatus = 'active' | 'completed' | 'abandoned';

export interface InterviewMessage {
  role: 'system' | 'assistant' | 'user';
  content: string;
  timestamp: string;
}

export interface QuestionScoreItem {
  question: string;
  score: number;
  comment: string;
}

export interface RecommendationItem {
  topic: string;
  resource: string;
}

export interface InterviewFeedbackReport {
  summary: string;
  score: number;
  strengths: string[];
  weaknesses: string[];
  recommendations: RecommendationItem[];
  questionScores: QuestionScoreItem[];
}

export interface InterviewSummary {
  id: string;
  userId: string;
  direction: InterviewDirection;
  level: InterviewLevel;
  language: InterviewLanguage;
  stage: InterviewStage;
  status: InterviewStatus;
  score: number | null;
  questionCount: number;
  createdAt: string;
  updatedAt: string;
}

export interface InterviewDetail extends InterviewSummary {
  transcript: InterviewMessage[];
  feedback: InterviewFeedbackReport | null;
}

export interface CreateInterviewRequest {
  direction: InterviewDirection;
  level: InterviewLevel;
  language: InterviewLanguage;
}

export interface CreateInterviewResponse {
  interviewId: string;
  firstQuestion: string;
}

export interface SendMessageRequest {
  content: string;
}

export interface SendMessageResponse {
  message: string;
  questionCount: number;
  isCompleted?: boolean;
}

export interface CompleteInterviewResponse {
  score: number;
  feedback: InterviewFeedbackReport;
}

export interface QuestionBankItem {
  id: string;
  direction: InterviewDirection;
  stage: InterviewStage;
  level: InterviewLevel;
  topic: string;
  questionText: string;
  isActive: boolean;
  createdBy: string | null;
  createdAt: string;
}

export interface CreateQuestionRequest {
  direction: InterviewDirection;
  stage: InterviewStage;
  level: InterviewLevel;
  topic: string;
  questionText: string;
}

export interface UpdateQuestionRequest {
  direction?: InterviewDirection;
  stage?: InterviewStage;
  level?: InterviewLevel;
  topic?: string;
  questionText?: string;
  isActive?: boolean;
}

export interface UserDashboardStats {
  totalInterviews: number;
  averageScore: number;
  dailyInterviewsCount: number;
  dailyLimit: number;
  remainingToday: number;
}
