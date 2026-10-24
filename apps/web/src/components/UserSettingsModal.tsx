'use client';

import React, { useState, useEffect } from 'react';
import { createPortal } from 'react-dom';
import { useAuthStore } from '@/store/authStore';
import { useTranslation } from '@/lib/useTranslation';
import { supabase, isSupabaseConfigured } from '@/lib/supabase';
import {
  X,
  User,
  Mail,
  Shield,
  Palette,
  Volume2,
  Cpu,
  Globe,
  Check,
  Sparkles,
  Lock,
  Eye,
  EyeOff,
  Link2,
  Unlink,
  Info,
  AlertCircle,
} from 'lucide-react';

interface UserSettingsModalProps {
  isOpen: boolean;
  onClose: () => void;
  defaultTab?: 'profile' | 'site';
}

export const UserSettingsModal: React.FC<UserSettingsModalProps> = ({
  isOpen,
  onClose,
  defaultTab = 'profile',
}) => {
  const { user, updateProfile, linkProvider, unlinkProvider } = useAuthStore();
  const { t, lang, toggleLanguage } = useTranslation();

  const [mounted, setMounted] = useState(false);
  const [activeTab, setActiveTab] = useState<'profile' | 'site'>(defaultTab);
  const [fullName, setFullName] = useState(user?.fullName || '');
  const [email, setEmail] = useState(user?.email || '');
  const [avatarUrl, setAvatarUrl] = useState(user?.avatarUrl || '');
  const [password, setPassword] = useState('');
  const [showPassword, setShowPassword] = useState(false);

  // Account Linking state
  const [linkingProvider, setLinkingProvider] = useState<string | null>(null);
  const [linkFeedback, setLinkFeedback] = useState<{
    type: 'success' | 'error';
    message: string;
  } | null>(null);

  // Site settings state (stored in localStorage)
  const [theme, setTheme] = useState('oled');
  const [soundEnabled, setSoundEnabled] = useState(true);
  const [aiPersona, setAiPersona] = useState('balanced');
  const [savedSuccess, setSavedSuccess] = useState(false);

  useEffect(() => {
    setActiveTab(defaultTab);
  }, [defaultTab, isOpen]);

  useEffect(() => {
    if (user) {
      setFullName(user.fullName || '');
      setEmail(user.email || '');
      setAvatarUrl(user.avatarUrl || '');
    }
  }, [user]);

  // Sync identities from live Supabase session when modal opens
  useEffect(() => {
    if (!isOpen || !isSupabaseConfigured() || !user?.id || user.id.startsWith('00000000-')) {
      return;
    }
    supabase.auth.getUserIdentities().then(({ data, error }) => {
      if (!error && data?.identities) {
        const providers = Array.from(new Set(data.identities.map((i) => i.provider)));
        if (providers.length > 0) {
          updateProfile({ linkedProviders: providers });
        }
      }
    });
  }, [isOpen, user?.id, updateProfile]);

  useEffect(() => {
    setMounted(true);
    if (typeof window !== 'undefined') {
      const savedTheme = localStorage.getItem('dd_theme') || 'oled';
      const savedSound = localStorage.getItem('dd_sound') !== 'false';
      const savedAi = localStorage.getItem('dd_ai_persona') || 'balanced';
      setTheme(savedTheme);
      setSoundEnabled(savedSound);
      setAiPersona(savedAi);
    }
  }, []);

  // Handle ESC key to close
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape' && isOpen) {
        onClose();
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [isOpen, onClose]);

  if (!isOpen || !mounted) return null;

  const isProviderLinked = (providerKey: string) => {
    if (!user?.linkedProviders) return false;
    if (providerKey === 'linkedin_oidc' || providerKey === 'linkedin') {
      return (
        user.linkedProviders.includes('linkedin_oidc') ||
        user.linkedProviders.includes('linkedin')
      );
    }
    return user.linkedProviders.includes(providerKey);
  };

  const handleToggleLink = async (provider: 'github' | 'google' | 'linkedin_oidc') => {
    const isLinked = isProviderLinked(provider);
    const providerLabel =
      provider === 'github' ? 'GitHub' : provider === 'google' ? 'Google' : 'LinkedIn';

    setLinkingProvider(provider);
    setLinkFeedback(null);

    try {
      if (isLinked) {
        // Unlink flow
        if (isSupabaseConfigured() && user?.id && !user.id.startsWith('00000000-')) {
          const { data: identitiesData, error: idErr } = await supabase.auth.getUserIdentities();
          if (idErr) throw idErr;

          const target = identitiesData?.identities?.find((id) =>
            provider === 'linkedin_oidc'
              ? id.provider === 'linkedin_oidc' || id.provider === 'linkedin'
              : id.provider === provider
          );

          if (target) {
            const { error: unlinkErr } = await supabase.auth.unlinkIdentity(target);
            if (unlinkErr) throw unlinkErr;
          }
        }

        unlinkProvider(provider);
        setLinkFeedback({
          type: 'success',
          message: `${providerLabel}: ${t.userSettings.unlinkSuccess}`,
        });
      } else {
        // Link flow
        if (isSupabaseConfigured() && user?.id && !user.id.startsWith('00000000-')) {
          const redirectUrl =
            typeof window !== 'undefined'
              ? `${window.location.origin}/dashboard`
              : undefined;

          const { data, error } = await supabase.auth.linkIdentity({
            provider,
            options: {
              redirectTo: redirectUrl,
              ...(provider === 'google'
                ? { queryParams: { access_type: 'offline', prompt: 'consent' } }
                : {}),
            },
          });

          if (error) throw error;
          if (data?.url) {
            window.location.href = data.url;
            return;
          }
        } else {
          // Demo / Mock mode
          linkProvider(provider);
          setLinkFeedback({
            type: 'success',
            message: `${providerLabel}: ${t.userSettings.linkSuccess}`,
          });
        }
      }
    } catch (err: unknown) {
      console.error('Account linking error:', err);
      const msg =
        err instanceof Error
          ? err.message
          : lang === 'uk'
          ? 'Помилка оновлення привʼязки сервісу.'
          : 'Failed to update linked service.';
      setLinkFeedback({
        type: 'error',
        message: msg,
      });
    } finally {
      setLinkingProvider(null);
    }
  };

  const handleSaveProfile = (e: React.FormEvent) => {
    e.preventDefault();
    const trimmedNick = fullName.trim();
    if (trimmedNick && (trimmedNick.length < 3 || !/^[a-zA-Z0-9_]+$/.test(trimmedNick))) {
      setLinkFeedback({
        type: 'error',
        message:
          lang === 'uk'
            ? 'Нікнейм повинен містити щонайменше 3 символи і складатись виключно з літер англійського алфавіту (та цифр).'
            : 'Nickname must be at least 3 characters and contain only English letters and numbers.',
      });
      setTimeout(() => setLinkFeedback(null), 3500);
      return;
    }

    updateProfile({
      fullName: trimmedNick || null,
      email: email.trim(),
      avatarUrl: avatarUrl.trim() || null,
    });
    setSavedSuccess(true);
    setTimeout(() => setSavedSuccess(false), 2500);
  };

  const handleSiteSettingChange = (key: string, value: string | boolean) => {
    if (key === 'theme') {
      setTheme(value as string);
      localStorage.setItem('dd_theme', value as string);
    } else if (key === 'sound') {
      setSoundEnabled(value as boolean);
      localStorage.setItem('dd_sound', String(value));
    } else if (key === 'aiPersona') {
      setAiPersona(value as string);
      localStorage.setItem('dd_ai_persona', value as string);
    }
    setSavedSuccess(true);
    setTimeout(() => setSavedSuccess(false), 2000);
  };

  const socialProviders = [
    {
      id: 'github' as const,
      name: 'GitHub',
      desc:
        lang === 'uk'
          ? 'Для розробників та репозиторіїв коду'
          : 'For developers and code repositories',
      icon: (
        <svg className="h-4 w-4 shrink-0 fill-current text-white" viewBox="0 0 24 24">
          <path
            fillRule="evenodd"
            clipRule="evenodd"
            d="M12 2C6.477 2 2 6.484 2 12.017c0 4.425 2.865 8.18 6.839 9.504.5.092.682-.217.682-.483 0-.237-.008-.868-.013-1.703-2.782.605-3.369-1.343-3.369-1.343-.454-1.158-1.11-1.466-1.11-1.466-.908-.62.069-.608.069-.608 1.003.07 1.53 1.032 1.53 1.032.892 1.53 2.341 1.088 2.91.832.092-.647.35-1.088.636-1.338-2.22-.253-4.555-1.113-4.555-4.951 0-1.093.39-1.988 1.029-2.688-.103-.253-.446-1.272.098-2.65 0 0 .84-.27 2.75 1.026A9.564 9.564 0 0112 6.844c.85.004 1.705.115 2.504.337 1.909-1.296 2.747-1.027 2.747-1.027.546 1.379.202 2.398.1 2.651.64.7 1.028 1.595 1.028 2.688 0 3.848-2.339 4.695-4.566 4.943.359.309.678.92.678 1.855 0 1.338-.012 2.419-.012 2.747 0 .268.18.58.688.482A10.019 10.019 0 0022 12.017C22 6.484 17.522 2 12 2z"
          />
        </svg>
      ),
    },
    {
      id: 'google' as const,
      name: 'Google',
      desc:
        lang === 'uk'
          ? 'Швидкий доступ через Google Workspace'
          : 'Fast sign-in via Google Workspace',
      icon: (
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
      ),
    },
    {
      id: 'linkedin_oidc' as const,
      name: 'LinkedIn',
      desc:
        lang === 'uk'
          ? 'Професійний профіль та карʼєра'
          : 'Professional network and career profile',
      icon: (
        <svg className="h-4 w-4 shrink-0 fill-[#0A66C2]" viewBox="0 0 24 24">
          <path d="M19 3a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h14m-.5 15.5v-5.3a3.26 3.26 0 0 0-3.26-3.26c-.85 0-1.84.52-2.28 1.3v-1.11h-2.79v8.37h2.79v-4.93c0-.77.62-1.4 1.39-1.4a1.4 1.4 0 0 1 1.4 1.4v4.93h2.75M6.88 8.56a1.68 1.68 0 0 0 1.68-1.68c0-.93-.75-1.69-1.68-1.69a1.69 1.69 0 0 0-1.69 1.69c0 .93.76 1.68 1.69 1.68m1.39 9.94v-8.37H5.5v8.37h2.77z" />
        </svg>
      ),
    },
  ];

  return createPortal(
    <div className="fixed inset-0 z-[9999] flex items-center justify-center p-4 sm:p-6 overflow-y-auto">
      {/* Backdrop */}
      <div
        className="fixed inset-0 bg-black/80 backdrop-blur-md transition-opacity animate-fade-in"
        onClick={onClose}
      />

      {/* Modal Dialog */}
      <div className="relative z-10 w-full max-w-lg max-h-[90vh] flex flex-col overflow-hidden rounded-2xl border border-white/[0.12] bg-zinc-950 p-6 sm:p-7 shadow-2xl shadow-black/90 backdrop-blur-2xl animate-fade-in-up">
        {/* Header */}
        <div className="flex items-center justify-between pb-4 border-b border-white/[0.08] shrink-0">
          <div className="flex items-center gap-2.5">
            <div className="flex h-8 w-8 items-center justify-center rounded-lg bg-blue-600/10 border border-blue-500/20 text-blue-400">
              <Sparkles className="h-4 w-4" />
            </div>
            <h3 className="text-base font-semibold text-zinc-100">{t.userSettings.title}</h3>
          </div>
          <button
            onClick={onClose}
            className="rounded-lg p-1.5 text-zinc-400 hover:text-zinc-200 hover:bg-white/[0.06] transition-colors"
            title={t.userSettings.close}
          >
            <X className="h-5 w-5" />
          </button>
        </div>

        {/* Tab Switcher */}
        <div className="mt-4 flex rounded-xl bg-zinc-900/90 p-1 border border-white/[0.08] shrink-0">
          <button
            type="button"
            onClick={() => setActiveTab('profile')}
            className={`btn-tactile flex-1 flex items-center justify-center gap-2 rounded-lg py-2 text-xs font-semibold transition-all ${
              activeTab === 'profile'
                ? 'bg-blue-600 text-white shadow-sm'
                : 'text-zinc-400 hover:text-zinc-200'
            }`}
          >
            <User className="h-3.5 w-3.5" />
            <span>{t.userSettings.profileTab}</span>
          </button>
          <button
            type="button"
            onClick={() => setActiveTab('site')}
            className={`btn-tactile flex-1 flex items-center justify-center gap-2 rounded-lg py-2 text-xs font-semibold transition-all ${
              activeTab === 'site'
                ? 'bg-blue-600 text-white shadow-sm'
                : 'text-zinc-400 hover:text-zinc-200'
            }`}
          >
            <Palette className="h-3.5 w-3.5" />
            <span>{t.userSettings.siteTab}</span>
          </button>
        </div>

        {/* Success Toast */}
        {savedSuccess && (
          <div className="mt-4 shrink-0 flex items-center gap-2 rounded-xl border border-emerald-500/30 bg-emerald-950/40 p-3 text-xs text-emerald-300 animate-fade-in">
            <Check className="h-4 w-4 text-emerald-400 shrink-0" />
            <span>{t.userSettings.savedSuccess}</span>
          </div>
        )}

        {/* Scrollable Modal Body */}
        <div className="mt-4 overflow-y-auto pr-1 space-y-5">
          {/* Profile Tab Content */}
          {activeTab === 'profile' && (
            <div className="space-y-6">
              <form onSubmit={handleSaveProfile} className="space-y-4">
                {/* User Role Status Banner */}
                <div className="flex items-center justify-between rounded-xl border border-white/[0.08] bg-zinc-900/40 px-3.5 py-2.5">
                  <div className="flex items-center gap-2 text-xs text-zinc-300">
                    <Shield className="h-4 w-4 text-blue-400" />
                    <span>{lang === 'uk' ? 'Поточна роль:' : 'Active Role:'}</span>
                  </div>
                  <span
                    className={`rounded-md px-2 py-0.5 text-[11px] font-mono font-semibold uppercase ${
                      user?.role === 'admin'
                        ? 'bg-blue-500/20 text-blue-300 border border-blue-500/30'
                        : 'bg-zinc-800 text-zinc-300 border border-white/[0.08]'
                    }`}
                  >
                    {user?.role || 'candidate'}
                  </span>
                </div>

                {/* Nickname */}
                <div>
                  <label className="block text-xs font-medium text-zinc-300 mb-1.5">
                    {t.userSettings.fullName}
                  </label>
                  <div className="relative">
                    <User className="pointer-events-none absolute left-3.5 top-1/2 -translate-y-1/2 h-4 w-4 text-zinc-500" />
                    <input
                      type="text"
                      value={fullName}
                      onChange={(e) => setFullName(e.target.value)}
                      placeholder={lang === 'uk' ? 'Нікнейм' : 'Nickname'}
                      className="w-full rounded-xl border border-white/[0.08] bg-zinc-900/60 py-2.5 pl-10 pr-3.5 text-xs sm:text-sm text-zinc-100 placeholder-zinc-500 transition-colors focus:border-blue-500/60 focus:bg-zinc-900 focus:outline-none focus:ring-1 focus:ring-blue-500/40"
                    />
                  </div>
                  <p className="mt-1 text-[11px] text-zinc-500">
                    {t.auth.nicknameHint}
                  </p>
                </div>

                {/* Email */}
                <div>
                  <label className="block text-xs font-medium text-zinc-300 mb-1.5">
                    {t.userSettings.email}
                  </label>
                  <div className="relative">
                    <Mail className="pointer-events-none absolute left-3.5 top-1/2 -translate-y-1/2 h-4 w-4 text-zinc-500" />
                    <input
                      type="email"
                      required
                      value={email}
                      onChange={(e) => setEmail(e.target.value)}
                      placeholder="you@example.com"
                      className="w-full rounded-xl border border-white/[0.08] bg-zinc-900/60 py-2.5 pl-10 pr-3.5 text-xs sm:text-sm text-zinc-100 placeholder-zinc-500 transition-colors focus:border-blue-500/60 focus:bg-zinc-900 focus:outline-none focus:ring-1 focus:ring-blue-500/40"
                    />
                  </div>
                </div>

                {/* Avatar URL */}
                <div>
                  <label className="block text-xs font-medium text-zinc-300 mb-1.5">
                    {t.userSettings.avatarUrl}
                  </label>
                  <input
                    type="url"
                    value={avatarUrl}
                    onChange={(e) => setAvatarUrl(e.target.value)}
                    placeholder="https://images.unsplash.com/..."
                    className="w-full rounded-xl border border-white/[0.08] bg-zinc-900/60 py-2.5 px-3.5 text-xs sm:text-sm text-zinc-100 placeholder-zinc-500 transition-colors focus:border-blue-500/60 focus:bg-zinc-900 focus:outline-none focus:ring-1 focus:ring-blue-500/40"
                  />
                  <p className="mt-1 text-[11px] text-zinc-500">
                    {lang === 'uk'
                      ? 'Якщо залишити порожнім — відображатиметься фірмова аватарка із заокругленою квадратною головою.'
                      : 'If left empty, the signature rounded square head avatar will be used.'}
                  </p>
                </div>

                {/* Password Change Field */}
                <div>
                  <label className="block text-xs font-medium text-zinc-300 mb-1.5">
                    {t.userSettings.changePassword}
                  </label>
                  <div className="relative">
                    <Lock className="pointer-events-none absolute left-3.5 top-1/2 -translate-y-1/2 h-4 w-4 text-zinc-500" />
                    <input
                      type={showPassword ? 'text' : 'password'}
                      value={password}
                      onChange={(e) => setPassword(e.target.value)}
                      placeholder="••••••••"
                      className="w-full rounded-xl border border-white/[0.08] bg-zinc-900/60 py-2.5 pl-10 pr-11 text-xs sm:text-sm text-zinc-100 placeholder-zinc-500 transition-colors focus:border-blue-500/60 focus:bg-zinc-900 focus:outline-none focus:ring-1 focus:ring-blue-500/40"
                    />
                    <button
                      type="button"
                      onClick={() => setShowPassword((prev) => !prev)}
                      title={showPassword ? t.auth.hidePassword : t.auth.showPassword}
                      className="absolute right-2.5 top-1/2 -translate-y-1/2 p-1.5 rounded-lg text-zinc-400 hover:text-zinc-200 hover:bg-white/[0.06] transition-all"
                    >
                      {showPassword ? <EyeOff className="h-4 w-4" /> : <Eye className="h-4 w-4" />}
                    </button>
                  </div>
                </div>

                <div className="pt-1">
                  <button
                    type="submit"
                    className="btn-tactile w-full inline-flex items-center justify-center gap-2 rounded-xl bg-blue-600 hover:bg-blue-500 py-2.5 px-4 text-xs sm:text-sm font-semibold text-white shadow-lg shadow-blue-600/25 border border-blue-400/30 transition-all"
                  >
                    <Check className="h-4 w-4" />
                    <span>{t.userSettings.saveProfile}</span>
                  </button>
                </div>
              </form>

              {/* Account Linking / Connected Services Section */}
              <div className="pt-4 border-t border-white/[0.08] space-y-3.5">
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-2">
                    <Link2 className="h-4 w-4 text-blue-400" />
                    <h4 className="text-xs font-semibold text-zinc-100">
                      {t.userSettings.linkedAccountsTitle}
                    </h4>
                  </div>
                </div>

                {/* Explanatory Caption Card */}
                <div className="rounded-xl border border-blue-500/20 bg-blue-500/5 p-3 text-xs leading-relaxed flex items-start gap-2.5">
                  <Info className="h-4 w-4 text-blue-400 shrink-0 mt-0.5" />
                  <div className="space-y-1">
                    <p className="font-semibold text-blue-300 text-xs">
                      {lang === 'uk' ? 'Для чого це використовується?' : 'What is this used for?'}
                    </p>
                    <p className="text-zinc-400 text-[11px] leading-relaxed">
                      {t.userSettings.linkedAccountsDesc}
                    </p>
                  </div>
                </div>

                {/* Linking Feedback Toast */}
                {linkFeedback && (
                  <div
                    className={`flex items-center gap-2 rounded-xl p-3 text-xs animate-fade-in ${
                      linkFeedback.type === 'success'
                        ? 'border border-emerald-500/30 bg-emerald-950/40 text-emerald-300'
                        : 'border border-red-500/30 bg-red-950/40 text-red-300'
                    }`}
                  >
                    {linkFeedback.type === 'success' ? (
                      <Check className="h-4 w-4 text-emerald-400 shrink-0" />
                    ) : (
                      <AlertCircle className="h-4 w-4 text-red-400 shrink-0" />
                    )}
                    <span>{linkFeedback.message}</span>
                  </div>
                )}

                {/* Provider List */}
                <div className="space-y-2">
                  {socialProviders.map((provider) => {
                    const isLinked = isProviderLinked(provider.id);
                    const isLoading = linkingProvider === provider.id;

                    return (
                      <div
                        key={provider.id}
                        className="flex items-center justify-between rounded-xl border border-white/[0.08] bg-zinc-900/40 p-3 hover:border-white/[0.14] transition-all"
                      >
                        <div className="flex items-center gap-3">
                          <div className="flex h-9 w-9 items-center justify-center rounded-lg bg-zinc-900 border border-white/[0.08] shrink-0">
                            {provider.icon}
                          </div>
                          <div>
                            <div className="flex items-center gap-2">
                              <span className="text-xs font-semibold text-zinc-200">
                                {provider.name}
                              </span>
                              {isLinked ? (
                                <span className="inline-flex items-center gap-1 rounded-full bg-emerald-500/10 border border-emerald-500/20 px-2 py-0.5 text-[10px] font-medium text-emerald-400">
                                  <span className="h-1.5 w-1.5 rounded-full bg-emerald-400 animate-pulse" />
                                  {t.userSettings.linkedStatusConnected}
                                </span>
                              ) : (
                                <span className="inline-flex items-center gap-1 rounded-full bg-zinc-800/80 border border-white/[0.06] px-2 py-0.5 text-[10px] text-zinc-400">
                                  <span className="h-1.5 w-1.5 rounded-full bg-zinc-600" />
                                  {t.userSettings.linkedStatusNotConnected}
                                </span>
                              )}
                            </div>
                            <p className="text-[11px] text-zinc-500 mt-0.5">{provider.desc}</p>
                          </div>
                        </div>

                        <button
                          type="button"
                          disabled={isLoading}
                          onClick={() => handleToggleLink(provider.id)}
                          className={`btn-tactile text-xs px-3 py-1.5 rounded-lg font-medium transition-all flex items-center gap-1.5 disabled:opacity-50 ${
                            isLinked
                              ? 'border border-red-500/25 bg-red-500/10 text-red-300 hover:bg-red-500/20 hover:border-red-500/40'
                              : 'border border-blue-500/30 bg-blue-600/20 text-blue-300 hover:bg-blue-600 hover:text-white'
                          }`}
                        >
                          {isLoading ? (
                            <div className="h-3 w-3 animate-spin rounded-full border-2 border-current border-t-transparent" />
                          ) : isLinked ? (
                            <>
                              <Unlink className="h-3 w-3" />
                              <span>{t.userSettings.unlinkBtn}</span>
                            </>
                          ) : (
                            <>
                              <Link2 className="h-3 w-3" />
                              <span>{t.userSettings.linkBtn}</span>
                            </>
                          )}
                        </button>
                      </div>
                    );
                  })}
                </div>
              </div>
            </div>
          )}

        {/* Site Settings Tab Content */}
        {activeTab === 'site' && (
          <div className="mt-5 space-y-4">
            {/* Color Theme Selector */}
            <div className="rounded-xl border border-white/[0.08] bg-zinc-900/40 p-3.5">
              <label className="block text-xs font-medium text-zinc-300 mb-2">
                {t.userSettings.theme}
              </label>
              <div className="grid grid-cols-2 gap-2">
                <button
                  type="button"
                  onClick={() => handleSiteSettingChange('theme', 'oled')}
                  className={`flex items-center justify-between rounded-lg p-2.5 text-xs font-medium border transition-all ${
                    theme === 'oled'
                      ? 'border-blue-500/60 bg-blue-950/30 text-blue-200'
                      : 'border-white/[0.08] bg-zinc-900/60 text-zinc-400 hover:border-zinc-700'
                  }`}
                >
                  <span>{t.userSettings.themeDark}</span>
                  {theme === 'oled' && <Check className="h-3.5 w-3.5 text-blue-400" />}
                </button>
                <button
                  type="button"
                  onClick={() => handleSiteSettingChange('theme', 'slate')}
                  className={`flex items-center justify-between rounded-lg p-2.5 text-xs font-medium border transition-all ${
                    theme === 'slate'
                      ? 'border-blue-500/60 bg-blue-950/30 text-blue-200'
                      : 'border-white/[0.08] bg-zinc-900/60 text-zinc-400 hover:border-zinc-700'
                  }`}
                >
                  <span>{t.userSettings.themeSlate}</span>
                  {theme === 'slate' && <Check className="h-3.5 w-3.5 text-blue-400" />}
                </button>
              </div>
            </div>

            {/* Sound Effects Toggle */}
            <div className="flex items-center justify-between rounded-xl border border-white/[0.08] bg-zinc-900/40 p-3.5">
              <div className="flex items-center gap-2.5">
                <Volume2 className="h-4 w-4 text-blue-400" />
                <span className="text-xs text-zinc-300">{t.userSettings.soundEffects}</span>
              </div>
              <button
                type="button"
                onClick={() => handleSiteSettingChange('sound', !soundEnabled)}
                className={`relative inline-flex h-5 w-10 shrink-0 cursor-pointer rounded-full border-2 border-transparent transition-colors duration-200 ease-in-out focus:outline-none ${
                  soundEnabled ? 'bg-blue-600' : 'bg-zinc-800'
                }`}
              >
                <span
                  className={`pointer-events-none inline-block h-4 w-4 transform rounded-full bg-white shadow-lg ring-0 transition duration-200 ease-in-out ${
                    soundEnabled ? 'translate-x-5' : 'translate-x-0'
                  }`}
                />
              </button>
            </div>

            {/* AI Response Style */}
            <div className="rounded-xl border border-white/[0.08] bg-zinc-900/40 p-3.5">
              <div className="flex items-center gap-2 mb-2">
                <Cpu className="h-4 w-4 text-blue-400" />
                <label className="text-xs font-medium text-zinc-300">
                  {t.userSettings.aiResponseSpeed}
                </label>
              </div>
              <div className="grid grid-cols-2 gap-2">
                <button
                  type="button"
                  onClick={() => handleSiteSettingChange('aiPersona', 'balanced')}
                  className={`flex items-center justify-between rounded-lg p-2.5 text-xs font-medium border transition-all ${
                    aiPersona === 'balanced'
                      ? 'border-blue-500/60 bg-blue-950/30 text-blue-200'
                      : 'border-white/[0.08] bg-zinc-900/60 text-zinc-400 hover:border-zinc-700'
                  }`}
                >
                  <span>{t.userSettings.aiBalanced}</span>
                  {aiPersona === 'balanced' && <Check className="h-3.5 w-3.5 text-blue-400" />}
                </button>
                <button
                  type="button"
                  onClick={() => handleSiteSettingChange('aiPersona', 'strict')}
                  className={`flex items-center justify-between rounded-lg p-2.5 text-xs font-medium border transition-all ${
                    aiPersona === 'strict'
                      ? 'border-blue-500/60 bg-blue-950/30 text-blue-200'
                      : 'border-white/[0.08] bg-zinc-900/60 text-zinc-400 hover:border-zinc-700'
                  }`}
                >
                  <span>{t.userSettings.aiStrict}</span>
                  {aiPersona === 'strict' && <Check className="h-3.5 w-3.5 text-blue-400" />}
                </button>
              </div>
            </div>

            {/* Language Switcher inside Site Settings */}
            <div className="flex items-center justify-between rounded-xl border border-white/[0.08] bg-zinc-900/40 p-3.5">
              <div className="flex items-center gap-2.5">
                <Globe className="h-4 w-4 text-blue-400" />
                <span className="text-xs text-zinc-300">{t.navbar.profileMenu.language}</span>
              </div>
              <button
                type="button"
                onClick={toggleLanguage}
                className="btn-tactile flex items-center gap-1.5 rounded-lg border border-white/[0.08] bg-zinc-800 px-3 py-1 text-xs font-medium text-zinc-200 hover:bg-zinc-700"
              >
                <span className={lang === 'uk' ? 'text-blue-400 font-bold' : 'text-zinc-400'}>
                  UA
                </span>
                <span className="text-zinc-500">/</span>
                <span className={lang === 'en' ? 'text-blue-400 font-bold' : 'text-zinc-400'}>
                  EN
                </span>
              </button>
            </div>
          </div>
        )}
        </div>
      </div>
    </div>,
    document.body
  );
};
