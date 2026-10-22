'use client';

import React, { useState, useEffect } from 'react';
import Link from 'next/link';
import { usePathname, useRouter } from 'next/navigation';
import { useAuthStore } from '@/store/authStore';
import { useTranslation } from '@/lib/useTranslation';
import {
  LayoutDashboard,
  PlayCircle,
  ShieldAlert,
  ShieldCheck,
  UserCheck,
  Sparkles,
  LogOut,
  Globe,
} from 'lucide-react';

export const Navbar: React.FC = () => {
  const router = useRouter();
  const pathname = usePathname();
  const { user, isAdminViewActive, toggleAdminView, mockLogin, logout } = useAuthStore();
  const { t, lang, toggleLanguage } = useTranslation();
  const [mounted, setMounted] = useState(false);

  useEffect(() => {
    setMounted(true);
  }, []);

  const handleLogout = async () => {
    await logout();
    router.push('/');
  };

  const isRealAdmin = mounted && user?.role === 'admin';
  const showAdminFeatures = isRealAdmin && isAdminViewActive;

  return (
    <header className="sticky top-0 z-50 w-full border-b border-white/[0.08] bg-zinc-950/75 backdrop-blur-xl transition-all">
      <div className="mx-auto flex h-16 max-w-7xl items-center justify-between px-4 sm:px-6 lg:px-8">
        {/* Brand Logo & Identity */}
        <Link href="/" className="group flex items-center gap-2.5 transition-opacity">
          <div className="relative flex h-[25px] w-[25px] items-center justify-center rounded-lg bg-gradient-to-tr from-blue-600 via-indigo-600 to-rose-500 p-[1px] shadow-sm transition-transform duration-200 group-hover:scale-105">
            <div className="flex h-full w-full items-center justify-center rounded-[7px] bg-zinc-950">
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                strokeWidth="2.2"
                strokeLinecap="round"
                strokeLinejoin="round"
                className="h-3 w-3 text-zinc-100 group-hover:text-blue-400 transition-colors"
              >
                <path d="M4 17l6-6-6-6" />
                <path d="M12 19h8" />
              </svg>
            </div>
          </div>
          <span className="text-base font-semibold tracking-tight text-zinc-100">Dev Dialogue</span>
        </Link>

        {/* Primary Navigation Links */}
        <nav className="hidden md:flex items-center gap-1">
          <Link
            href="/dashboard"
            className={`flex items-center gap-2 rounded-lg px-3 py-1.5 text-xs font-medium transition-all ${
              pathname === '/dashboard'
                ? 'bg-zinc-800/80 text-zinc-100 border border-white/[0.08] shadow-sm'
                : 'text-zinc-400 hover:bg-zinc-900/60 hover:text-zinc-200'
            }`}
          >
            <LayoutDashboard className="h-3.5 w-3.5 text-blue-400" />
            {t.navbar.dashboard}
          </Link>

          <Link
            href="/interview/setup"
            className={`flex items-center gap-2 rounded-lg px-3 py-1.5 text-xs font-medium transition-all ${
              pathname?.startsWith('/interview')
                ? 'bg-zinc-800/80 text-zinc-100 border border-white/[0.08] shadow-sm'
                : 'text-zinc-400 hover:bg-zinc-900/60 hover:text-zinc-200'
            }`}
          >
            <PlayCircle className="h-3.5 w-3.5 text-blue-400" />
            {t.navbar.interview}
          </Link>

          {showAdminFeatures && (
            <Link
              href="/admin"
              className={`flex items-center gap-2 rounded-lg px-3 py-1.5 text-xs font-medium transition-all ${
                pathname === '/admin'
                  ? 'bg-blue-950/60 text-blue-200 border border-blue-500/30 shadow-sm'
                  : 'text-blue-400/90 hover:bg-blue-950/30 hover:text-blue-200'
              }`}
            >
              <ShieldAlert className="h-3.5 w-3.5 text-blue-400" />
              {t.navbar.admin}
            </Link>
          )}
        </nav>

        {/* Right Section: Mode Toggle, Language Toggle & Profile */}
        <div className="flex items-center gap-2.5">
          {/* Admin Mode Switcher Button — ONLY visible to real admins */}
          {isRealAdmin && (
            <button
              onClick={toggleAdminView}
              title={isAdminViewActive ? 'Switch to Candidate View' : 'Switch to Admin View'}
              className={`flex items-center gap-1.5 rounded-lg px-2.5 py-1 text-xs font-medium transition-all border ${
                isAdminViewActive
                  ? 'bg-blue-950/60 text-blue-200 border-blue-500/40 hover:bg-blue-900/50 shadow-sm shadow-blue-500/10'
                  : 'bg-zinc-900/90 text-zinc-300 border-white/[0.08] hover:bg-zinc-800'
              }`}
            >
              {isAdminViewActive ? (
                <>
                  <ShieldCheck className="h-3.5 w-3.5 text-blue-400" />
                  <span>{t.navbar.adminRole}</span>
                </>
              ) : (
                <>
                  <UserCheck className="h-3.5 w-3.5 text-blue-400" />
                  <span>{t.navbar.candidateRole}</span>
                </>
              )}
            </button>
          )}

          {/* Language Toggle Button — Placed immediately to the left of Sign In / User Profile */}
          <button
            onClick={toggleLanguage}
            title={lang === 'uk' ? 'Switch to English' : 'Перемкнути на українську'}
            aria-label={t.navbar.langAria}
            className="btn-tactile flex items-center gap-1.5 rounded-xl border border-white/[0.08] bg-zinc-900/80 px-2.5 py-1.5 text-xs font-medium text-zinc-300 hover:border-zinc-700 hover:bg-zinc-800 transition-all shadow-sm"
          >
            <Globe className="h-3.5 w-3.5 text-blue-400" />
            <span
              className={
                lang === 'uk'
                  ? 'text-blue-400 font-bold font-mono'
                  : 'text-zinc-500 font-mono hover:text-zinc-300'
              }
            >
              UA
            </span>
            <span className="text-zinc-600 font-mono">/</span>
            <span
              className={
                lang === 'en'
                  ? 'text-blue-400 font-bold font-mono'
                  : 'text-zinc-500 font-mono hover:text-zinc-300'
              }
            >
              EN
            </span>
          </button>

          {/* User Profile or Sign In Button */}
          {user ? (
            <div
              className="flex items-center gap-2 pl-2 border-l border-white/[0.08]"
              suppressHydrationWarning
            >
              <div className="hidden sm:block text-right leading-tight" suppressHydrationWarning>
                <div className="text-xs font-medium text-zinc-200" suppressHydrationWarning>
                  {user.fullName || user.email}
                </div>
                <div
                  className="text-[10px] text-zinc-500 font-mono capitalize"
                  suppressHydrationWarning
                >
                  {user.role} {isRealAdmin && !isAdminViewActive && '(User View)'}
                </div>
              </div>
              <button
                onClick={() => mockLogin(user.role === 'admin' ? 'user' : 'admin')}
                title={t.navbar.roleSwitchTitle}
                className="btn-tactile flex h-8 w-8 items-center justify-center rounded-lg bg-zinc-900 border border-white/[0.08] text-zinc-400 hover:text-blue-300 hover:border-blue-500/30 hover:bg-zinc-800"
              >
                <Sparkles className="h-3.5 w-3.5 text-blue-400" />
              </button>
              <button
                onClick={handleLogout}
                title={t.navbar.logoutTitle}
                className="btn-tactile flex h-8 w-8 items-center justify-center rounded-lg bg-zinc-900 border border-white/[0.08] text-zinc-400 hover:text-red-300 hover:border-red-500/30 hover:bg-zinc-800"
              >
                <LogOut className="h-3.5 w-3.5" />
              </button>
            </div>
          ) : (
            <Link
              href="/login"
              className="btn-tactile inline-flex items-center gap-2 rounded-xl bg-blue-600 hover:bg-blue-500 px-3.5 py-1.5 text-xs font-semibold text-white shadow-md shadow-blue-600/25 border border-blue-400/30"
            >
              {/* Custom Generated Login SVG Icon */}
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                strokeWidth="2.2"
                strokeLinecap="round"
                strokeLinejoin="round"
                className="h-3.5 w-3.5 text-blue-100"
              >
                <path d="M15 3h4a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2h-4" />
                <polyline points="10 17 15 12 10 7" />
                <line x1="15" y1="12" x2="3" y2="12" />
              </svg>
              <span>{t.navbar.signIn}</span>
            </Link>
          )}
        </div>
      </div>
    </header>
  );
};
