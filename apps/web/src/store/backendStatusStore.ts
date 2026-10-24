import { create } from 'zustand';

export type BackendConnectionStatus = 'checking' | 'online' | 'offline';

interface BackendStatusState {
  status: BackendConnectionStatus;
  isDismissed: boolean;
  lastChecked: number | null;
  errorMessage: string | null;
  checkHealth: () => Promise<boolean>;
  setOnline: () => void;
  setOffline: (reason?: string) => void;
  dismissBanner: () => void;
  reopenBanner: () => void;
}

const API_BASE_URL = process.env.NEXT_PUBLIC_API_URL || 'http://localhost:5000';

export const useBackendStatusStore = create<BackendStatusState>()((set, get) => ({
  status: 'checking',
  isDismissed: false,
  lastChecked: null,
  errorMessage: null,

  setOnline: () => {
    const prev = get().status;
    if (prev !== 'online') {
      set({
        status: 'online',
        lastChecked: Date.now(),
        errorMessage: null,
      });
    }
  },

  setOffline: (reason?: string) => {
    const prev = get().status;
    if (prev !== 'offline') {
      set({
        status: 'offline',
        lastChecked: Date.now(),
        errorMessage: reason || null,
      });
    }
  },

  dismissBanner: () => set({ isDismissed: true }),
  reopenBanner: () => set({ isDismissed: false }),

  checkHealth: async () => {
    set({ status: 'checking' });
    const cleanUrl = API_BASE_URL.replace(/\/$/, '');
    const pingEndpoint = `${cleanUrl}/health`;

    try {
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 3500);

      const res = await fetch(pingEndpoint, {
        method: 'GET',
        cache: 'no-store',
        signal: controller.signal,
      });
      clearTimeout(timeoutId);

      if (res.ok || (res.status >= 200 && res.status < 500)) {
        set({
          status: 'online',
          lastChecked: Date.now(),
          errorMessage: null,
        });
        return true;
      } else {
        set({
          status: 'offline',
          lastChecked: Date.now(),
          errorMessage: `HTTP ${res.status}`,
        });
        return false;
      }
    } catch (err: unknown) {
      const msg = err instanceof Error ? err.message : 'Connection failed';
      set({
        status: 'offline',
        lastChecked: Date.now(),
        errorMessage: msg,
      });
      return false;
    }
  },
}));
