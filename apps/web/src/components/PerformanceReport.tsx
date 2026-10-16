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

interface PerformanceReportProps {
  interviewId: string;
}

export const PerformanceReport: React.FC<PerformanceReportProps> = ({ interviewId }) => {
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

      // Default mock report
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
      setLoading(false);
    }

    loadReport();
  }, [interviewId]);

  if (loading || !report) {
    return (
      <div className="flex h-96 items-center justify-center">
        <div className="text-slate-400 text-sm">Генерація та завантаження звіту...</div>
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
      <div className="relative overflow-hidden rounded-3xl border border-slate-800 bg-gradient-to-b from-slate-900 to-slate-950 p-8 text-center shadow-2xl">
        <div className="inline-flex items-center gap-2 rounded-full bg-blue-500/10 px-3 py-1 text-xs font-semibold text-blue-400 border border-blue-500/20 mb-4">
          <Trophy className="h-3.5 w-3.5" />
          Підсумковий результат технічної співбесіди
        </div>

        <div className="flex items-center justify-center">
          <div className="relative flex h-36 w-36 items-center justify-center rounded-full border-4 border-slate-800 bg-slate-950 shadow-inner">
            <div className="text-center">
              <span className={`text-5xl font-black tracking-tight ${scoreColor}`}>
                {report.score}
              </span>
              <span className="block text-xs font-semibold uppercase tracking-wider text-slate-500">
                / 100 балів
              </span>
            </div>
          </div>
        </div>

        <p className="mt-6 max-w-2xl mx-auto text-base text-slate-300 leading-relaxed">
          {report.summary}
        </p>

        {/* Action Buttons */}
        <div className="mt-8 flex flex-wrap items-center justify-center gap-4">
          <Link
            href="/interview/setup"
            className="flex items-center gap-2 rounded-xl bg-blue-600 px-5 py-2.5 text-sm font-semibold text-white shadow-lg shadow-blue-500/20 hover:bg-blue-500 transition"
          >
            <RotateCcw className="h-4 w-4" />
            <span>Пройти ще раз</span>
          </Link>
          <Link
            href="/dashboard"
            className="flex items-center gap-2 rounded-xl border border-slate-700 bg-slate-800/80 px-5 py-2.5 text-sm font-semibold text-slate-200 hover:bg-slate-800 transition"
          >
            <LayoutDashboard className="h-4 w-4" />
            <span>До дашборду</span>
          </Link>
        </div>
      </div>

      {/* Strengths & Weaknesses Grid */}
      <div className="mt-8 grid grid-cols-1 gap-6 md:grid-cols-2">
        {/* Strengths */}
        <div className="rounded-2xl border border-emerald-500/20 bg-emerald-950/10 p-6">
          <div className="flex items-center gap-2 text-emerald-400 font-bold text-base mb-4">
            <CheckCircle2 className="h-5 w-5" />
            <span>Сильні сторони</span>
          </div>
          <ul className="space-y-3">
            {report.strengths.map((item, idx) => (
              <li key={idx} className="flex items-start gap-2.5 text-sm text-slate-300">
                <span className="h-1.5 w-1.5 rounded-full bg-emerald-400 mt-2 shrink-0" />
                <span>{item}</span>
              </li>
            ))}
          </ul>
        </div>

        {/* Growth Areas */}
        <div className="rounded-2xl border border-amber-500/20 bg-amber-950/10 p-6">
          <div className="flex items-center gap-2 text-amber-400 font-bold text-base mb-4">
            <AlertTriangle className="h-5 w-5" />
            <span>Зони росту (Weaknesses)</span>
          </div>
          <ul className="space-y-3">
            {report.weaknesses.map((item, idx) => (
              <li key={idx} className="flex items-start gap-2.5 text-sm text-slate-300">
                <span className="h-1.5 w-1.5 rounded-full bg-amber-400 mt-2 shrink-0" />
                <span>{item}</span>
              </li>
            ))}
          </ul>
        </div>
      </div>

      {/* Detailed Question Scores */}
      {report.questionScores?.length > 0 && (
        <div className="mt-8 rounded-2xl border border-slate-800 bg-slate-900/60 p-6">
          <h3 className="font-bold text-white text-lg mb-4">Детальний розбір запитань</h3>
          <div className="space-y-3">
            {report.questionScores.map((q, idx) => (
              <div
                key={idx}
                className="rounded-xl border border-slate-800 bg-slate-950/60 p-4 transition"
              >
                <div className="flex items-center justify-between">
                  <span className="font-semibold text-white text-sm">{q.question}</span>
                  <span className="rounded-lg bg-blue-500/10 px-2.5 py-1 text-xs font-bold text-blue-400 border border-blue-500/20">
                    {q.score} / 100
                  </span>
                </div>
                <p className="mt-2 text-xs text-slate-400 leading-relaxed">{q.comment}</p>
              </div>
            ))}
          </div>
        </div>
      )}

      {/* Recommended Learning Resources */}
      {report.recommendations?.length > 0 && (
        <div className="mt-8 rounded-2xl border border-slate-800 bg-slate-900/60 p-6">
          <div className="flex items-center gap-2 font-bold text-white text-lg mb-4">
            <BookOpen className="h-5 w-5 text-indigo-400" />
            <span>Рекомендовані матеріали для підготовки</span>
          </div>
          <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
            {report.recommendations.map((rec, idx) => (
              <a
                key={idx}
                href={rec.resource}
                target="_blank"
                rel="noreferrer"
                className="flex items-center justify-between rounded-xl border border-slate-800 bg-slate-950/80 p-4 hover:border-slate-700 hover:bg-slate-900 transition group"
              >
                <div>
                  <div className="font-semibold text-white text-sm group-hover:text-blue-400 transition">
                    {rec.topic}
                  </div>
                  <div className="text-xs text-slate-500 mt-0.5 truncate max-w-[280px]">
                    {rec.resource}
                  </div>
                </div>
                <ExternalLink className="h-4 w-4 text-slate-500 group-hover:text-blue-400 transition shrink-0 ml-2" />
              </a>
            ))}
          </div>
        </div>
      )}
    </div>
  );
};
