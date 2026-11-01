'use client';

import React, { useEffect, useState } from 'react';
import { useBackendStatusStore } from '@/store/backendStatusStore';
import { useTranslation } from '@/lib/useTranslation';
import { ServerOff, RefreshCw, X } from 'lucide-react';

export const BackendStatusBanner: React.FC = () => {
  const { status, isDismissed, checkHealth, dismissBanner } = useBackendStatusStore();
  const { t } = useTranslation();
  const [mounted, setMounted] = useState(false);
  const [isChecking, setIsChecking] = useState(false);

  // 1. Initial health check on mount only
  useEffect(() => {
    setMounted(true);
    checkHealth();
  }, [checkHealth]);

  // 2. Periodic polling based on connection status without immediate synchronous re-trigger
  useEffect(() => {
    const intervalMs = status === 'online' ? 60000 : 15000;
    const interval = setInterval(() => {
      checkHealth();
    }, intervalMs);

    return () => clearInterval(interval);
  }, [checkHealth, status]);

  const handleRetry = async () => {
    setIsChecking(true);
    try {
      await checkHealth();
    } finally {
      setIsChecking(false);
    }
  };

  if (!mounted || status !== 'offline' || isDismissed) {
    return null;
  }

  return (
    <div
      role="alert"
      aria-live="assertive"
      className="relative z-40 w-full border-b border-amber-500/25 bg-gradient-to-r from-amber-950/80 via-zinc-950/95 to-amber-950/80 text-zinc-200 backdrop-blur-md transition-all duration-300"
    >
      <div className="mx-auto flex max-w-7xl items-center justify-between gap-3 px-4 py-2.5 sm:px-6 lg:px-8">
        <div className="flex items-center gap-3">
          {/* Animated radar status dot */}
          <span className="relative flex h-2.5 w-2.5 shrink-0">
            <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-amber-400 opacity-75"></span>
            <span className="relative inline-flex h-2.5 w-2.5 rounded-full bg-amber-500"></span>
          </span>

          <ServerOff className="h-4 w-4 shrink-0 text-amber-400" />

          <div className="flex flex-wrap items-center gap-x-2 gap-y-0.5 text-xs">
            <span className="font-semibold text-amber-300 tracking-wide">
              {t.backendStatus.offlineTitle}:
            </span>
            <span className="text-zinc-300">
              {t.backendStatus.offlineDesc}
            </span>
          </div>
        </div>

        {/* Action Controls */}
        <div className="flex items-center gap-2 shrink-0">
          <button
            type="button"
            onClick={handleRetry}
            disabled={isChecking}
            className="flex items-center gap-1.5 rounded-lg border border-amber-500/30 bg-amber-500/10 px-2.5 py-1 text-xs font-medium text-amber-200 transition-all hover:bg-amber-500/20 hover:border-amber-500/50 hover:scale-[1.02] active:scale-[0.98] disabled:opacity-60 disabled:cursor-not-allowed"
            title={t.backendStatus.recheck}
          >
            <RefreshCw className={`h-3 w-3 ${isChecking ? 'animate-spin' : ''}`} />
            <span className="hidden sm:inline">
              {isChecking ? t.backendStatus.checking : t.backendStatus.recheck}
            </span>
          </button>

          <button
            type="button"
            onClick={dismissBanner}
            aria-label="Приховати сповіщення"
            className="rounded-lg p-1 text-zinc-400 hover:bg-zinc-800/80 hover:text-zinc-200 transition-colors"
          >
            <X className="h-3.5 w-3.5" />
          </button>
        </div>
      </div>
    </div>
  );
};
