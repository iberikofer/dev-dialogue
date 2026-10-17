import {
  AuthMeResponse,
  CreateInterviewRequest,
  CreateInterviewResponse,
  InterviewDetail,
  InterviewSummary,
  SendMessageResponse,
  CompleteInterviewResponse,
  UserDashboardStats,
  QuestionBankItem,
  CreateQuestionRequest,
  UpdateQuestionRequest,
} from '@dev-dialogue/api-contracts';
import { useAuthStore } from '@/store/authStore';

const API_BASE_URL = process.env.NEXT_PUBLIC_API_URL || 'http://localhost:5000';

class ApiClientError extends Error {
  status: number;
  data: unknown;

  constructor(status: number, message: string, data?: unknown) {
    super(message);
    this.status = status;
    this.data = data;
  }
}

async function request<T>(endpoint: string, options: RequestInit = {}): Promise<T> {
  const token = useAuthStore.getState().token;

  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
    ...(options.headers as Record<string, string>),
  };

  if (token) {
    headers['Authorization'] = `Bearer ${token}`;
  }

  const url = `${API_BASE_URL.replace(/\/$/, '')}/${endpoint.replace(/^\//, '')}`;

  try {
    const res = await fetch(url, {
      ...options,
      headers,
    });

    if (!res.ok) {
      let errorData: unknown;
      try {
        errorData = await res.json();
      } catch {
        errorData = await res.text();
      }
      throw new ApiClientError(res.status, `API request failed: ${res.statusText}`, errorData);
    }

    if (res.status === 204) {
      return {} as T;
    }

    return await res.json();
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : String(err);
    console.warn(`[ApiClient] Failed to connect to backend at ${url}:`, message);
    throw err;
  }
}

export const api = {
  auth: {
    getMe: () => request<AuthMeResponse>('/api/auth/me'),
  },

  interviews: {
    create: (dto: CreateInterviewRequest) =>
      request<CreateInterviewResponse>('/api/interviews', {
        method: 'POST',
        body: JSON.stringify(dto),
      }),

    list: (page = 1, pageSize = 20) =>
      request<InterviewSummary[]>(`/api/interviews?page=${page}&pageSize=${pageSize}`),

    getById: (id: string) => request<InterviewDetail>(`/api/interviews/${id}`),

    sendMessage: (id: string, content: string) =>
      request<SendMessageResponse>(`/api/interviews/${id}/message`, {
        method: 'POST',
        body: JSON.stringify({ content }),
      }),

    submitAnswer: (id: string, payload: { answer: string }) =>
      request<SendMessageResponse>(`/api/interviews/${id}/message`, {
        method: 'POST',
        body: JSON.stringify({ content: payload.answer }),
      }),

    complete: (id: string) =>
      request<CompleteInterviewResponse>(`/api/interviews/${id}/complete`, {
        method: 'POST',
      }),

    getStats: () => request<UserDashboardStats>('/api/interviews/stats'),
  },

  admin: {
    getQuestions: (params?: {
      direction?: string;
      level?: string;
      stage?: string;
      isActive?: boolean;
    }) => {
      const query = new URLSearchParams();
      if (params?.direction) query.append('direction', params.direction);
      if (params?.level) query.append('level', params.level);
      if (params?.stage) query.append('stage', params.stage);
      if (params?.isActive !== undefined) query.append('isActive', String(params.isActive));
      return request<QuestionBankItem[]>(`/api/admin/questions?${query.toString()}`);
    },

    createQuestion: (dto: CreateQuestionRequest) =>
      request<QuestionBankItem>('/api/admin/questions', {
        method: 'POST',
        body: JSON.stringify(dto),
      }),

    updateQuestion: (id: string, dto: UpdateQuestionRequest) =>
      request<QuestionBankItem>(`/api/admin/questions/${id}`, {
        method: 'PUT',
        body: JSON.stringify(dto),
      }),

    deleteQuestion: (id: string) =>
      request<void>(`/api/admin/questions/${id}`, {
        method: 'DELETE',
      }),
  },
};
