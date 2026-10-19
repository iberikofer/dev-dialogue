'use client';

import React, { useState } from 'react';
import { useRouter } from 'next/navigation';
import {
  Code2,
  Server,
  CloudCog,
  CheckCircle2,
  Briefcase,
  Sparkles,
  ArrowRight,
  LucideIcon,
} from 'lucide-react';
import { InterviewDirection, InterviewLevel, InterviewLanguage } from '@dev-dialogue/api-contracts';
import { api } from '@/lib/api-client';

const DIRECTIONS: { id: InterviewDirection; title: string; desc: string; icon: LucideIcon }[] = [
  {
    id: 'frontend',
    title: 'Frontend Development',
    desc: 'React, Next.js, TypeScript, Browser APIs, CSS Architecture & Web Vitals',
    icon: Code2,
  },
  {
    id: 'backend',
    title: 'Backend Engineering',
    desc: 'ASP.NET Core, C#, Databases, REST/gRPC, Async, Systems Architecture',
    icon: Server,
  },
  {
    id: 'devops',
    title: 'DevOps & Cloud',
    desc: 'Docker, CI/CD pipelines, Kubernetes, Monitoring, Cloud Infrastructure',
    icon: CloudCog,
  },
  {
    id: 'qa',
    title: 'Quality Assurance',
    desc: 'Test Design, Automation, Smoke/Regression strategies, API Testing',
    icon: CheckCircle2,
  },
  {
    id: 'pm',
    title: 'Project Management',
    desc: 'Agile/Scrum, Backlog prioritization, Risk mitigation, Team dynamics',
    icon: Briefcase,
  },
];

const LEVELS: { id: InterviewLevel; title: string; subtitle: string }[] = [
  {
    id: 'trainee',
    title: 'Trainee / Intern',
    subtitle: 'Базові концепції, синтаксис та алгоритми',
  },
  {
    id: 'junior',
    title: 'Junior Engineer',
    subtitle: 'Практичний досвід, веб-стандарти та робота з API',
  },
  {
    id: 'middle',
    title: 'Middle Engineer',
    subtitle: 'Архітектура, оптимізація, глибинні механізми',
  },
];

export const InterviewSetup: React.FC = () => {
  const router = useRouter();
  const [direction, setDirection] = useState<InterviewDirection>('frontend');
  const [level, setLevel] = useState<InterviewLevel>('junior');
  const [language, setLanguage] = useState<InterviewLanguage>('uk');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const handleStart = async () => {
    setLoading(true);
    setError(null);

    try {
      const response = await api.interviews.create({
        direction,
        level,
        language,
      });

      router.push(`/interview/${response.interviewId}`);
    } catch (err: unknown) {
      console.error('Failed to create interview:', err);
      const fallbackId = 'session-' + Math.random().toString(36).substring(2, 9);
      router.push(`/interview/${fallbackId}?dir=${direction}&lvl=${level}&lang=${language}`);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="mx-auto max-w-4xl px-4 py-12">
      {/* Header */}
      <div className="mb-10 text-center">
        <div className="inline-flex items-center gap-2 rounded-full bg-blue-500/10 px-3.5 py-1 text-xs font-medium text-blue-300 border border-blue-500/30 mb-3 shadow-sm shadow-blue-500/10">
          <Sparkles className="h-3.5 w-3.5 text-blue-400" />
          Конфігурація технічної сесії
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold tracking-[-0.03em] text-zinc-100">
          Налаштування співбесіди
        </h1>
        <p className="mt-2 text-sm sm:text-base text-zinc-400 max-w-xl mx-auto">
          Оберіть технологічний напрямок, бажаний рівень складності та мову для AI-інтерв&apos;юера.
        </p>
      </div>

      {error && (
        <div className="mb-6 rounded-xl border border-red-500/30 bg-red-950/20 p-4 text-sm text-red-300">
          {error}
        </div>
      )}

      <div className="space-y-9">
        {/* Step 1: Direction */}
        <div>
          <label className="block text-xs font-semibold uppercase tracking-wider text-zinc-400 mb-3 font-mono">
            1. Напрямок (Career Track)
          </label>
          <div className="grid grid-cols-1 gap-3 sm:grid-cols-2 lg:grid-cols-3">
            {DIRECTIONS.map((item) => {
              const Icon = item.icon;
              const isSelected = direction === item.id;
              return (
                <button
                  key={item.id}
                  type="button"
                  onClick={() => setDirection(item.id)}
                  className={`card-interactive flex flex-col text-left rounded-2xl p-4 border ${
                    isSelected
                      ? 'border-blue-500/70 bg-blue-950/25 shadow-lg shadow-blue-500/10 ring-1 ring-blue-500/50'
                      : 'border-white/[0.08] bg-zinc-900/40 hover:border-white/[0.16] hover:bg-zinc-900/70'
                  }`}
                >
                  <div className="flex items-center justify-between mb-3">
                    <div
                      className={`flex h-10 w-10 items-center justify-center rounded-xl transition-colors ${
                        isSelected
                          ? 'bg-blue-600 text-white shadow-sm shadow-blue-500/30'
                          : 'bg-zinc-800 text-zinc-400 border border-white/[0.06]'
                      }`}
                    >
                      <Icon className="h-5 w-5" />
                    </div>
                  </div>
                  <div className="font-semibold text-zinc-100 text-sm">{item.title}</div>
                  <div className="text-xs text-zinc-400 mt-1 line-clamp-2 leading-relaxed">
                    {item.desc}
                  </div>
                </button>
              );
            })}
          </div>
        </div>

        {/* Step 2: Level */}
        <div>
          <label className="block text-xs font-semibold uppercase tracking-wider text-zinc-400 mb-3 font-mono">
            2. Рівень складності (Seniority Level)
          </label>
          <div className="grid grid-cols-1 gap-3 sm:grid-cols-3">
            {LEVELS.map((item) => {
              const isSelected = level === item.id;
              return (
                <button
                  key={item.id}
                  type="button"
                  onClick={() => setLevel(item.id)}
                  className={`card-interactive rounded-2xl p-4 text-left border ${
                    isSelected
                      ? 'border-blue-500/70 bg-blue-950/25 shadow-md shadow-blue-500/10 ring-1 ring-blue-500/50'
                      : 'border-white/[0.08] bg-zinc-900/40 hover:border-white/[0.16] hover:bg-zinc-900/70'
                  }`}
                >
                  <div className="font-semibold text-zinc-100 text-sm">{item.title}</div>
                  <div className="text-xs text-zinc-400 mt-1 leading-normal">{item.subtitle}</div>
                </button>
              );
            })}
          </div>
        </div>

        {/* Step 3: Language */}
        <div>
          <label className="block text-xs font-semibold uppercase tracking-wider text-zinc-400 mb-3 font-mono">
            3. Мова співбесіди
          </label>
          <div className="grid grid-cols-2 gap-3 max-w-md">
            <button
              type="button"
              onClick={() => setLanguage('uk')}
              className={`btn-tactile flex items-center gap-3 rounded-2xl p-4 border ${
                language === 'uk'
                  ? 'border-blue-500/60 bg-blue-950/25 text-white ring-1 ring-blue-500/50 shadow-sm shadow-blue-500/10'
                  : 'border-white/[0.08] bg-zinc-900/40 text-zinc-400 hover:border-white/[0.16]'
              }`}
            >
              <span className="text-2xl">🇺🇦</span>
              <div className="text-left">
                <div className="font-semibold text-zinc-100 text-sm">Українська</div>
                <div className="text-[11px] text-zinc-400">Рідна мова</div>
              </div>
            </button>

            <button
              type="button"
              onClick={() => setLanguage('en')}
              className={`btn-tactile flex items-center gap-3 rounded-2xl p-4 border ${
                language === 'en'
                  ? 'border-blue-500/60 bg-blue-950/25 text-white ring-1 ring-blue-500/50 shadow-sm shadow-blue-500/10'
                  : 'border-white/[0.08] bg-zinc-900/40 text-zinc-400 hover:border-white/[0.16]'
              }`}
            >
              <span className="text-2xl">🇬🇧</span>
              <div className="text-left">
                <div className="font-semibold text-zinc-100 text-sm">English</div>
                <div className="text-[11px] text-zinc-400">International interview</div>
              </div>
            </button>
          </div>
        </div>

        {/* CTA Launch */}
        <div className="pt-6 border-t border-white/[0.08] flex flex-col sm:flex-row sm:items-center justify-between gap-4">
          <div className="text-xs text-zinc-400 font-mono">
            Сесія містить 5–7 запитань з детальним аналізом та генерацією скоркарту.
          </div>
          <button
            onClick={handleStart}
            disabled={loading}
            className="btn-tactile inline-flex items-center justify-center gap-2 rounded-xl bg-blue-600 hover:bg-blue-500 px-6 py-3.5 text-sm font-semibold text-white shadow-lg shadow-blue-600/25 border border-blue-400/30 disabled:opacity-50"
          >
            {loading ? (
              <span>Ініціалізація AI...</span>
            ) : (
              <>
                <span>Почати співбесіду</span>
                <ArrowRight className="h-4 w-4" />
              </>
            )}
          </button>
        </div>
      </div>
    </div>
  );
};
