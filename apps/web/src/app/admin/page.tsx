import React from 'react';
import { AdminPanel } from '@/components/AdminPanel';

export const metadata = {
  title: 'Банк питань (Адмін) — Dev Dialogue',
  description: 'Керування банком технічних запитань.',
};

export default function AdminPage() {
  return <AdminPanel />;
}
