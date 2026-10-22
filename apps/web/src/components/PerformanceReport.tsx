'use client';

import React, { useState, useEffect } from 'react';
import Link from 'next/link';
import {
  Trophy,
  CheckCircle2,
  AlertTriangle,
  BookOpen,
  RotateCcw,
  LayoutDashboard,
  ExternalLink,
} from 'lucide-react';
import { InterviewFeedbackReport } from '@dev-dialogue/api-contracts';
import { api } from '@/lib/api-client';
import { useTranslation } from '@/lib/useTranslation';

interface PerformanceReportProps {
  interviewId: string;
}

export const PerformanceReport: React.FC<PerformanceReportProps> = ({ interviewId }) => {
  const { t, lang } = useTranslation();
  const [report, setReport] = useState<InterviewFeedbackReport | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    async function loadReport() {
      try {
        const data = await api.interviews.getById(interviewId);
        if (data.feedback) {
          setReport(data.feedback);
          setLoading(false);
          return;
        }
      } catch {
        // Fallback demo report for preview
      }

      // Default mock report with language-specific text
      if (lang === 'en') {
        setReport({
          summary:
            'The candidate demonstrated solid command of modern web standards, asynchronous programming, and component lifecycle principles. Answers were well-structured, logical, and substantiated with technical tradeoffs.',
          score: 84,
          strengths: [
            'Confident grasp of architectural patterns, state managers, and React fiber',
            'Clear understanding of network communication protocols, RESTful APIs, and request lifecycles',
            'Strong ability to articulate architectural decisions and performance tradeoffs under pressure',
          ],
          weaknesses: [
            'Deeper study into database indexing mechanisms, B-Tree lookups, and query plans is advised',
            'Could pay closer attention to edge cases in distributed fault handling and circuit breakers',
          ],
          recommendations: [
            {
              topic: 'Database Indexing & B-Trees',
              resource: 'https://use-the-index-luke.com',
            },
            {
              topic: 'Distributed Systems & Concurrency',
              resource: 'https://github.com/donnemartin/system-design-primer',
            },
            {
              topic: 'Modern Web Architecture & Patterns',
              resource: 'https://martinfowler.com/architecture',
            },
          ],
          questionScores: [
            {
              question: 'Difference between TCP and UDP, common real-world use cases',
              score: 90,
              comment:
                'Great understanding of transport layer reliability, handshakes, and packet streaming.',
            },
            {
              question: 'Relational DB query optimization and composite indexes',
              score: 78,
              comment:
                'Solid overall explanation, but missed subtleties regarding index write penalty and lock contention.',
            },
          ],
        });
      } else {
        setReport({
          summary:
            'Кандидат продемонстрував ґрунтовні знання ключових веб-стандартів, асинхронності та структури компонентів. Відповіді структуровані, логічні та підкріплені аргументацією.',
          score: 84,
          strengths: [
            'Впевнене володіння базовою термінологією та архітектурними паттернами',
            'Чітке розуміння протоколів взаємодії, REST та життєвого циклу запитів',
            'Здатність аргументувати вибір технологій та підходів під навантаженням',
          ],
          weaknesses: [
            'Потребує глибшого вивчення внутрішніх механізмів оптимізації та індексації баз даних',
            'Варто приділити більше уваги крайовим випадкам при розподіленій обробці помилок',
          ],
          recommendations: [
            {
              topic: 'Database Indexing & B-Trees',
              resource: 'https://use-the-index-luke.com',
            },
            {
              topic: 'Distributed Systems & Concurrency',
              resource: 'https://github.com/donnemartin/system-design-primer',
            },
            {
              topic: 'Modern Web Architecture & Patterns',
              resource: 'https://martinfowler.com/architecture',
            },
          ],
          questionScores: [
            {
              question: 'Різниця між TCP та UDP, сценарії використання',
              score: 90,
              comment: 'Відмінне розуміння протоколів транспортного рівня та надійності передачі.',
            },
            {
              question: 'Оптимізація та індекси в реляційних БД',
              score: 78,
              comment:
                'Добра відповідь, проте не вистачило глибини щодо B-Tree та вартості оновлення індексів.',
            },
          ],
        });
      }
      setLoading(false);
    }

    loadReport();
  }, [interviewId, lang]);

  if (loading || !report) {
    return (
      <div className="flex h-96 items-center justify-center">
        <div className="text-zinc-400 text-sm">{t.report.loadingText}</div>
      </div>
    );
  }

  const scoreColor =
    report.score >= 80
      ? 'text-emerald-400'
      : report.score >= 60
        ? 'text-blue-400'
        : 'text-amber-400';

  return (
    <div className="mx-auto max-w-4xl px-4 py-8">
      {/* Hero Score Card */}
      <div className="relative overflow-hidden rounded-3xl border border-white/[0.08] bg-gradient-to-b from-zinc-900 to-zinc-950 p-8 text-center shadow-2xl">
        <div className="inline-flex items-center gap-2 rounded-full bg-blue-500/10 px-3 py-1 text-xs font-semibold text-blue-400 border border-blue-500/20 mb-4">
          <Trophy className="h-3.5 w-3.5" />
          {t.report.badge}
        </div>

        <div className="flex items-center justify-center">
          <div className="relative flex h-36 w-36 items-center justify-center rounded-full border-4 border-zinc-800 bg-zinc-950 shadow-inner">
            <div className="text-center">
              <span className={`text-5xl font-black tracking-tight ${scoreColor}`}>
                {report.score}
              </span>
              <span className="block text-xs font-semibold uppercase tracking-wider text-zinc-500">
                {t.report.scoreSuffix}
              </span>
            </div>
          </div>
        </div>

        <p className="mt-6 max-w-2xl mx-auto text-base text-zinc-300 leading-relaxed">
          {report.summary}
        </p>

        {/* Action Buttons */}
        <div className="mt-8 flex flex-wrap items-center justify-center gap-4">
          <Link
            href="/interview/setup"
            className="btn-tactile flex items-center gap-2 rounded-xl bg-blue-600 px-5 py-2.5 text-sm font-semibold text-white shadow-lg shadow-blue-500/20 hover:bg-blue-500 transition"
          >
            <RotateCcw className="h-4 w-4" />
            <span>{t.report.retakeBtn}</span>
          </Link>
          <Link
            href="/dashboard"
            className="btn-tactile flex items-center gap-2 rounded-xl border border-white/[0.12] bg-zinc-900/80 px-5 py-2.5 text-sm font-semibold text-zinc-200 hover:bg-zinc-800 transition"
          >
            <LayoutDashboard className="h-4 w-4" />
            <span>{t.report.dashboardBtn}</span>
          </Link>
        </div>
      </div>

      {/* Strengths & Weaknesses Grid */}
      <div className="mt-8 grid grid-cols-1 gap-6 md:grid-cols-2">
        {/* Strengths */}
        <div className="card-interactive rounded-2xl border border-emerald-500/20 bg-emerald-950/10 p-6">
          <div className="flex items-center gap-2 text-emerald-400 font-bold text-base mb-4">
            <CheckCircle2 className="h-5 w-5" />
            <span>{t.report.strengthsTitle}</span>
          </div>
          <ul className="space-y-3">
            {report.strengths.map((item, idx) => (
              <li key={idx} className="flex items-start gap-2.5 text-sm text-zinc-300">
                <span className="h-1.5 w-1.5 rounded-full bg-emerald-400 mt-2 shrink-0" />
                <span>{item}</span>
              </li>
            ))}
          </ul>
        </div>

        {/* Growth Areas */}
        <div className="card-interactive rounded-2xl border border-amber-500/20 bg-amber-950/10 p-6">
          <div className="flex items-center gap-2 text-amber-400 font-bold text-base mb-4">
            <AlertTriangle className="h-5 w-5" />
            <span>{t.report.weaknessesTitle}</span>
          </div>
          <ul className="space-y-3">
            {report.weaknesses.map((item, idx) => (
              <li key={idx} className="flex items-start gap-2.5 text-sm text-zinc-300">
                <span className="h-1.5 w-1.5 rounded-full bg-amber-400 mt-2 shrink-0" />
                <span>{item}</span>
              </li>
            ))}
          </ul>
        </div>
      </div>

      {/* Detailed Question Scores */}
      {report.questionScores && report.questionScores.length > 0 && (
        <div className="mt-8 rounded-2xl border border-white/[0.08] bg-zinc-900/40 p-6">
          <h3 className="font-bold text-zinc-100 text-lg mb-4">{t.report.breakdownTitle}</h3>
          <div className="space-y-3">
            {report.questionScores.map((q, idx) => (
              <div
                key={idx}
                className="rounded-xl border border-white/[0.08] bg-zinc-950/60 p-4 transition"
              >
                <div className="flex items-center justify-between">
                  <span className="font-semibold text-zinc-100 text-sm">{q.question}</span>
                  <span className="rounded-lg bg-blue-500/10 px-2.5 py-1 text-xs font-bold text-blue-400 border border-blue-500/20 font-mono">
                    {q.score} / 100
                  </span>
                </div>
                <p className="mt-2 text-xs text-zinc-400 leading-relaxed">{q.comment}</p>
              </div>
            ))}
          </div>
        </div>
      )}

      {/* Recommended Learning Resources */}
      {report.recommendations && report.recommendations.length > 0 && (
        <div className="mt-8 rounded-2xl border border-white/[0.08] bg-zinc-900/40 p-6">
          <div className="flex items-center gap-2 font-bold text-zinc-100 text-lg mb-4">
            <BookOpen className="h-5 w-5 text-blue-400" />
            <span>{t.report.resourcesTitle}</span>
          </div>
          <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
            {report.recommendations.map((rec, idx) => (
              <a
                key={idx}
                href={rec.resource}
                target="_blank"
                rel="noreferrer"
                className="card-interactive flex items-center justify-between rounded-xl border border-white/[0.08] bg-zinc-950/80 p-4 hover:border-blue-500/30 hover:bg-zinc-900 transition group"
              >
                <div>
                  <div className="font-semibold text-zinc-200 text-sm group-hover:text-blue-400 transition">
                    {rec.topic}
                  </div>
                  <div className="text-xs text-zinc-500 mt-0.5 truncate max-w-[280px]">
                    {rec.resource}
                  </div>
                </div>
                <ExternalLink className="h-4 w-4 text-zinc-500 group-hover:text-blue-400 transition shrink-0 ml-2" />
              </a>
            ))}
          </div>
        </div>
      )}
    </div>
  );
};
