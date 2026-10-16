'use client';

import React, { useState, useEffect } from 'react';
import Link from 'next/link';
import { usePathname } from 'next/navigation';
import { useAuthStore } from '@/store/authStore';
import {
  BrainCircuit,
  LayoutDashboard,
  PlayCircle,
  ShieldAlert,
  ShieldCheck,
  UserCheck,
  Sparkles,
} from 'lucide-react';

export const Navbar: React.FC = () => {
  const pathname = usePathname();
  const { user, isAdminViewActive, toggleAdminView, mockLogin } = useAuthStore();
  const [mounted, setMounted] = useState(false);

  useEffect(() => {
    setMounted(true);
  }, []);

  const isRealAdmin = mounted && user?.role === 'admin';
  const showAdminFeatures = isRealAdmin && isAdminViewActive;

  return (
    <header className="sticky top-0 z-50 w-full border-b border-white/[0.08] bg-zinc-950/75 backdrop-blur-xl transition-all">
      <div className="mx-auto flex h-16 max-w-7xl items-center justify-between px-4 sm:px-6 lg:px-8">
        {/* Brand Logo & Identity */}
        <Link href="/" className="group flex items-center gap-3 transition-opacity">
          <div className="relative flex h-9 w-9 items-center justify-center rounded-xl bg-gradient-to-tr from-blue-600 via-indigo-600 to-rose-500 p-[1px] shadow-sm transition-transform duration-200 group-hover:scale-105">
            <div className="flex h-full w-full items-center justify-center rounded-[11px] bg-zinc-950">
              <BrainCircuit className="h-4.5 w-4.5 text-zinc-100 group-hover:text-rose-300 transition-colors" />
            </div>
          </div>
          <div className="flex items-center gap-2">
            <span className="text-base font-semibold tracking-tight text-zinc-100">
              Dev Dialogue
            </span>
            <span className="hidden sm:inline-flex items-center gap-1 rounded-md bg-zinc-900/80 px-2 py-0.5 text-[10px] font-mono font-medium text-zinc-400 border border-white/[0.06]">
              <span className="h-1.5 w-1.5 rounded-full bg-emerald-400 animate-pulse" />
              v2.0
            </span>
          </div>
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
            <LayoutDashboard className="h-3.5 w-3.5 text-rose-400" />
            Дашборд
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
            Співбесіда
          </Link>

          {showAdminFeatures && (
            <Link
              href="/admin"
              className={`flex items-center gap-2 rounded-lg px-3 py-1.5 text-xs font-medium transition-all ${
                pathname === '/admin'
                  ? 'bg-indigo-950/60 text-indigo-200 border border-indigo-500/30 shadow-sm'
                  : 'text-indigo-400/90 hover:bg-indigo-950/30 hover:text-indigo-200'
              }`}
            >
              <ShieldAlert className="h-3.5 w-3.5" />
              Банк питань (Адмін)
            </Link>
          )}
        </nav>

        {/* Right Section: Mode Toggle & Profile */}
        <div className="flex items-center gap-2.5">
          {/* Admin Mode Switcher Button — ONLY visible to real admins */}
          {isRealAdmin && (
            <button
              onClick={toggleAdminView}
              title={
                isAdminViewActive
                  ? 'Перемкнутись на режим кандидата'
                  : 'Перемкнутись на режим адміна'
              }
              className={`flex items-center gap-1.5 rounded-lg px-2.5 py-1 text-xs font-medium transition-all border ${
                isAdminViewActive
                  ? 'bg-indigo-950/50 text-indigo-300 border-indigo-500/30 hover:bg-indigo-900/40'
                  : 'bg-zinc-900/90 text-zinc-300 border-white/[0.08] hover:bg-zinc-800'
              }`}
            >
              {isAdminViewActive ? (
                <>
                  <ShieldCheck className="h-3.5 w-3.5 text-indigo-400" />
                  <span>🛡 Адмін</span>
                </>
              ) : (
                <>
                  <UserCheck className="h-3.5 w-3.5 text-emerald-400" />
                  <span>👤 Кандидат</span>
                </>
              )}
            </button>
          )}

          {/* User Profile or Quick Demo Role Switcher */}
          {mounted && user ? (
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
                title="Швидкий тест: змінити роль (Admin ↔ User)"
                className="flex h-8 w-8 items-center justify-center rounded-lg bg-zinc-900 border border-white/[0.08] text-zinc-400 hover:text-rose-300 hover:border-rose-500/30 hover:bg-zinc-800 transition"
              >
                <Sparkles className="h-3.5 w-3.5 text-rose-400" />
              </button>
            </div>
          ) : (
            <div className="h-8 w-20" />
          )}
        </div>
      </div>
    </header>
  );
};
