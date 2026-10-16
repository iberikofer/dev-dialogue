import React from 'react';
import { InterviewSetup } from '@/components/InterviewSetup';

export const metadata = {
  title: 'Налаштування співбесіди — Dev Dialogue',
  description: 'Оберіть напрямок, рівень та мову технічної співбесіди.',
};

export default function SetupPage() {
  return <InterviewSetup />;
}
