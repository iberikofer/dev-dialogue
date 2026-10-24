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
  User,
  Settings,
  ChevronDown,
  HelpCircle,
} from 'lucide-react';
import { UserSettingsModal } from './UserSettingsModal';

// Custom SVG: User with a square head with rounded corners
export const SquareHeadUserIcon: React.FC<{ className?: string }> = ({ className = 'h-4 w-4' }) => (
  <svg
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    strokeWidth="1.8"
    strokeLinecap="round"
    strokeLinejoin="round"
    className={className}
  >
    {/* Square head with rounded corners */}
    <rect x="7" y="3" width="10" height="9" rx="2.5" />
    {/* Eyes */}
    <circle cx="10" cy="7.5" r="0.75" fill="currentColor" stroke="none" />
    <circle cx="14" cy="7.5" r="0.75" fill="currentColor" stroke="none" />
    {/* Gentle tech smile */}
    <path d="M10.5 9.5a2 2 0 0 0 3 0" strokeWidth="1.2" />
    {/* Shoulders */}
    <path d="M5 20.5a7 7 0 0 1 14 0" />
  </svg>
);

export const Navbar: React.FC = () => {
  const router = useRouter();
  const pathname = usePathname();
  const { user, isAdminViewActive, toggleAdminView, mockLogin, logout } = useAuthStore();
  const { t, lang, toggleLanguage } = useTranslation();
  const [mounted, setMounted] = useState(false);
  const [dropdownOpen, setDropdownOpen] = useState(false);
  const [settingsModalOpen, setSettingsModalOpen] = useState(false);
  const [settingsModalTab, setSettingsModalTab] = useState<'profile' | 'site'>('profile');
  const dropdownRef = React.useRef<HTMLDivElement>(null);

  useEffect(() => {
    setMounted(true);
  }, []);

  // Close dropdown on click outside
  useEffect(() => {
    const handleClickOutside = (event: MouseEvent) => {
      if (dropdownRef.current && !dropdownRef.current.contains(event.target as Node)) {
        setDropdownOpen(false);
      }
    };
    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
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
          <div className="relative flex h-7 w-7 items-center justify-center rounded-lg border border-blue-500/60 bg-blue-500/10 shadow-sm shadow-blue-500/25 transition-all duration-200 group-hover:scale-105 group-hover:border-blue-400 group-hover:bg-blue-500/20">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              strokeWidth="2.2"
              strokeLinecap="round"
              strokeLinejoin="round"
              className="h-3.5 w-3.5 text-blue-400 group-hover:text-blue-300 transition-colors"
            >
              <path d="M4 17l6-6-6-6" />
              <path d="M12 19h8" />
            </svg>
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
            <div className="relative flex items-center gap-2 pl-1" ref={dropdownRef} suppressHydrationWarning>
              {/* User Block Trigger: Bordered pill with vertically centered name, Admin label if admin, squircle avatar and chevron */}
              <button
                type="button"
                onClick={() => setDropdownOpen((prev) => !prev)}
                aria-expanded={dropdownOpen}
                aria-haspopup="true"
                className="btn-tactile group flex items-center gap-2.5 rounded-xl border border-white/[0.12] bg-zinc-900/80 hover:bg-zinc-800/90 hover:border-white/[0.22] px-3 py-1.5 transition-all text-left shadow-sm focus:outline-none focus:ring-1 focus:ring-blue-500/50"
              >
                {/* User Name & Admin Status */}
                <div className="flex flex-col justify-center text-right leading-tight select-none min-w-[3rem]">
                  <span className="text-xs font-semibold text-zinc-100 group-hover:text-white transition-colors">
                    {user.fullName || user.email.split('@')[0]}
                  </span>
                  {user.role === 'admin' && (
                    <span className="text-[10px] font-mono font-medium text-blue-400 mt-0.5">
                      Admin
                    </span>
                  )}
                </div>

                {/* Squircle (square with rounded corners) box for user picture */}
                <div className="relative flex h-8 w-8 shrink-0 items-center justify-center overflow-hidden rounded-lg border border-white/[0.12] bg-zinc-800 shadow-inner group-hover:border-blue-500/40 transition-colors">
                  {user.avatarUrl ? (
                    <img
                      src={user.avatarUrl}
                      alt={user.fullName || 'User avatar'}
                      className="h-full w-full object-cover"
                    />
                  ) : (
                    <SquareHeadUserIcon className="h-4.5 w-4.5 text-blue-400 group-hover:text-blue-300 transition-colors" />
                  )}
                </div>

                {/* Subtle dropdown chevron */}
                <ChevronDown
                  className={`h-3.5 w-3.5 text-zinc-400 transition-transform duration-200 ${
                    dropdownOpen ? 'rotate-180 text-blue-400' : 'group-hover:text-zinc-200'
                  }`}
                />
              </button>

              {/* Dropdown Menu */}
              {dropdownOpen && (
                <div className="absolute right-0 top-full mt-2 w-64 origin-top-right rounded-2xl border border-white/[0.12] bg-zinc-950/95 p-1.5 shadow-2xl shadow-black/90 backdrop-blur-2xl z-50 animate-fade-in-up">
                  {/* Top User Info Header */}
                  <div className="flex items-center gap-2.5 px-3 py-2.5 border-b border-white/[0.06] mb-1">
                    <div className="flex h-9 w-9 shrink-0 items-center justify-center overflow-hidden rounded-lg border border-white/[0.12] bg-zinc-800 text-blue-400">
                      {user.avatarUrl ? (
                        <img
                          src={user.avatarUrl}
                          alt={user.fullName || 'User'}
                          className="h-full w-full object-cover"
                        />
                      ) : (
                        <SquareHeadUserIcon className="h-5 w-5 text-blue-400" />
                      )}
                    </div>
                    <div className="flex flex-col min-w-0">
                      <span className="text-xs font-semibold text-zinc-100 truncate">
                        {user.fullName || user.email.split('@')[0]}
                      </span>
                      <span className="text-[11px] text-zinc-400 truncate">{user.email}</span>
                      <span className="text-[10px] font-mono text-blue-400 uppercase tracking-wider">
                        {user.role === 'admin' ? '🛡 Admin' : '👤 Candidate'}
                      </span>
                    </div>
                  </div>

                  {/* Menu Items */}
                  <div className="space-y-0.5">
                    {/* Profile Settings */}
                    <button
                      onClick={() => {
                        setSettingsModalTab('profile');
                        setSettingsModalOpen(true);
                        setDropdownOpen(false);
                      }}
                      className="flex w-full items-center gap-2.5 rounded-xl px-2.5 py-2 text-xs font-medium text-zinc-300 hover:bg-zinc-900 hover:text-white transition-colors"
                    >
                      <User className="h-4 w-4 text-blue-400 shrink-0" />
                      <span>{t.navbar.profileMenu.profile}</span>
                    </button>

                    {/* Site Settings */}
                    <button
                      onClick={() => {
                        setSettingsModalTab('site');
                        setSettingsModalOpen(true);
                        setDropdownOpen(false);
                      }}
                      className="flex w-full items-center gap-2.5 rounded-xl px-2.5 py-2 text-xs font-medium text-zinc-300 hover:bg-zinc-900 hover:text-white transition-colors"
                    >
                      <Settings className="h-4 w-4 text-indigo-400 shrink-0" />
                      <span>{t.navbar.profileMenu.siteSettings}</span>
                    </button>

                    {/* My Interviews / Dashboard */}
                    <Link
                      href="/dashboard"
                      onClick={() => setDropdownOpen(false)}
                      className="flex w-full items-center gap-2.5 rounded-xl px-2.5 py-2 text-xs font-medium text-zinc-300 hover:bg-zinc-900 hover:text-white transition-colors"
                    >
                      <LayoutDashboard className="h-4 w-4 text-amber-400 shrink-0" />
                      <span>{t.navbar.profileMenu.interviewHistory}</span>
                    </Link>

                    {/* Start New Interview */}
                    <Link
                      href="/interview/setup"
                      onClick={() => setDropdownOpen(false)}
                      className="flex w-full items-center gap-2.5 rounded-xl px-2.5 py-2 text-xs font-medium text-zinc-300 hover:bg-zinc-900 hover:text-white transition-colors"
                    >
                      <PlayCircle className="h-4 w-4 text-rose-400 shrink-0" />
                      <span>{t.navbar.profileMenu.startInterview}</span>
                    </Link>

                    {/* Demo Role Switcher Item */}
                    <button
                      onClick={() => {
                        mockLogin(user.role === 'admin' ? 'user' : 'admin');
                      }}
                      className="flex w-full items-center justify-between rounded-xl px-2.5 py-2 text-xs font-medium text-zinc-300 hover:bg-zinc-900 hover:text-white transition-colors"
                    >
                      <div className="flex items-center gap-2.5">
                        <Sparkles className="h-4 w-4 text-blue-400 shrink-0" />
                        <span>{t.navbar.profileMenu.switchRole}</span>
                      </div>
                      <span className="rounded bg-blue-950/80 px-1.5 py-0.5 text-[10px] font-mono text-blue-300 border border-blue-500/30">
                        {user.role === 'admin' ? '→ User' : '→ Admin'}
                      </span>
                    </button>

                    {/* Help & FAQ */}
                    <a
                      href="https://github.com/iberikofer/dev-dialogue"
                      target="_blank"
                      rel="noopener noreferrer"
                      onClick={() => setDropdownOpen(false)}
                      className="flex w-full items-center gap-2.5 rounded-xl px-2.5 py-2 text-xs font-medium text-zinc-300 hover:bg-zinc-900 hover:text-white transition-colors"
                    >
                      <HelpCircle className="h-4 w-4 text-zinc-400 shrink-0" />
                      <span>{t.navbar.profileMenu.help}</span>
                    </a>
                  </div>

                  {/* Divider & Sign Out */}
                  <div className="mt-1 pt-1 border-t border-white/[0.06]">
                    <button
                      onClick={() => {
                        setDropdownOpen(false);
                        handleLogout();
                      }}
                      className="flex w-full items-center gap-2.5 rounded-xl px-2.5 py-2 text-xs font-medium text-red-400 hover:bg-red-950/30 hover:text-red-300 transition-colors"
                    >
                      <LogOut className="h-4 w-4 shrink-0" />
                      <span>{t.navbar.profileMenu.signOut}</span>
                    </button>
                  </div>
                </div>
              )}
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

      {/* User Settings Modal */}
      <UserSettingsModal
        isOpen={settingsModalOpen}
        onClose={() => setSettingsModalOpen(false)}
        defaultTab={settingsModalTab}
      />
    </header>
  );
};
