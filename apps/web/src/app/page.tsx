'use client';

import React, { useState } from 'react';
import Link from 'next/link';
import {
  ArrowRight,
  CheckCircle2,
  Code2,
  Database,
  Server,
  Bot,
  User,
  Zap,
  Shield,
  Layers,
  ChevronRight,
  Sparkles,
} from 'lucide-react';
import { useTranslation } from '@/lib/useTranslation';

interface PreviewTrack {
  id: string;
  name: string;
  role: string;
  level: string;
  question: string;
  candidateAnswer: string;
  feedbackScore: number;
  highlight: string;
}

const PREVIEW_TRACKS_UK: PreviewTrack[] = [
  {
    id: 'frontend',
    name: 'Frontend',
    role: 'React / Next.js',
    level: 'Junior+',
    question:
      'Як працює Reconciliation у React, і чому не варто використовувати index масиву як key при динамічній зміні списків?',
    candidateAnswer:
      'React використовує Virtual DOM diffing. Якщо використати index як key при видаленні або сортуванні елементів, стан компонентів (input focus, локальний state) прив’яжеться до неправильного вузла в DOM.',
    feedbackScore: 94,
    highlight: 'Глибоке розуміння fiber architecture та алгоритму узгодження.',
  },
  {
    id: 'backend',
    name: 'Backend',
    role: 'C# .NET 9 / EF Core',
    level: 'Junior/Middle',
    question:
      'Поясніть різницю між AsNoTracking() та звичайним вибірковим запитом в Entity Framework Core. У яких сценаріях це критично?',
    candidateAnswer:
      'AsNoTracking вимикає механізм Change Tracker у контексті. Це суттєво економить пам’ять і прискорює запити на читання (read-only endpoints), але забороняє подальший SaveChanges без ручного attach.',
    feedbackScore: 91,
    highlight: 'Точне визначення поведінки памʼяті та сценаріїв оптимізації.',
  },
  {
    id: 'devops',
    name: 'DevOps',
    role: 'Docker / CI/CD',
    level: 'Junior',
    question:
      'Чому в Dockerfile для production-збірки рекомендується використовувати Multi-stage builds?',
    candidateAnswer:
      'Щоб відділити важкий SDK компіляції від середовища виконання. У фінальний образ потрапляють лише зібрані бінарники, що зменшує розмір контейнера з 800MB до 90MB і закриває вразливості SDK.',
    feedbackScore: 96,
    highlight: 'Відмінна аргументація з точки зору безпеки та ваги образів.',
  },
];

const PREVIEW_TRACKS_EN: PreviewTrack[] = [
  {
    id: 'frontend',
    name: 'Frontend',
    role: 'React / Next.js',
    level: 'Junior+',
    question:
      'How does Reconciliation work in React, and why should you avoid using array index as a key for dynamic lists?',
    candidateAnswer:
      'React relies on Virtual DOM diffing. If you use an array index as a key during element deletion or sorting, component state (input focus, local state) will attach to the wrong DOM node.',
    feedbackScore: 94,
    highlight: 'Deep understanding of fiber architecture and reconciliation algorithm.',
  },
  {
    id: 'backend',
    name: 'Backend',
    role: 'C# .NET 9 / EF Core',
    level: 'Junior/Middle',
    question:
      'Explain the difference between AsNoTracking() and standard queries in Entity Framework Core. When is this critical?',
    candidateAnswer:
      'AsNoTracking disables the Change Tracker within the DbContext. This saves memory and significantly speeds up read-only queries, but prevents subsequent SaveChanges without manual entity attachment.',
    feedbackScore: 91,
    highlight: 'Accurate evaluation of memory footprints and query optimization scenarios.',
  },
  {
    id: 'devops',
    name: 'DevOps',
    role: 'Docker / CI/CD',
    level: 'Junior',
    question: 'Why are Multi-stage builds recommended in Dockerfiles for production releases?',
    candidateAnswer:
      'To decouple the heavy build/compiler SDK from the runtime environment. Only the compiled binaries make it into the final image, shrinking container footprint from 800MB to 90MB while eliminating build-tool security vulnerabilities.',
    feedbackScore: 96,
    highlight: 'Excellent reasoning regarding container security and attack surface reduction.',
  },
];

export default function Home() {
  const { t, lang } = useTranslation();
  const tracks = lang === 'en' ? PREVIEW_TRACKS_EN : PREVIEW_TRACKS_UK;
  const [selectedTrackId, setSelectedTrackId] = useState<string>('frontend');

  const activeTrack = tracks.find((track) => track.id === selectedTrackId) || tracks[0];

  return (
    <div className="relative min-h-screen overflow-hidden bg-[#09090b] text-zinc-100 selection:bg-blue-500/25 selection:text-blue-200">
      {/* Subtle Technical Grid Mask */}
      <div
        className="pointer-events-none absolute inset-0 z-0 opacity-[0.04]"
        style={{
          backgroundImage: 'radial-gradient(rgba(255, 255, 255, 0.45) 1px, transparent 1px)',
          backgroundSize: '28px 28px',
        }}
      />

      {/* Dual Luminous Ambient Glows — Electric Blue + Soft Violet */}
      <div className="pointer-events-none absolute top-[-50px] left-[15%] w-[550px] h-[380px] bg-gradient-to-br from-blue-600/20 via-indigo-600/12 to-transparent blur-[140px]" />
      <div className="pointer-events-none absolute top-[20px] right-[10%] w-[500px] h-[360px] bg-gradient-to-bl from-blue-500/15 via-indigo-600/10 to-transparent blur-[150px]" />

      <div className="relative z-10 mx-auto max-w-7xl px-4 pt-14 pb-24 sm:px-6 lg:px-8">
        {/* Top Status Badge */}
        <div className="flex justify-center">
          <div className="inline-flex items-center gap-2 rounded-full border border-blue-500/30 bg-blue-500/10 px-4 py-1 text-xs font-medium text-blue-200 backdrop-blur-md shadow-sm shadow-blue-500/10 transition-all hover:border-blue-500/50">
            <span className="flex h-2 w-2 rounded-full bg-blue-400">
              <span className="h-2 w-2 rounded-full bg-blue-400 animate-ping opacity-75" />
            </span>
            <span>{t.home.olympiadBadge}</span>
            <span className="text-blue-400/50">•</span>
            <span className="font-mono text-[11px] text-blue-300">{t.home.liveBadge}</span>
          </div>
        </div>

        {/* Hero Headline & Subtitle */}
        <div className="mt-8 text-center max-w-3xl mx-auto animate-fade-in-up">
          <h1 className="text-4xl sm:text-6xl font-extrabold tracking-[-0.035em] text-zinc-100 leading-[1.08]">
            {t.home.heroTitle}{' '}
            <span className="text-transparent bg-clip-text bg-gradient-to-r from-blue-400 via-sky-300 to-indigo-400">
              {t.home.heroTitleAccent}
            </span>
          </h1>

          <p className="mt-6 text-base sm:text-lg text-zinc-300/90 leading-relaxed max-w-2xl mx-auto font-normal">
            {t.home.heroSubtitle}
          </p>

          {/* Primary Action Buttons */}
          <div className="mt-9 flex flex-wrap items-center justify-center gap-3.5">
            <Link
              href="/interview/setup"
              className="btn-tactile inline-flex items-center gap-2.5 rounded-xl bg-blue-600 hover:bg-blue-500 px-6 py-3.5 text-sm font-semibold text-white shadow-lg shadow-blue-600/25 border border-blue-400/30"
            >
              <Sparkles className="h-4 w-4 text-blue-100" />
              <span>{t.home.startInterview}</span>
              <ArrowRight className="h-4 w-4" />
            </Link>

            <Link
              href="/dashboard"
              className="btn-tactile inline-flex items-center gap-2 rounded-xl border border-white/[0.12] bg-zinc-900/80 px-5 py-3.5 text-sm font-medium text-zinc-200 hover:bg-zinc-800 hover:border-white/[0.22] hover:text-white"
            >
              <span>{t.home.viewDashboard}</span>
              <ChevronRight className="h-4 w-4 text-zinc-400" />
            </Link>
          </div>
        </div>

        {/* Interactive Live Simulation Terminal */}
        <div className="mt-16 mx-auto max-w-4xl">
          <div className="rounded-2xl border border-white/[0.1] bg-zinc-950/90 shadow-2xl shadow-black/90 backdrop-blur-xl overflow-hidden hover:border-blue-500/30 transition-all duration-300">
            {/* Terminal Top Window Bar */}
            <div className="flex flex-wrap items-center justify-between border-b border-white/[0.08] bg-zinc-900/50 px-4 py-3 gap-2">
              <div className="flex items-center gap-2">
                <div className="h-3 w-3 rounded-full bg-rose-500/70" />
                <div className="h-3 w-3 rounded-full bg-amber-500/70" />
                <div className="h-3 w-3 rounded-full bg-emerald-500/70" />
                <span className="ml-2 font-mono text-xs text-zinc-300">{t.home.terminalTitle}</span>
              </div>

              {/* Track Selection Tabs */}
              <div className="flex items-center gap-1 bg-zinc-900/90 p-0.5 rounded-lg border border-white/[0.08]">
                {tracks.map((track) => (
                  <button
                    key={track.id}
                    onClick={() => setSelectedTrackId(track.id)}
                    className={`btn-tactile rounded-md px-3 py-1 text-xs font-medium ${
                      activeTrack.id === track.id
                        ? 'bg-blue-600 text-white shadow-sm'
                        : 'text-zinc-400 hover:text-zinc-200'
                    }`}
                  >
                    {track.name}
                  </button>
                ))}
              </div>
            </div>

            {/* Conversation Flow Showcase */}
            <div className="p-6 space-y-5">
              {/* Interviewer Message */}
              <div className="flex items-start gap-3">
                <div className="flex h-8 w-8 shrink-0 items-center justify-center rounded-lg bg-rose-500/10 border border-rose-500/30 text-rose-300">
                  <Bot className="h-4 w-4" />
                </div>
                <div className="flex-1 space-y-1.5">
                  <div className="flex items-center gap-2">
                    <span className="text-xs font-semibold text-zinc-200">
                      {t.home.aiInterviewer}
                    </span>
                    <span className="rounded bg-rose-500/15 px-2 py-0.5 font-mono text-[10px] text-rose-300 border border-rose-500/30">
                      {activeTrack.role} • {activeTrack.level}
                    </span>
                  </div>
                  <p className="rounded-xl border border-white/[0.08] bg-zinc-900/60 p-4 text-sm text-zinc-100 leading-relaxed shadow-sm">
                    {activeTrack.question}
                  </p>
                </div>
              </div>

              {/* Candidate Answer */}
              <div className="flex items-start gap-3">
                <div className="flex h-8 w-8 shrink-0 items-center justify-center rounded-lg bg-zinc-900 border border-white/[0.08] text-zinc-400">
                  <User className="h-4 w-4 text-blue-400" />
                </div>
                <div className="flex-1 space-y-1.5">
                  <div className="flex items-center gap-2">
                    <span className="text-xs font-semibold text-zinc-300">
                      {t.home.candidateAnswer}
                    </span>
                    <span className="font-mono text-[10px] text-zinc-500">{t.home.voiceText}</span>
                  </div>
                  <p className="rounded-xl border border-white/[0.06] bg-zinc-900/40 p-4 text-sm text-zinc-200 leading-relaxed font-sans">
                    {activeTrack.candidateAnswer}
                  </p>
                </div>
              </div>

              {/* Dynamic Live AI Scorecard Pill with Raspberry Glow */}
              <div className="rounded-xl border border-rose-500/30 bg-gradient-to-r from-rose-950/30 via-zinc-900/50 to-emerald-950/20 p-4 flex flex-col sm:flex-row sm:items-center justify-between gap-3 shadow-inner">
                <div className="flex items-center gap-3">
                  <div className="flex h-10 w-10 shrink-0 items-center justify-center rounded-lg bg-rose-500/20 border border-rose-500/40 text-rose-300 font-mono font-bold text-sm shadow-sm">
                    {activeTrack.feedbackScore}
                  </div>
                  <div>
                    <div className="text-xs font-medium text-rose-200 flex items-center gap-1.5">
                      <CheckCircle2 className="h-3.5 w-3.5 text-rose-400" />
                      <span>{t.home.instantAnalysis}</span>
                    </div>
                    <p className="text-xs text-zinc-300/80 mt-0.5">{activeTrack.highlight}</p>
                  </div>
                </div>

                <Link
                  href="/interview/setup"
                  className="inline-flex items-center justify-center gap-1.5 rounded-lg bg-blue-600 hover:bg-blue-500 border border-blue-400/30 px-3.5 py-1.5 text-xs font-semibold text-white shadow-sm shadow-blue-500/20 transition-all whitespace-nowrap"
                >
                  <span>{t.home.takeFullTest}</span>
                  <ArrowRight className="h-3.5 w-3.5" />
                </Link>
              </div>
            </div>
          </div>
        </div>

        {/* Metrics High-Density Strip */}
        <div className="mt-16 grid grid-cols-2 gap-4 sm:grid-cols-4 border-y border-white/[0.08] py-8 text-center bg-zinc-900/20 rounded-xl">
          <div>
            <div className="font-mono text-2xl sm:text-3xl font-bold tracking-tight text-zinc-100">
              5
            </div>
            <div className="mt-1 text-xs text-zinc-400 font-medium">{t.home.metrics.tracks}</div>
          </div>
          <div>
            <div className="font-mono text-2xl sm:text-3xl font-bold tracking-tight text-zinc-100">
              {t.home.metrics.levels}
            </div>
            <div className="mt-1 text-xs text-zinc-400 font-medium">
              {t.home.metrics.levelsDesc}
            </div>
          </div>
          <div>
            <div className="font-mono text-2xl sm:text-3xl font-bold tracking-tight text-blue-400">
              &lt; 1.8s
            </div>
            <div className="mt-1 text-xs text-zinc-400 font-medium">{t.home.metrics.latency}</div>
          </div>
          <div>
            <div className="font-mono text-2xl sm:text-3xl font-bold tracking-tight text-emerald-400">
              100%
            </div>
            <div className="mt-1 text-xs text-zinc-400 font-medium">{t.home.metrics.contracts}</div>
          </div>
        </div>

        {/* Specialized Career Tracks Bento */}
        <div className="mt-20">
          <div className="text-center max-w-2xl mx-auto mb-12">
            <h2 className="text-2xl sm:text-3xl font-bold tracking-tight text-zinc-100">
              {t.home.tracksTitle}
            </h2>
            <p className="mt-2 text-sm text-zinc-400">{t.home.tracksSubtitle}</p>
          </div>

          <div className="grid grid-cols-1 md:grid-cols-3 gap-5">
            {/* Frontend */}
            <div className="card-interactive group rounded-2xl border border-white/[0.08] bg-zinc-900/40 p-6 hover:border-blue-500/40 hover:bg-zinc-900/70 hover:shadow-lg hover:shadow-blue-500/5">
              <div className="flex h-10 w-10 items-center justify-center rounded-xl bg-blue-500/10 border border-blue-500/30 text-blue-400 mb-4 group-hover:scale-105 transition-transform duration-200">
                <Code2 className="h-5 w-5" />
              </div>
              <h3 className="text-base font-semibold text-zinc-100 group-hover:text-blue-300 transition-colors">
                {t.home.frontendTitle}
              </h3>
              <p className="mt-2 text-xs text-zinc-400 leading-relaxed">{t.home.frontendDesc}</p>
              <div className="mt-4 flex items-center gap-2 font-mono text-[11px] text-zinc-400">
                <span className="rounded bg-zinc-800/80 px-2 py-0.5 border border-white/[0.06]">
                  Junior
                </span>
                <span className="rounded bg-zinc-800/80 px-2 py-0.5 border border-white/[0.06]">
                  Middle
                </span>
              </div>
            </div>

            {/* Backend */}
            <div className="card-interactive group rounded-2xl border border-white/[0.08] bg-zinc-900/40 p-6 hover:border-blue-500/40 hover:bg-zinc-900/70 hover:shadow-lg hover:shadow-blue-500/5">
              <div className="flex h-10 w-10 items-center justify-center rounded-xl bg-blue-500/10 border border-blue-500/30 text-blue-400 mb-4 group-hover:scale-105 transition-transform duration-200">
                <Database className="h-5 w-5" />
              </div>
              <h3 className="text-base font-semibold text-zinc-100 group-hover:text-blue-300 transition-colors">
                {t.home.backendTitle}
              </h3>
              <p className="mt-2 text-xs text-zinc-400 leading-relaxed">{t.home.backendDesc}</p>
              <div className="mt-4 flex items-center gap-2 font-mono text-[11px] text-zinc-400">
                <span className="rounded bg-zinc-800/80 px-2 py-0.5 border border-white/[0.06]">
                  Trainee
                </span>
                <span className="rounded bg-zinc-800/80 px-2 py-0.5 border border-white/[0.06]">
                  Junior
                </span>
                <span className="rounded bg-zinc-800/80 px-2 py-0.5 border border-white/[0.06]">
                  Middle
                </span>
              </div>
            </div>

            {/* DevOps & Infra */}
            <div className="card-interactive group rounded-2xl border border-white/[0.08] bg-zinc-900/40 p-6 hover:border-emerald-500/40 hover:bg-zinc-900/70 hover:shadow-lg hover:shadow-emerald-500/5">
              <div className="flex h-10 w-10 items-center justify-center rounded-xl bg-emerald-500/10 border border-emerald-500/30 text-emerald-400 mb-4 group-hover:scale-105 transition-transform duration-200">
                <Server className="h-5 w-5" />
              </div>
              <h3 className="text-base font-semibold text-zinc-100 group-hover:text-emerald-300 transition-colors">
                {t.home.devopsTitle}
              </h3>
              <p className="mt-2 text-xs text-zinc-400 leading-relaxed">{t.home.devopsDesc}</p>
              <div className="mt-4 flex items-center gap-2 font-mono text-[11px] text-zinc-400">
                <span className="rounded bg-zinc-800/80 px-2 py-0.5 border border-white/[0.06]">
                  Junior
                </span>
                <span className="rounded bg-zinc-800/80 px-2 py-0.5 border border-white/[0.06]">
                  Docker
                </span>
              </div>
            </div>
          </div>
        </div>

        {/* Engineering Architecture & Security Card */}
        <div className="mt-20 rounded-2xl border border-white/[0.1] bg-zinc-950/80 p-8 backdrop-blur-md shadow-lg">
          <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
            <div className="flex items-start gap-4">
              <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-lg bg-zinc-800 text-zinc-200 border border-white/[0.1]">
                <Layers className="h-4 w-4 text-blue-400" />
              </div>
              <div>
                <h4 className="text-sm font-semibold text-zinc-100">{t.home.archTurborepoTitle}</h4>
                <p className="mt-1 text-xs text-zinc-400 leading-relaxed">
                  {t.home.archTurborepoDesc}
                </p>
              </div>
            </div>

            <div className="flex items-start gap-4">
              <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-lg bg-zinc-800 text-zinc-200 border border-white/[0.1]">
                <Zap className="h-4 w-4 text-amber-400" />
              </div>
              <div>
                <h4 className="text-sm font-semibold text-zinc-100">{t.home.archMockTitle}</h4>
                <p className="mt-1 text-xs text-zinc-400 leading-relaxed">{t.home.archMockDesc}</p>
              </div>
            </div>

            <div className="flex items-start gap-4">
              <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-lg bg-zinc-800 text-zinc-200 border border-white/[0.1]">
                <Shield className="h-4 w-4 text-blue-400" />
              </div>
              <div>
                <h4 className="text-sm font-semibold text-zinc-100">{t.home.archRbacTitle}</h4>
                <p className="mt-1 text-xs text-zinc-400 leading-relaxed">{t.home.archRbacDesc}</p>
              </div>
            </div>
          </div>
        </div>

        {/* Final CTA */}
        <div className="mt-16 text-center">
          <Link
            href="/interview/setup"
            className="btn-tactile inline-flex items-center gap-2.5 rounded-xl bg-blue-600 hover:bg-blue-500 px-8 py-4 text-sm font-bold text-white shadow-xl shadow-blue-600/25 border border-blue-400/30"
          >
            <Sparkles className="h-4 w-4 text-blue-100" />
            <span>{t.home.finalCta}</span>
            <ArrowRight className="h-4 w-4" />
          </Link>
        </div>
      </div>
    </div>
  );
}
