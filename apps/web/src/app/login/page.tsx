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
import { useTranslation } from '@/lib/useTranslation';

function LoginForm() {
  const router = useRouter();
  const searchParams = useSearchParams();
  const initialMode = searchParams.get('mode') === 'signup' ? 'signup' : 'signin';
  const redirectPath = searchParams.get('redirect') || '/dashboard';

  const { user, mockLogin } = useAuthStore();
  const { t, lang } = useTranslation();
  const [mode, setMode] = useState<'signin' | 'signup'>(initialMode);
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [fullName, setFullName] = useState('');
  const [showPassword, setShowPassword] = useState(false);
  const [showConfirmPassword, setShowConfirmPassword] = useState(false);
  const [loading, setLoading] = useState(false);
  const [oauthLoading, setOauthLoading] = useState<'google' | 'github' | 'linkedin_oidc' | null>(null);
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

  // OAuth Handler for Google, GitHub, LinkedIn
  const handleOAuth = async (provider: 'google' | 'github' | 'linkedin_oidc') => {
    setError(null);
    setSuccessMessage(null);
    setOauthLoading(provider);

    if (!isSupabaseConfigured()) {
      setError(
        lang === 'uk'
          ? 'Supabase не налаштовано локально (відсутній NEXT_PUBLIC_SUPABASE_URL). Використайте швидкий Демо-доступ нижче або вкажіть змінні у .env.'
          : 'Supabase is not configured locally (missing NEXT_PUBLIC_SUPABASE_URL). Please use the 1-click Demo mode below or provide .env variables.'
      );
      setOauthLoading(null);
      return;
    }

    try {
      const redirectUrl =
        typeof window !== 'undefined'
          ? `${window.location.origin}/login?redirect=${encodeURIComponent(redirectPath)}`
          : undefined;

      const { error: oauthError } = await supabase.auth.signInWithOAuth({
        provider,
        options: {
          redirectTo: redirectUrl,
          ...(provider === 'google'
            ? {
                queryParams: {
                  access_type: 'offline',
                  prompt: 'consent',
                },
              }
            : {}),
        },
      });

      if (oauthError) {
        throw oauthError;
      }
    } catch (err: unknown) {
      const providerName =
        provider === 'github' ? 'GitHub' : provider === 'google' ? 'Google' : 'LinkedIn';
      console.error(`${providerName} Auth Error:`, err);
      const msg =
        err instanceof Error
          ? err.message
          : lang === 'uk'
            ? `Не вдалося ініціалізувати вхід через ${providerName}`
            : `Failed to initialize ${providerName} authentication`;
      setError(msg);
      setOauthLoading(null);
    }
  };

  // Email / Password Handler
  const handleEmailAuth = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);
    setSuccessMessage(null);
    setLoading(true);

    if (!email || !password) {
      setError(
        lang === 'uk'
          ? 'Будь ласка, заповніть усі обовʼязкові поля.'
          : 'Please fill in all required fields.'
      );
      setLoading(false);
      return;
    }

    if (mode === 'signup') {
      const trimmedNick = fullName.trim();
      if (!trimmedNick) {
        setError(
          lang === 'uk'
            ? 'Будь ласка, вкажіть ваш нікнейм для реєстрації.'
            : 'Please provide your nickname to sign up.'
        );
        setLoading(false);
        return;
      }

      // Nickname validation: minimum 3 characters, only English letters/numbers/underscore
      const isEnglishOnly = /^[a-zA-Z0-9_]+$/.test(trimmedNick);
      if (trimmedNick.length < 3 || !isEnglishOnly) {
        setError(t.auth.nicknameValidationError);
        setLoading(false);
        return;
      }

      // Password validation: between 8 and 72 characters, must include both letters and numbers
      const hasLetter = /[a-zA-Z]/.test(password);
      const hasDigit = /[0-9]/.test(password);
      if (password.length < 8 || password.length > 72 || !hasLetter || !hasDigit) {
        setError(t.auth.passwordComplexityError);
        setLoading(false);
        return;
      }

      if (password !== confirmPassword) {
        setError(t.auth.passwordsMismatch);
        setLoading(false);
        return;
      }
    }

    if (!isSupabaseConfigured()) {
      // Offline fallback: simulate successful login
      setTimeout(() => {
        setLoading(false);
        mockLogin('user');
        router.push(redirectPath);
      }, 700);
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
              role: 'candidate',
            },
          },
        });

        if (signUpError) throw signUpError;

        if (data?.session) {
          router.push(redirectPath);
        } else {
          setSuccessMessage(
            lang === 'uk'
              ? 'Акаунт успішно створено! Перевірте електронну пошту для підтвердження або увійдіть.'
              : 'Account created successfully! Please check your email to confirm your account or sign in.'
          );
          setMode('signin');
        }
      } else {
        const { error: signInError } = await supabase.auth.signInWithPassword({
          email,
          password,
        });

        if (signInError) throw signInError;
        router.push(redirectPath);
      }
    } catch (err: unknown) {
      console.warn('Auth action notice:', err);
      let msg =
        err instanceof Error
          ? err.message
          : lang === 'uk'
            ? 'Помилка авторизації. Спробуйте ще раз або використайте Демо-режим.'
            : 'Authentication error. Please try again or use Demo mode.';

      if (
        msg.includes('Failed to fetch') ||
        msg.includes('network') ||
        msg.includes('timeout') ||
        msg.includes('ERR_CONNECTION_TIMED_OUT')
      ) {
        msg =
          lang === 'uk'
            ? 'Сервер авторизації Supabase не відповів вчасно (Timeout). Зачекайте кілька секунд і повторіть спробу (хмарний проєкт міг бути в сплячому режимі).'
            : 'Supabase authentication timed out. Please wait a few seconds and try again (the cloud instance may be waking up).';
      }

      setError(msg);
    } finally {
      setLoading(false);
    }
  };

  const handleQuickDemo = (role: 'admin' | 'user') => {
    mockLogin(role);
    setSuccessMessage(
      lang === 'uk'
        ? `Ви успішно авторизовані як ${role === 'admin' ? 'Адміністратор' : 'Кандидат'} (Демо)! Перенаправлення...`
        : `Signed in as ${role === 'admin' ? 'Administrator' : 'Candidate'} (Demo mode)! Redirecting...`
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
              className="inline-flex items-center gap-2.5 mb-4 group transition-transform hover:scale-105"
            >
              <div className="relative flex h-8 w-8 items-center justify-center rounded-xl border border-blue-500/60 bg-blue-500/10 shadow-sm shadow-blue-500/25 transition-all duration-200 group-hover:border-blue-400 group-hover:bg-blue-500/20">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  strokeWidth="2.2"
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  className="h-4 w-4 text-blue-400 group-hover:text-blue-300 transition-colors"
                >
                  <path d="M4 17l6-6-6-6" />
                  <path d="M12 19h8" />
                </svg>
              </div>
              <span className="font-bold tracking-tight text-zinc-100 text-lg">Dev Dialogue</span>
            </Link>

            <h1 className="text-2xl font-bold tracking-tight text-zinc-100">
              {mode === 'signin' ? t.auth.titleSignIn : t.auth.titleSignUp}
            </h1>
            <p className="mt-1.5 text-xs text-zinc-400">
              {mode === 'signin' ? t.auth.subtitleSignIn : t.auth.subtitleSignUp}
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
              {lang === 'uk' ? 'Вхід (Sign In)' : 'Sign In'}
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
              {lang === 'uk' ? 'Реєстрація (Sign Up)' : 'Sign Up'}
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

          {/* Social OAuth Buttons */}
          <div className="space-y-2.5">
            {/* GitHub OAuth Button (Featured for developers) */}
            <button
              type="button"
              onClick={() => handleOAuth('github')}
              disabled={!!oauthLoading || loading}
              className="btn-tactile w-full flex items-center justify-center gap-2.5 rounded-xl border border-white/[0.12] bg-zinc-900/90 hover:bg-zinc-800 hover:border-white/[0.22] px-4 py-2.5 text-xs sm:text-sm font-medium text-zinc-100 shadow-sm disabled:opacity-50 transition-all"
            >
              {oauthLoading === 'github' ? (
                <div className="h-4 w-4 animate-spin rounded-full border-2 border-zinc-400 border-t-transparent" />
              ) : (
                <svg className="h-4 w-4 shrink-0 fill-current text-white" viewBox="0 0 24 24">
                  <path
                    fillRule="evenodd"
                    clipRule="evenodd"
                    d="M12 2C6.477 2 2 6.484 2 12.017c0 4.425 2.865 8.18 6.839 9.504.5.092.682-.217.682-.483 0-.237-.008-.868-.013-1.703-2.782.605-3.369-1.343-3.369-1.343-.454-1.158-1.11-1.466-1.11-1.466-.908-.62.069-.608.069-.608 1.003.07 1.53 1.032 1.53 1.032.892 1.53 2.341 1.088 2.91.832.092-.647.35-1.088.636-1.338-2.22-.253-4.555-1.113-4.555-4.951 0-1.093.39-1.988 1.029-2.688-.103-.253-.446-1.272.098-2.65 0 0 .84-.27 2.75 1.026A9.564 9.564 0 0112 6.844c.85.004 1.705.115 2.504.337 1.909-1.296 2.747-1.027 2.747-1.027.546 1.379.202 2.398.1 2.651.64.7 1.028 1.595 1.028 2.688 0 3.848-2.339 4.695-4.566 4.943.359.309.678.92.678 1.855 0 1.338-.012 2.419-.012 2.747 0 .268.18.58.688.482A10.019 10.019 0 0022 12.017C22 6.484 17.522 2 12 2z"
                  />
                </svg>
              )}
              <span>{lang === 'uk' ? 'Продовжити з GitHub' : 'Continue with GitHub'}</span>
            </button>

            {/* Google & LinkedIn Secondary Row */}
            <div className="grid grid-cols-2 gap-2.5">
              {/* Google */}
              <button
                type="button"
                onClick={() => handleOAuth('google')}
                disabled={!!oauthLoading || loading}
                className="btn-tactile flex items-center justify-center gap-2 rounded-xl border border-white/[0.12] bg-zinc-900/90 hover:bg-zinc-800 hover:border-white/[0.22] px-3 py-2.5 text-xs sm:text-sm font-medium text-zinc-100 shadow-sm disabled:opacity-50 transition-all"
              >
                {oauthLoading === 'google' ? (
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
                <span>Google</span>
              </button>

              {/* LinkedIn */}
              <button
                type="button"
                onClick={() => handleOAuth('linkedin_oidc')}
                disabled={!!oauthLoading || loading}
                className="btn-tactile flex items-center justify-center gap-2 rounded-xl border border-white/[0.12] bg-zinc-900/90 hover:bg-zinc-800 hover:border-white/[0.22] px-3 py-2.5 text-xs sm:text-sm font-medium text-zinc-100 shadow-sm disabled:opacity-50 transition-all"
              >
                {oauthLoading === 'linkedin_oidc' ? (
                  <div className="h-4 w-4 animate-spin rounded-full border-2 border-zinc-400 border-t-transparent" />
                ) : (
                  <svg className="h-4 w-4 shrink-0 fill-[#0A66C2]" viewBox="0 0 24 24">
                    <path d="M19 3a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h14m-.5 15.5v-5.3a3.26 3.26 0 0 0-3.26-3.26c-.85 0-1.84.52-2.28 1.3v-1.11h-2.79v8.37h2.79v-4.93c0-.77.62-1.4 1.39-1.4a1.4 1.4 0 0 1 1.4 1.4v4.93h2.75M6.88 8.56a1.68 1.68 0 0 0 1.68-1.68c0-.93-.75-1.69-1.68-1.69a1.69 1.69 0 0 0-1.69 1.69c0 .93.76 1.68 1.69 1.68m1.39 9.94v-8.37H5.5v8.37h2.77z" />
                  </svg>
                )}
                <span>LinkedIn</span>
              </button>
            </div>
          </div>

          {/* Divider */}
          <div className="relative my-6 text-center">
            <div className="absolute inset-0 flex items-center">
              <div className="w-full border-t border-white/[0.08]" />
            </div>
            <span className="relative bg-zinc-950 px-3 text-[11px] font-mono uppercase tracking-wider text-zinc-500">
              {t.auth.orEmail}
            </span>
          </div>

          {/* Email / Password Form */}
          <form onSubmit={handleEmailAuth} className="space-y-4">
            {mode === 'signup' && (
              <div>
                <label className="block text-xs font-medium text-zinc-300 mb-1.5">
                  {t.auth.fullNameLabel}
                  <span className="text-red-500 ml-1 font-bold">*</span>
                </label>
                <div className="relative">
                  <UserIcon className="pointer-events-none absolute left-3.5 top-1/2 -translate-y-1/2 h-4 w-4 text-zinc-500" />
                  <input
                    type="text"
                    required
                    value={fullName}
                    onChange={(e) => setFullName(e.target.value)}
                    placeholder={t.auth.fullNamePlaceholder}
                    className="w-full rounded-xl border border-white/[0.08] bg-zinc-900/60 py-2.5 pl-10 pr-3.5 text-xs sm:text-sm text-zinc-100 placeholder-zinc-500 transition-colors focus:border-blue-500/60 focus:bg-zinc-900 focus:outline-none focus:ring-1 focus:ring-blue-500/40"
                  />
                </div>
                <p className="mt-1 text-[11px] text-zinc-500">
                  {t.auth.nicknameHint}
                </p>
              </div>
            )}

            <div>
              <label className="block text-xs font-medium text-zinc-300 mb-1.5">
                {t.auth.emailLabel}
                {mode === 'signup' && <span className="text-red-500 ml-1 font-bold">*</span>}
              </label>
              <div className="relative">
                <Mail className="pointer-events-none absolute left-3.5 top-1/2 -translate-y-1/2 h-4 w-4 text-zinc-500" />
                <input
                  type="email"
                  required
                  value={email}
                  onChange={(e) => setEmail(e.target.value)}
                  placeholder={t.auth.emailPlaceholder}
                  className="w-full rounded-xl border border-white/[0.08] bg-zinc-900/60 py-2.5 pl-10 pr-3.5 text-xs sm:text-sm text-zinc-100 placeholder-zinc-500 transition-colors focus:border-blue-500/60 focus:bg-zinc-900 focus:outline-none focus:ring-1 focus:ring-blue-500/40"
                />
              </div>
            </div>

            {/* Password Field */}
            <div>
              <div className="flex items-center justify-between mb-1.5">
                <label className="block text-xs font-medium text-zinc-300">
                  {t.auth.passwordLabel}
                  {mode === 'signup' && <span className="text-red-500 ml-1 font-bold">*</span>}
                </label>
              </div>
              <div className="relative">
                <Lock className="pointer-events-none absolute left-3.5 top-1/2 -translate-y-1/2 h-4 w-4 text-zinc-500" />
                <input
                  type={showPassword ? 'text' : 'password'}
                  required
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                  placeholder={t.auth.passwordPlaceholder}
                  className="w-full rounded-xl border border-white/[0.08] bg-zinc-900/60 py-2.5 pl-10 pr-11 text-xs sm:text-sm text-zinc-100 placeholder-zinc-500 transition-colors focus:border-blue-500/60 focus:bg-zinc-900 focus:outline-none focus:ring-1 focus:ring-blue-500/40"
                />
                <button
                  type="button"
                  onClick={() => setShowPassword((prev) => !prev)}
                  title={showPassword ? t.auth.hidePassword : t.auth.showPassword}
                  aria-label={showPassword ? t.auth.hidePassword : t.auth.showPassword}
                  className="absolute right-2.5 top-1/2 -translate-y-1/2 p-1.5 rounded-lg text-zinc-400 hover:text-zinc-200 hover:bg-white/[0.06] transition-all"
                >
                  {showPassword ? <EyeOff className="h-4 w-4" /> : <Eye className="h-4 w-4" />}
                </button>
              </div>
              {mode === 'signup' && (
                <p className="mt-1 text-[11px] text-zinc-500">
                  {t.auth.passwordHint}
                </p>
              )}
            </div>

            {/* Confirm Password Field (Sign Up mode only) */}
            {mode === 'signup' && (
              <div className="animate-fade-in-up">
                <div className="flex items-center justify-between mb-1.5">
                  <label className="block text-xs font-medium text-zinc-300">
                    {t.auth.confirmPasswordLabel}
                    <span className="text-red-500 ml-1 font-bold">*</span>
                  </label>
                </div>
                <div className="relative">
                  <Lock className="pointer-events-none absolute left-3.5 top-1/2 -translate-y-1/2 h-4 w-4 text-zinc-500" />
                  <input
                    type={showConfirmPassword ? 'text' : 'password'}
                    required
                    value={confirmPassword}
                    onChange={(e) => setConfirmPassword(e.target.value)}
                    placeholder={t.auth.confirmPasswordPlaceholder}
                    className="w-full rounded-xl border border-white/[0.08] bg-zinc-900/60 py-2.5 pl-10 pr-11 text-xs sm:text-sm text-zinc-100 placeholder-zinc-500 transition-colors focus:border-blue-500/60 focus:bg-zinc-900 focus:outline-none focus:ring-1 focus:ring-blue-500/40"
                  />
                  <button
                    type="button"
                    onClick={() => setShowConfirmPassword((prev) => !prev)}
                    title={showConfirmPassword ? t.auth.hidePassword : t.auth.showPassword}
                    aria-label={showConfirmPassword ? t.auth.hidePassword : t.auth.showPassword}
                    className="absolute right-2.5 top-1/2 -translate-y-1/2 p-1.5 rounded-lg text-zinc-400 hover:text-zinc-200 hover:bg-white/[0.06] transition-all"
                  >
                    {showConfirmPassword ? <EyeOff className="h-4 w-4" /> : <Eye className="h-4 w-4" />}
                  </button>
                </div>
              </div>
            )}

            <button
              type="submit"
              disabled={loading || !!oauthLoading}
              className="w-full mt-2 inline-flex items-center justify-center gap-2 rounded-xl bg-blue-600 hover:bg-blue-500 py-3 px-4 text-xs sm:text-sm font-semibold text-white shadow-lg shadow-blue-600/25 hover:shadow-blue-600/40 border border-blue-400/30 transition-all duration-200 disabled:opacity-50 active:scale-[0.99]"
            >
              {loading ? (
                <div className="h-4 w-4 animate-spin rounded-full border-2 border-white border-t-transparent" />
              ) : (
                <>
                  <span>{mode === 'signin' ? t.auth.submitSignIn : t.auth.submitSignUp}</span>
                  <ArrowRight className="h-4 w-4" />
                </>
              )}
            </button>

            {mode === 'signin' ? (
              <p className="mt-4 text-center text-xs text-zinc-400">
                {t.auth.noAccount}{' '}
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
                  {t.auth.signUpLink}
                </Link>
              </p>
            ) : (
              <p className="mt-4 text-center text-xs text-zinc-400">
                {t.auth.haveAccount}{' '}
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
                  {t.auth.signInLink}
                </Link>
              </p>
            )}
          </form>

          {/* Quick Demo Access Bar */}
          <div className="mt-8 pt-6 border-t border-white/[0.08]">
            <div className="flex items-center justify-between mb-3">
              <span className="text-[11px] font-mono text-zinc-400 uppercase tracking-wider flex items-center gap-1.5">
                <Sparkles className="h-3 w-3 text-blue-400" />
                {lang === 'uk' ? 'Швидкий тест (Демо-режим)' : 'Quick Test (Demo Mode)'}
              </span>
              <span className="text-[10px] text-zinc-400 font-mono">1 click</span>
            </div>
            <div className="grid grid-cols-2 gap-2">
              <button
                type="button"
                onClick={() => handleQuickDemo('admin')}
                className="btn-tactile flex items-center justify-center gap-1.5 rounded-lg border border-blue-500/30 bg-blue-950/30 px-3 py-2 text-xs font-medium text-blue-300 hover:bg-blue-900/40 hover:text-blue-200 transition-colors"
              >
                <ShieldCheck className="h-3.5 w-3.5 text-blue-400" />
                <span>{lang === 'uk' ? 'Як Адмін' : 'As Admin'}</span>
              </button>
              <button
                type="button"
                onClick={() => handleQuickDemo('user')}
                className="btn-tactile flex items-center justify-center gap-1.5 rounded-lg border border-white/[0.08] bg-zinc-900/70 px-3 py-2 text-xs font-medium text-zinc-300 hover:bg-zinc-800 hover:text-white transition-colors"
              >
                <UserCheck className="h-3.5 w-3.5 text-emerald-400" />
                <span>{lang === 'uk' ? 'Як Кандидат' : 'As Candidate'}</span>
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
          Loading...
        </div>
      }
    >
      <LoginForm />
    </Suspense>
  );
}
