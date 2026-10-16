import React from 'react';
import { PerformanceReport } from '@/components/PerformanceReport';

interface PageProps {
  params: {
    id: string;
  };
}

export const metadata = {
  title: 'Звіт про співбесіду — Dev Dialogue',
  description: 'Аналіз відповідей, сильних сторін та зон росту кандидата.',
};

export default function ReportPage({ params }: PageProps) {
  return <PerformanceReport interviewId={params.id} />;
}
