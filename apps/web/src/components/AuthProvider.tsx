'use client';

import React, { useEffect, useCallback } from 'react';
import { Session } from '@supabase/supabase-js';
import { supabase, isSupabaseConfigured } from '@/lib/supabase';
import { useAuthStore, UserSession } from '@/store/authStore';
import { api } from '@/lib/api-client';
import { UserRole } from '@dev-dialogue/api-contracts';

export const AuthProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const { setUser, setToken } = useAuthStore();

  const syncSession = useCallback(
    async (session: Session) => {
      const rawRole = session.user.user_metadata?.role as string | undefined;
      const role: UserRole = rawRole === 'admin' ? 'admin' : 'user';

      const userSession: UserSession = {
        id: session.user.id,
        email: session.user.email || '',
        fullName:
          session.user.user_metadata?.full_name ||
          session.user.user_metadata?.name ||
          session.user.email?.split('@')[0] ||
          'User',
        role,
      };

      setUser(userSession);
      setToken(session.access_token);

      // Call /api/auth/me to trigger backend user self-healing creation & fetch server-assigned role
      try {
        const me = await api.auth.getMe();
        if (me && me.role) {
          setUser({
            ...userSession,
            fullName: me.fullName || userSession.fullName,
            role: me.role,
          });
        }
      } catch (err) {
        // Backend may be offline or in mock mode; client session remains valid
        console.warn('API sync profile warning:', err);
      }
    },
    [setUser, setToken]
  );

  useEffect(() => {
    if (!isSupabaseConfigured()) {
      return;
    }

    // 1. Initial session check
    supabase.auth.getSession().then(({ data: { session } }) => {
      if (session?.user) {
        syncSession(session);
      }
    });

    // 2. Real-time auth state changes (OAuth redirect, sign in, sign out)
    const {
      data: { subscription },
    } = supabase.auth.onAuthStateChange(async (event, session) => {
      if (session?.user) {
        syncSession(session);
      } else if (event === 'SIGNED_OUT') {
        setUser(null);
        setToken(null);
      }
    });

    return () => {
      subscription.unsubscribe();
    };
  }, [setUser, setToken, syncSession]);

  return <>{children}</>;
};
