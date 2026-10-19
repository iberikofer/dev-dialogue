'use client';

import React, { useState, useEffect, Suspense } from 'react';
import Link from 'next/link';
import { useRouter, useSearchParams } from 'next/navigation';
import {
  Mail,
  Lock,
  User as UserIcon,
  Eye,
  EyeOff,
  Sparkles,
  ArrowRight,
  AlertCircle,
  CheckCircle2,
  ShieldCheck,
  UserCheck,
} from 'lucide-react';
import { supabase, isSupabaseConfigured } from '@/lib/supabase';
import { useAuthStore } from '@/store/authStore';

function LoginForm() {
  const router = useRouter();
  const searchParams = useSearchParams();
  const initialMode = searchParams.get('mode') === 'signup' ? 'signup' : 'signin';
  const redirectPath = searchParams.get('redirect') || '/dashboard';

  const { user, mockLogin } = useAuthStore();
  const [mode, setMode] = useState<'signin' | 'signup'>(initialMode);
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [fullName, setFullName] = useState('');
  const [showPassword, setShowPassword] = useState(false);
  const [loading, setLoading] = useState(false);
  const [googleLoading, setGoogleLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [successMessage, setSuccessMessage] = useState<string | null>(null);

  // Sync mode with URL search parameters
  useEffect(() => {
    const urlMode = searchParams.get('mode');
    if (urlMode === 'signup') {
      setMode('signup');
    } else if (urlMode === 'signin') {
      setMode('signin');
    }
  }, [searchParams]);

  // If already logged in with a real session, redirect to dashboard
  useEffect(() => {
    if (
      user &&
      user.id !== '00000000-0000-0000-0000-000000000001' &&
      user.id !== '00000000-0000-0000-0000-000000000002'
    ) {
      router.push(redirectPath);
    }
  }, [user, redirectPath, router]);

  // Google OAuth Handler
  const handleGoogleAuth = async () => {
    setError(null);
    setSuccessMessage(null);
    setGoogleLoading(true);

    if (!isSupabaseConfigured()) {
      setError(
        'Supabase не налаштовано локально (відсутній NEXT_PUBLIC_SUPABASE_URL). Використайте швидкий Демо-доступ нижче або вкажіть змінні у .env.'
      );
      setGoogleLoading(false);
      return;
    }

    try {
      const redirectUrl =
        typeof window !== 'undefined'
          ? `${window.location.origin}/login?redirect=${encodeURIComponent(redirectPath)}`
          : undefined;

      const { error: oauthError } = await supabase.auth.signInWithOAuth({
        provider: 'google',
        options: {
          redirectTo: redirectUrl,
          queryParams: {
            access_type: 'offline',
            prompt: 'consent',
          },
        },
      });

      if (oauthError) {
        throw oauthError;
      }
    } catch (err: unknown) {
      console.error('Google Auth Error:', err);
      const msg = err instanceof Error ? err.message : 'Не вдалося ініціалізувати Google вхід';
      setError(msg);
      setGoogleLoading(false);
    }
  };

  // Email / Password Handler
  const handleEmailAuth = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);
    setSuccessMessage(null);
    setLoading(true);

    if (!email || !password) {
      setError('Будь ласка, заповніть усі обовʼязкові поля.');
      setLoading(false);
      return;
    }

    if (mode === 'signup' && !fullName) {
      setError('Будь ласка, вкажіть ваше імʼя для реєстрації.');
      setLoading(false);
      return;
    }

    if (!isSupabaseConfigured()) {
      // Local fallback simulation if Supabase is unconfigured
      mockLogin('user');
      setSuccessMessage('Успішний локальний вхід у демонстраційному режимі!');
      setTimeout(() => router.push(redirectPath), 600);
      return;
    }

    try {
      if (mode === 'signup') {
        const { data, error: signUpError } = await supabase.auth.signUp({
          email,
          password,
          options: {
            data: {
              full_name: fullName,
              role: 'user',
            },
          },
        });

        if (signUpError) throw signUpError;

        if (data.session) {
          setSuccessMessage('Акаунт успішно створено! Перенаправляємо...');
          setTimeout(() => router.push(redirectPath), 800);
        } else {
          setSuccessMessage(
            'Реєстрація успішна! Якщо на вашому Supabase увімкнено підтвердження email — перевірте поштову скриньку.'
          );
        }
      } else {
        const { error: signInError } = await supabase.auth.signInWithPassword({
          email,
          password,
        });

        if (signInError) throw signInError;

        setSuccessMessage('Успішний вхід! Перенаправляємо...');
        setTimeout(() => router.push(redirectPath), 600);
      }
    } catch (err: unknown) {
      console.error('Auth Error:', err);
      const msg =
        err instanceof Error
          ? err.message
          : 'Помилка авторизації. Перевірте правильність введених даних.';
      setError(msg);
    } finally {
      setLoading(false);
    }
  };

  // Quick Demo Login Handler
  const handleQuickDemo = (role: 'admin' | 'user') => {
    mockLogin(role);
    setSuccessMessage(
      `Активовано сесію ${role === 'admin' ? 'Адміністратора (Yaroslav)' : 'Кандидата'}!`
    );
    setTimeout(() => router.push(redirectPath), 400);
  };

  return (
    <div className="relative min-h-[calc(100vh-4rem)] flex items-center justify-center px-4 py-12 overflow-hidden bg-[#09090b]">
      {/* Background Ambience */}
      <div
        className="pointer-events-none absolute inset-0 z-0 opacity-[0.035]"
        style={{
          backgroundImage: 'radial-gradient(rgba(255, 255, 255, 0.45) 1px, transparent 1px)',
          backgroundSize: '24px 24px',
        }}
      />
      <div className="pointer-events-none absolute top-1/4 left-1/2 -translate-x-1/2 -translate-y-1/2 w-[600px] h-[350px] bg-blue-600/15 blur-[160px] rounded-full" />

      <div className="relative z-10 w-full max-w-md animate-fade-in-up">
        {/* Card Container */}
        <div className="rounded-2xl border border-white/[0.1] bg-zinc-950/85 p-7 sm:p-9 shadow-2xl shadow-black/80 backdrop-blur-2xl transition-all">
          {/* Header */}
          <div className="text-center mb-8">
            <Link
              href="/"
              className="inline-flex items-center gap-2 mb-4 group transition-transform hover:scale-105"
            >
              <div className="relative flex h-[28px] w-[28px] items-center justify-center rounded-lg bg-gradient-to-tr from-blue-600 to-indigo-600 p-[1px] shadow-sm">
                <div className="flex h-full w-full items-center justify-center rounded-[7px] bg-zinc-950">
                  <svg
                    viewBox="0 0 24 24"
                    fill="none"
                    stroke="currentColor"
                    strokeWidth="2.2"
                    strokeLinecap="round"
                    strokeLinejoin="round"
                    className="h-3.5 w-3.5 text-blue-400"
                  >
                    <path d="M4 17l6-6-6-6" />
                    <path d="M12 19h8" />
                  </svg>
                </div>
              </div>
              <span className="font-bold tracking-tight text-zinc-100 text-lg">Dev Dialogue</span>
            </Link>

            <h1 className="text-2xl font-bold tracking-tight text-zinc-100">
              {mode === 'signin' ? 'Вхід у систему' : 'Створення акаунту'}
            </h1>
            <p className="mt-1.5 text-xs text-zinc-400">
              {mode === 'signin'
                ? 'Авторизуйтесь для збереження результатів симуляцій'
                : 'Розпочніть тренування технічних інтервʼю з AI'}
            </p>
          </div>

          {/* Mode Switcher Tabs */}
          <div className="flex rounded-xl bg-zinc-900/90 p-1 border border-white/[0.08] mb-6">
            <button
              type="button"
              onClick={() => {
                setMode('signin');
                setError(null);
                setSuccessMessage(null);
              }}
              className={`btn-tactile flex-1 rounded-lg py-2 text-xs font-semibold ${
                mode === 'signin'
                  ? 'bg-blue-600 text-white shadow-sm'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              Вхід (Sign In)
            </button>
            <button
              type="button"
              onClick={() => {
                setMode('signup');
                setError(null);
                setSuccessMessage(null);
              }}
              className={`btn-tactile flex-1 rounded-lg py-2 text-xs font-semibold ${
                mode === 'signup'
                  ? 'bg-blue-600 text-white shadow-sm'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              Реєстрація (Sign Up)
            </button>
          </div>

          {/* Alert Messages */}
          {error && (
            <div className="mb-5 flex items-start gap-2.5 rounded-xl border border-red-500/30 bg-red-950/30 p-3.5 text-xs text-red-200">
              <AlertCircle className="h-4 w-4 shrink-0 text-red-400 mt-0.5" />
              <div className="leading-relaxed">{error}</div>
            </div>
          )}

          {successMessage && (
            <div className="mb-5 flex items-start gap-2.5 rounded-xl border border-emerald-500/30 bg-emerald-950/30 p-3.5 text-xs text-emerald-200">
              <CheckCircle2 className="h-4 w-4 shrink-0 text-emerald-400 mt-0.5" />
              <div className="leading-relaxed">{successMessage}</div>
            </div>
          )}

          {/* Google OAuth Button */}
          <button
            type="button"
            onClick={handleGoogleAuth}
            disabled={googleLoading || loading}
            className="btn-tactile w-full flex items-center justify-center gap-3 rounded-xl border border-white/[0.12] bg-zinc-900/90 hover:bg-zinc-800/90 hover:border-white/[0.22] px-4 py-3 text-xs sm:text-sm font-medium text-zinc-100 shadow-sm disabled:opacity-50"
          >
            {googleLoading ? (
              <div className="h-4 w-4 animate-spin rounded-full border-2 border-zinc-400 border-t-transparent" />
            ) : (
              <svg className="h-4 w-4 shrink-0" viewBox="0 0 24 24">
                <path
                  fill="#4285F4"
                  d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.09z"
                />
                <path
                  fill="#34A853"
                  d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z"
                />
                <path
                  fill="#FBBC05"
                  d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.06H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.94l2.85-2.22.81-.63z"
                />
                <path
                  fill="#EA4335"
                  d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.06l3.66 2.84c.87-2.6 3.3-4.52 6.16-4.52z"
                />
              </svg>
            )}
            <span>
              {mode === 'signin' ? 'Продовжити через Google' : 'Зареєструватися через Google'}
            </span>
          </button>

          {/* Divider */}
          <div className="relative my-6 text-center">
            <div className="absolute inset-0 flex items-center">
              <div className="w-full border-t border-white/[0.08]" />
            </div>
            <span className="relative bg-zinc-950 px-3 text-[11px] font-mono uppercase tracking-wider text-zinc-500">
              або через email
            </span>
          </div>

          {/* Email / Password Form */}
          <form onSubmit={handleEmailAuth} className="space-y-4">
            {mode === 'signup' && (
              <div>
                <label className="block text-xs font-medium text-zinc-300 mb-1.5">
                  Ім&apos;я / Псевдонім
                </label>
                <div className="relative">
                  <UserIcon className="pointer-events-none absolute left-3.5 top-1/2 -translate-y-1/2 h-4 w-4 text-zinc-500" />
                  <input
                    type="text"
                    required
                    value={fullName}
                    onChange={(e) => setFullName(e.target.value)}
                    placeholder="Name / Nickname"
                    className="w-full rounded-xl border border-white/[0.08] bg-zinc-900/60 py-2.5 pl-10 pr-3.5 text-xs sm:text-sm text-zinc-100 placeholder-zinc-500 transition-colors focus:border-blue-500/60 focus:bg-zinc-900 focus:outline-none focus:ring-1 focus:ring-blue-500/40"
                  />
                </div>
              </div>
            )}

            <div>
              <label className="block text-xs font-medium text-zinc-300 mb-1.5">Email адреса</label>
              <div className="relative">
                <Mail className="pointer-events-none absolute left-3.5 top-1/2 -translate-y-1/2 h-4 w-4 text-zinc-500" />
                <input
                  type="email"
                  required
                  value={email}
                  onChange={(e) => setEmail(e.target.value)}
                  placeholder="engineer@domain.com"
                  className="w-full rounded-xl border border-white/[0.08] bg-zinc-900/60 py-2.5 pl-10 pr-3.5 text-xs sm:text-sm text-zinc-100 placeholder-zinc-500 transition-colors focus:border-blue-500/60 focus:bg-zinc-900 focus:outline-none focus:ring-1 focus:ring-blue-500/40"
                />
              </div>
            </div>

            <div>
              <div className="flex items-center justify-between mb-1.5">
                <label className="block text-xs font-medium text-zinc-300">Пароль</label>
                {mode === 'signin' && (
                  <span className="text-[11px] text-blue-400 hover:text-blue-300 cursor-pointer">
                    Забули пароль?
                  </span>
                )}
              </div>
              <div className="relative">
                <Lock className="pointer-events-none absolute left-3.5 top-1/2 -translate-y-1/2 h-4 w-4 text-zinc-500" />
                <input
                  type={showPassword ? 'text' : 'password'}
                  required
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                  placeholder="••••••••••••"
                  className="w-full rounded-xl border border-white/[0.08] bg-zinc-900/60 py-2.5 pl-10 pr-10 text-xs sm:text-sm text-zinc-100 placeholder-zinc-500 transition-colors focus:border-blue-500/60 focus:bg-zinc-900 focus:outline-none focus:ring-1 focus:ring-blue-500/40"
                />
                <button
                  type="button"
                  onClick={() => setShowPassword(!showPassword)}
                  className="absolute right-3 top-1/2 -translate-y-1/2 text-zinc-500 hover:text-zinc-300 transition-colors"
                >
                  {showPassword ? <EyeOff className="h-4 w-4" /> : <Eye className="h-4 w-4" />}
                </button>
              </div>
            </div>

            <button
              type="submit"
              disabled={loading || googleLoading}
              className="w-full mt-2 inline-flex items-center justify-center gap-2 rounded-xl bg-blue-600 hover:bg-blue-500 py-3 px-4 text-xs sm:text-sm font-semibold text-white shadow-lg shadow-blue-600/25 hover:shadow-blue-600/40 border border-blue-400/30 transition-all duration-200 disabled:opacity-50 active:scale-[0.99]"
            >
              {loading ? (
                <div className="h-4 w-4 animate-spin rounded-full border-2 border-white border-t-transparent" />
              ) : (
                <>
                  <span>{mode === 'signin' ? 'Sign In' : 'Sign Up'}</span>
                  <ArrowRight className="h-4 w-4" />
                </>
              )}
            </button>

            {mode === 'signin' ? (
              <p className="mt-4 text-center text-xs text-zinc-400">
                no account?{' '}
                <Link
                  href="/login?mode=signup"
                  onClick={(e) => {
                    e.preventDefault();
                    setMode('signup');
                    setError(null);
                    setSuccessMessage(null);
                    router.push('/login?mode=signup');
                  }}
                  className="font-semibold text-blue-400 hover:text-blue-300 underline underline-offset-2 transition-colors"
                >
                  Sign Up!
                </Link>
              </p>
            ) : (
              <p className="mt-4 text-center text-xs text-zinc-400">
                Already have an account?{' '}
                <Link
                  href="/login?mode=signin"
                  onClick={(e) => {
                    e.preventDefault();
                    setMode('signin');
                    setError(null);
                    setSuccessMessage(null);
                    router.push('/login?mode=signin');
                  }}
                  className="font-semibold text-blue-400 hover:text-blue-300 underline underline-offset-2 transition-colors"
                >
                  Sign In
                </Link>
              </p>
            )}
          </form>

          {/* Quick Demo Access Bar (Guaranteed Offline / Testing reliability) */}
          <div className="mt-8 pt-6 border-t border-white/[0.08]">
            <div className="flex items-center justify-between mb-3">
              <span className="text-[11px] font-mono text-zinc-400 uppercase tracking-wider flex items-center gap-1.5">
                <Sparkles className="h-3 w-3 text-blue-400" />
                Швидкий тест (Демо-режим)
              </span>
              <span className="text-[10px] text-zinc-400">1 клік</span>
            </div>
            <div className="grid grid-cols-2 gap-2">
              <button
                type="button"
                onClick={() => handleQuickDemo('admin')}
                className="flex items-center justify-center gap-1.5 rounded-lg border border-blue-500/30 bg-blue-950/30 px-3 py-2 text-xs font-medium text-blue-300 hover:bg-blue-900/40 hover:text-blue-200 transition-colors"
              >
                <ShieldCheck className="h-3.5 w-3.5 text-blue-400" />
                <span>Як Адмін</span>
              </button>
              <button
                type="button"
                onClick={() => handleQuickDemo('user')}
                className="flex items-center justify-center gap-1.5 rounded-lg border border-white/[0.08] bg-zinc-900/70 px-3 py-2 text-xs font-medium text-zinc-300 hover:bg-zinc-800 hover:text-white transition-colors"
              >
                <UserCheck className="h-3.5 w-3.5 text-emerald-400" />
                <span>Як Кандидат</span>
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default function LoginPage() {
  return (
    <Suspense
      fallback={
        <div className="min-h-[calc(100vh-4rem)] flex items-center justify-center bg-[#09090b] text-zinc-400 text-sm">
          Завантаження форми авторизації...
        </div>
      }
    >
      <LoginForm />
    </Suspense>
  );
}
