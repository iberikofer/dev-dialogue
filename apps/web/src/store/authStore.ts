import { create } from 'zustand';
import { UserRole } from '@dev-dialogue/api-contracts';

export interface UserSession {
  id: string;
  email: string;
  fullName: string | null;
  role: UserRole;
}

interface AuthState {
  user: UserSession | null;
  token: string | null;
  isAdminViewActive: boolean;
  setUser: (user: UserSession | null) => void;
  setToken: (token: string | null) => void;
  toggleAdminView: () => void;
  setAdminView: (active: boolean) => void;
  logout: () => void;
  mockLogin: (role?: UserRole) => void;
}

const STORAGE_KEY = 'dev_dialogue_auth';

export const useAuthStore = create<AuthState>((set) => {
  // Initialize from localStorage if present in browser
  let initialUser: UserSession | null = null;
  let initialToken: string | null = null;
  let initialAdminView = true;

  if (typeof window !== 'undefined') {
    try {
      const stored = localStorage.getItem(STORAGE_KEY);
      if (stored) {
        const parsed = JSON.parse(stored);
        initialUser = parsed.user;
        initialToken = parsed.token;
        initialAdminView = parsed.isAdminViewActive ?? true;
      }
    } catch {
      // Ignore parse errors
    }
  }

  const persist = (user: UserSession | null, token: string | null, isAdminViewActive: boolean) => {
    if (typeof window !== 'undefined') {
      localStorage.setItem(STORAGE_KEY, JSON.stringify({ user, token, isAdminViewActive }));
    }
  };

  return {
    user: initialUser ?? {
      id: '00000000-0000-0000-0000-000000000001',
      email: 'demo@devdialogue.io',
      fullName: 'Yaroslav (Admin)',
      role: 'admin',
    },
    token: initialToken ?? 'mock-dev-jwt-token',
    isAdminViewActive: initialAdminView,

    setUser: (user) =>
      set((state) => {
        persist(user, state.token, state.isAdminViewActive);
        return { user };
      }),

    setToken: (token) =>
      set((state) => {
        persist(state.user, token, state.isAdminViewActive);
        return { token };
      }),

    toggleAdminView: () =>
      set((state) => {
        const next = !state.isAdminViewActive;
        persist(state.user, state.token, next);
        return { isAdminViewActive: next };
      }),

    setAdminView: (active) =>
      set((state) => {
        persist(state.user, state.token, active);
        return { isAdminViewActive: active };
      }),

    logout: () => {
      persist(null, null, true);
      set({ user: null, token: null, isAdminViewActive: true });
    },

    mockLogin: (role = 'admin') => {
      const mockUser: UserSession = {
        id:
          role === 'admin'
            ? '00000000-0000-0000-0000-000000000001'
            : '00000000-0000-0000-0000-000000000002',
        email: role === 'admin' ? 'admin@devdialogue.io' : 'student@devdialogue.io',
        fullName: role === 'admin' ? 'Yaroslav (Admin)' : 'Candidate User',
        role,
      };
      persist(mockUser, 'mock-dev-jwt-token', true);
      set({ user: mockUser, token: 'mock-dev-jwt-token', isAdminViewActive: true });
    },
  };
});
