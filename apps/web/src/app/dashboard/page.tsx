import React from 'react';
import { DashboardView } from '@/components/DashboardView';

export const metadata = {
  title: 'Дашборд — Dev Dialogue',
  description: 'Центр підготовки до співбесід та історія результатів.',
};

export default function DashboardPage() {
  return <DashboardView />;
}
