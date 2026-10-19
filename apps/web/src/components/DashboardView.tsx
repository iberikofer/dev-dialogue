'use client';

import React, { useState, useEffect } from 'react';
import Link from 'next/link';
import { History, TrendingUp, Clock, PlusCircle, ExternalLink } from 'lucide-react';
import { InterviewSummary, UserDashboardStats } from '@dev-dialogue/api-contracts';
import { api } from '@/lib/api-client';
import { useAuthStore } from '@/store/authStore';
import { useTranslation } from '@/lib/useTranslation';

export const DashboardView: React.FC = () => {
  const { user } = useAuthStore();
  const { t } = useTranslation();
  const [loading, setLoading] = useState(true);
  const [stats, setStats] = useState<UserDashboardStats>({
    totalInterviews: 0,
    averageScore: 0,
    dailyInterviewsCount: 0,
    dailyLimit: 10,
    remainingToday: 10,
  });
  const [interviews, setInterviews] = useState<InterviewSummary[]>([]);

  useEffect(() => {
    async function loadDashboard() {
      try {
        const [statsData, listData] = await Promise.all([
          api.interviews.getStats(),
          api.interviews.list(),
        ]);
        setStats(statsData);
        setInterviews(listData);
      } catch {
        // Fallback demo data for immediate testing
        setStats({
          totalInterviews: 4,
          averageScore: 82.5,
          dailyInterviewsCount: 2,
          dailyLimit: 10,
          remainingToday: 8,
        });
        setInterviews([
          {
            id: 'mock-1',
            userId: 'u1',
            direction: 'frontend',
            level: 'junior',
            language: 'uk',
            stage: 'hard_skills',
            status: 'completed',
            score: 88,
            questionCount: 5,
            createdAt: new Date(Date.now() - 3600000 * 2).toISOString(),
            updatedAt: new Date().toISOString(),
          },
          {
            id: 'mock-2',
            userId: 'u1',
            direction: 'backend',
            level: 'junior',
            language: 'uk',
            stage: 'hard_skills',
            status: 'completed',
            score: 77,
            questionCount: 6,
            createdAt: new Date(Date.now() - 86400000).toISOString(),
            updatedAt: new Date().toISOString(),
          },
        ]);
      } finally {
        setLoading(false);
      }
    }

    loadDashboard();
  }, []);

  return (
    <div className="mx-auto max-w-6xl px-4 py-8">
      {/* Welcome Banner */}
      <div className="flex flex-col gap-4 sm:flex-row sm:items-center sm:justify-between border-b border-white/[0.08] pb-6 mb-8">
        <div>
          <h1 className="text-2xl font-bold tracking-tight text-zinc-100 sm:text-3xl">
            {t.dashboard.welcome} {user?.fullName || t.dashboard.candidateDefault} 👋
          </h1>
          <p className="mt-1 text-sm text-zinc-400">{t.dashboard.subtitle}</p>
        </div>
        <Link
          href="/interview/setup"
          className="btn-tactile inline-flex items-center gap-2 rounded-xl bg-blue-600 hover:bg-blue-500 px-5 py-2.5 text-sm font-semibold text-white shadow-lg shadow-blue-600/25 border border-blue-400/30"
        >
          <PlusCircle className="h-4 w-4" />
          <span>{t.dashboard.startNew}</span>
        </Link>
      </div>

      {/* KPI Stats Grid */}
      <div className="grid grid-cols-1 gap-4 sm:grid-cols-3 mb-8">
        {/* Total Interviews */}
        <div className="card-interactive rounded-2xl border border-white/[0.08] bg-zinc-900/40 p-5">
          <div className="flex items-center justify-between text-zinc-400 mb-2">
            <span className="text-xs font-semibold uppercase tracking-wider font-mono">
              {t.dashboard.kpiPassed}
            </span>
            <History className="h-4 w-4 text-blue-400" />
          </div>
          {loading ? (
            <div className="h-9 w-20 rounded-lg bg-zinc-800/80 animate-pulse my-0.5" />
          ) : (
            <div className="text-3xl font-extrabold text-zinc-100 animate-fade-in-up font-mono">
              {stats.totalInterviews}
            </div>
          )}
          <div className="mt-1 text-xs text-zinc-500">{t.dashboard.kpiPassedDesc}</div>
        </div>

        {/* Average Score */}
        <div className="card-interactive rounded-2xl border border-white/[0.08] bg-zinc-900/40 p-5">
          <div className="flex items-center justify-between text-zinc-400 mb-2">
            <span className="text-xs font-semibold uppercase tracking-wider font-mono">
              {t.dashboard.kpiScore}
            </span>
            <TrendingUp className="h-4 w-4 text-emerald-400" />
          </div>
          {loading ? (
            <div className="h-9 w-24 rounded-lg bg-zinc-800/80 animate-pulse my-0.5" />
          ) : (
            <div className="text-3xl font-extrabold text-emerald-400 animate-fade-in-up font-mono">
              {stats.averageScore ? `${stats.averageScore}%` : '—'}
            </div>
          )}
          <div className="mt-1 text-xs text-zinc-500">{t.dashboard.kpiScoreDesc}</div>
        </div>

        {/* Daily Quota */}
        <div className="card-interactive rounded-2xl border border-white/[0.08] bg-zinc-900/40 p-5">
          <div className="flex items-center justify-between text-zinc-400 mb-2">
            <span className="text-xs font-semibold uppercase tracking-wider font-mono">
              {t.dashboard.kpiLimit}
            </span>
            <Clock className="h-4 w-4 text-blue-400" />
          </div>
          {loading ? (
            <div className="h-9 w-28 rounded-lg bg-zinc-800/80 animate-pulse my-0.5" />
          ) : (
            <div className="text-3xl font-extrabold text-zinc-100 animate-fade-in-up font-mono">
              {stats.remainingToday}{' '}
              <span className="text-sm font-normal text-zinc-500">/ {stats.dailyLimit}</span>
            </div>
          )}
          <div className="mt-1 text-xs text-zinc-500">{t.dashboard.kpiLimitDesc}</div>
        </div>
      </div>

      {/* Sessions History Table */}
      <div className="rounded-2xl border border-white/[0.08] bg-zinc-900/40 overflow-hidden shadow-lg">
        <div className="border-b border-white/[0.08] px-6 py-4 flex items-center justify-between">
          <h2 className="font-bold text-zinc-100 text-base">{t.dashboard.historyTitle}</h2>
          <span className="text-xs text-zinc-400 font-mono">
            {loading ? (
              <span className="inline-block h-3 w-12 bg-zinc-800/80 rounded animate-pulse" />
            ) : (
              `${t.dashboard.totalPrefix} ${interviews.length}`
            )}
          </span>
        </div>

        {loading ? (
          <div className="p-6 space-y-4">
            {[1, 2, 3].map((i) => (
              <div
                key={i}
                className="flex items-center justify-between gap-4 py-2 border-b border-white/[0.04] last:border-none"
              >
                <div className="h-4 w-28 bg-zinc-800/70 rounded animate-pulse" />
                <div className="h-4 w-24 bg-zinc-800/70 rounded animate-pulse" />
                <div className="h-4 w-16 bg-zinc-800/70 rounded animate-pulse" />
                <div className="h-4 w-14 bg-zinc-800/70 rounded animate-pulse" />
                <div className="h-4 w-20 bg-zinc-800/70 rounded animate-pulse" />
              </div>
            ))}
          </div>
        ) : interviews.length === 0 ? (
          <div className="p-8 text-center text-zinc-400 text-sm">{t.dashboard.emptyHistory}</div>
        ) : (
          <div className="overflow-x-auto">
            <table className="w-full text-left text-sm text-zinc-300">
              <thead className="bg-zinc-950/60 text-xs font-semibold uppercase tracking-wider text-zinc-400 border-b border-white/[0.08]">
                <tr>
                  <th className="px-6 py-3">{t.dashboard.tableDate}</th>
                  <th className="px-6 py-3">{t.dashboard.tableDirection}</th>
                  <th className="px-6 py-3">{t.dashboard.tableLevel}</th>
                  <th className="px-6 py-3">{t.dashboard.tableStatus}</th>
                  <th className="px-6 py-3">{t.dashboard.tableScore}</th>
                  <th className="px-6 py-3 text-right">{t.dashboard.tableReport}</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-white/[0.06]">
                {interviews.map((item) => (
                  <tr key={item.id} className="hover:bg-zinc-800/30 transition-colors">
                    <td className="px-6 py-4 whitespace-nowrap text-xs text-zinc-400 font-mono">
                      {new Date(item.createdAt).toLocaleDateString([], {
                        month: 'short',
                        day: 'numeric',
                        hour: '2-digit',
                        minute: '2-digit',
                      })}
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap font-medium text-zinc-100 capitalize">
                      {item.direction}
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-xs capitalize text-zinc-300">
                      {item.level}
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap">
                      <span
                        className={`inline-flex items-center gap-1 rounded-md px-2 py-0.5 text-xs font-medium ${
                          item.status === 'completed'
                            ? 'bg-emerald-500/10 text-emerald-400 border border-emerald-500/20'
                            : 'bg-blue-500/10 text-blue-400 border border-blue-500/20'
                        }`}
                      >
                        {item.status === 'completed'
                          ? t.dashboard.statusCompleted
                          : t.dashboard.statusActive}
                      </span>
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap font-bold text-zinc-100 font-mono">
                      {item.score !== null ? `${item.score}%` : '—'}
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-right">
                      <Link
                        href={`/interview/${item.id}/report`}
                        className="btn-tactile inline-flex items-center gap-1 text-xs font-semibold text-blue-400 hover:text-blue-300"
                      >
                        <span>{t.dashboard.viewReport}</span>
                        <ExternalLink className="h-3 w-3" />
                      </Link>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}
      </div>
    </div>
  );
};
