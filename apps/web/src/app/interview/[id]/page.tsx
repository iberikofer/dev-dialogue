import React from 'react';
import { ChatRoom } from '@/components/ChatRoom';

interface PageProps {
  params: {
    id: string;
  };
  searchParams?: {
    dir?: string;
    lvl?: string;
    lang?: string;
  };
}

export const metadata = {
  title: 'Кімната співбесіди — Dev Dialogue',
  description: 'Інтерактивна сесія технічної співбесіди з AI.',
};

export default function InterviewRoomPage({ params, searchParams }: PageProps) {
  return (
    <ChatRoom
      interviewId={params.id}
      initialDirection={searchParams?.dir || 'Frontend'}
      initialLevel={searchParams?.lvl || 'Junior'}
      initialLanguage={searchParams?.lang || 'uk'}
    />
  );
}
