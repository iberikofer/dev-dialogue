'use client';

import React from 'react';
import { usePathname } from 'next/navigation';
import { useTranslation } from '@/lib/useTranslation';

export const Footer: React.FC = () => {
  const { lang } = useTranslation();
  const pathname = usePathname();

  // Hide global footer on active interview sessions to give full viewport to the 3D room
  const isInterviewSession =
    pathname?.startsWith('/interview/') &&
    pathname !== '/interview/setup' &&
    !pathname?.endsWith('/report');

  if (isInterviewSession) {
    return null;
  }

  return (
    <footer className="border-t border-white/[0.08] bg-zinc-950/80 py-6 text-center text-xs text-zinc-500 font-mono">
      Dev Dialogue &copy; 2026 &middot;{' '}
      {lang === 'uk'
        ? 'Платформа розроблена для олімпіади ВНТУ ІПЗ ФІТКІ'
        : 'Platform engineered for VNTU Software Engineering Olympiad'}
    </footer>
  );
};
