'use client';

import React, { useState, useEffect, useRef } from 'react';
import { useRouter } from 'next/navigation';
import { Send, Bot, User, CheckCircle, Sparkles, AlertCircle, Clock } from 'lucide-react';
import { InterviewMessage } from '@dev-dialogue/api-contracts';
import { api } from '@/lib/api-client';
import { useTranslation } from '@/lib/useTranslation';

interface ChatRoomProps {
  interviewId: string;
  initialDirection?: string;
  initialLevel?: string;
  initialLanguage?: string;
}

export const ChatRoom: React.FC<ChatRoomProps> = ({
  interviewId,
  initialDirection = 'Frontend',
  initialLevel = 'Junior',
  initialLanguage,
}) => {
  const router = useRouter();
  const { t, lang } = useTranslation();
  const sessionLanguage = initialLanguage || lang;

  const [messages, setMessages] = useState<InterviewMessage[]>([]);
  const [input, setInput] = useState('');
  const [loading, setLoading] = useState(false);
  const [completing, setCompleting] = useState(false);
  const [error] = useState<string | null>(null);
  const scrollRef = useRef<HTMLDivElement>(null);

  // Load existing messages or initialize first message
  useEffect(() => {
    let isMounted = true;

    async function loadData() {
      try {
        const data = await api.interviews.getById(interviewId);
        if (isMounted && data.transcript?.length > 0) {
          setMessages(data.transcript);
          return;
        }
      } catch {
        // Fallback demo initial message if running in offline mode
      }

      if (isMounted) {
        setMessages((prev) => {
          if (prev.length > 0) return prev;
          return [
            {
              role: 'assistant',
              content:
                sessionLanguage === 'uk'
                  ? `Вітаю на технічній співбесіді за напрямком ${initialDirection} (${initialLevel})! Я ваш AI-інтерв'юер. Давайте розпочнемо. Розкажіть коротко про себе та ваш практичний досвід у цій сфері.`
                  : `Welcome to your ${initialDirection} (${initialLevel}) technical interview! I am your AI interviewer. Let's begin. Could you please briefly introduce yourself and share your relevant background?`,
              timestamp: new Date().toISOString(),
            },
          ];
        });
      }
    }

    loadData();

    return () => {
      isMounted = false;
    };
  }, [interviewId, initialDirection, initialLevel, sessionLanguage]);

  // Scroll to bottom on new messages
  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  }, [messages, loading]);

  const handleSend = async (e?: React.FormEvent) => {
    if (e) e.preventDefault();
    if (!input.trim() || loading || completing) return;

    const userText = input.trim();
    setInput('');

    const userMessage: InterviewMessage = {
      role: 'user',
      content: userText,
      timestamp: new Date().toISOString(),
    };

    setMessages((prev) => [...prev, userMessage]);
    setLoading(true);

    try {
      const response = await api.interviews.sendMessage(interviewId, userText);

      if (response && response.message) {
        setMessages((prev) => [
          ...prev,
          {
            role: 'assistant',
            content: response.message,
            timestamp: new Date().toISOString(),
          },
        ]);
      }
    } catch {
      // Fallback response for offline prototype demo
      setTimeout(() => {
        const fallbackReplies =
          sessionLanguage === 'uk'
            ? [
                'Дякую за відповідь! Скажіть, будь ласка, як саме ви оптимізуєте продуктивність та уникаєте зайвих ререндерів у ваших компонентах?',
                'Чудово. А які патерни обробки помилок та асинхронних запитів ви зазвичай використовуєте на практиці?',
                'Цікавий підхід. Чи доводилося вам налаштовувати CI/CD або працювати з Docker контейнерами для розгортання?',
                'Дякую за ґрунтовні відповіді! Усі питання цієї сесії вичерпано. Тепер ви можете натиснути кнопку завершення, щоб отримати детальний скоркарт.',
              ]
            : [
                'Thank you for your answer! Could you elaborate on how you optimize performance and prevent unnecessary re-renders in your code?',
                'Great point. What patterns do you typically apply for handling async states and API error boundaries in production?',
                'Interesting approach. Have you had hands-on experience configuring Docker or CI/CD pipelines?',
                'Thank you for articulating your reasoning. All questions for this session are complete! You can now finish the interview to view your full scorecard.',
              ];

        const userCount = messages.filter((m) => m.role === 'user').length;
        const nextReply = fallbackReplies[Math.min(userCount, fallbackReplies.length - 1)];

        setMessages((prev) => [
          ...prev,
          {
            role: 'assistant',
            content: nextReply,
            timestamp: new Date().toISOString(),
          },
        ]);
        setLoading(false);
      }, 900);
      return;
    } finally {
      setLoading(false);
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  };

  const handleComplete = async () => {
    setCompleting(true);

    try {
      await api.interviews.complete(interviewId);
      router.push(`/interview/${interviewId}/report`);
    } catch {
      router.push(`/interview/${interviewId}/report`);
    } finally {
      setCompleting(false);
    }
  };

  const userAnswersCount = messages.filter((m) => m.role === 'user').length;

  return (
    <div className="flex flex-col h-[calc(100vh-4rem)] max-w-5xl mx-auto px-4 py-4 font-sans">
      {/* Top Session Info Bar */}
      <div className="flex items-center justify-between border-b border-white/[0.08] pb-3 mb-3">
        <div className="flex items-center gap-3">
          <div className="flex h-10 w-10 items-center justify-center rounded-xl bg-gradient-to-tr from-rose-500/20 via-indigo-500/10 to-blue-500/20 text-rose-300 border border-rose-500/30 shadow-sm">
            <Bot className="h-5 w-5" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h2 className="text-sm sm:text-base font-semibold text-zinc-100">{t.chat.title}</h2>
              <span className="rounded-md bg-rose-500/15 px-2 py-0.5 text-[11px] font-mono font-medium text-rose-300 border border-rose-500/30 uppercase">
                {initialDirection}
              </span>
              <span className="rounded-md bg-zinc-800 px-2 py-0.5 text-[11px] font-mono text-zinc-300 border border-white/[0.06]">
                {initialLevel}
              </span>
            </div>
            <div className="flex items-center gap-2 text-xs text-zinc-400 mt-0.5 font-mono">
              <Clock className="h-3.5 w-3.5 text-zinc-500" />
              <span>
                {t.chat.candidateAnswers} {userAnswersCount}
              </span>
            </div>
          </div>
        </div>

        {/* Complete Session Action Button */}
        <button
          onClick={handleComplete}
          disabled={completing || userAnswersCount === 0}
          className="btn-tactile flex items-center gap-2 rounded-xl bg-gradient-to-r from-emerald-600 to-teal-600 px-4 py-2 text-xs font-semibold text-white shadow-md shadow-emerald-600/20 hover:from-emerald-500 hover:to-teal-500 disabled:opacity-40 transition-all"
        >
          {completing ? (
            <span>{t.chat.generatingReport}</span>
          ) : (
            <>
              <CheckCircle className="h-4 w-4" />
              <span>{t.chat.finishBtn}</span>
            </>
          )}
        </button>
      </div>

      {error && (
        <div className="mb-3 flex items-center gap-2 rounded-xl bg-red-950/20 border border-red-500/30 p-3 text-xs text-red-300">
          <AlertCircle className="h-4 w-4 shrink-0" />
          <span>{error}</span>
        </div>
      )}

      {/* Messages Scroll Area */}
      <div
        ref={scrollRef}
        className="flex-1 overflow-y-auto space-y-4 pr-2 rounded-2xl bg-zinc-950/70 border border-white/[0.08] p-5 shadow-inner"
      >
        {messages.map((msg, index) => {
          const isAssistant = msg.role === 'assistant';
          return (
            <div
              key={index}
              className={`flex items-start gap-3 ${isAssistant ? 'justify-start' : 'justify-end'}`}
            >
              {isAssistant && (
                <div className="flex h-8 w-8 shrink-0 items-center justify-center rounded-xl bg-zinc-900 border border-white/[0.08] text-rose-300 shadow-sm">
                  <Bot className="h-4 w-4" />
                </div>
              )}

              <div
                className={`max-w-[82%] rounded-2xl px-4 py-3 text-sm leading-relaxed ${
                  isAssistant
                    ? 'bg-zinc-900/90 text-zinc-200 border border-white/[0.08] shadow-sm'
                    : 'bg-gradient-to-r from-rose-500 via-pink-500 to-indigo-600 text-white shadow-md shadow-rose-500/15'
                }`}
              >
                <div className="whitespace-pre-wrap">{msg.content}</div>
                <div
                  className={`mt-1.5 text-[10px] font-mono ${
                    isAssistant ? 'text-zinc-500' : 'text-rose-100/80'
                  }`}
                >
                  {new Date(msg.timestamp).toLocaleTimeString([], {
                    hour: '2-digit',
                    minute: '2-digit',
                  })}
                </div>
              </div>

              {!isAssistant && (
                <div className="flex h-8 w-8 shrink-0 items-center justify-center rounded-xl bg-zinc-900 border border-white/[0.08] text-zinc-300 shadow-sm">
                  <User className="h-4 w-4 text-blue-400" />
                </div>
              )}
            </div>
          );
        })}

        {/* Loading / Thinking Indicator */}
        {loading && (
          <div className="flex items-start gap-3">
            <div className="flex h-8 w-8 shrink-0 items-center justify-center rounded-xl bg-zinc-900 border border-white/[0.08] text-rose-300 animate-pulse">
              <Bot className="h-4 w-4" />
            </div>
            <div className="rounded-2xl bg-zinc-900/90 border border-white/[0.08] px-4 py-3 text-sm text-zinc-300 flex items-center gap-2">
              <Sparkles className="h-4 w-4 text-rose-400 animate-spin" />
              <span>{t.chat.aiThinking}</span>
            </div>
          </div>
        )}
      </div>

      {/* Input Area */}
      <form onSubmit={handleSend} className="mt-3 relative">
        <textarea
          rows={2}
          value={input}
          onChange={(e) => setInput(e.target.value)}
          onKeyDown={handleKeyDown}
          placeholder={t.chat.placeholder}
          className="w-full resize-none rounded-2xl border border-white/[0.08] bg-zinc-900/70 p-4 pr-14 text-sm text-zinc-100 placeholder-zinc-500 focus:border-rose-500/50 focus:outline-none focus:ring-1 focus:ring-rose-500/40 transition-colors shadow-sm"
        />
        <button
          type="submit"
          disabled={!input.trim() || loading || completing}
          className="btn-tactile absolute right-3.5 bottom-4 flex h-9 w-9 items-center justify-center rounded-xl bg-rose-500 text-white shadow-md shadow-rose-500/25 hover:bg-rose-400 disabled:opacity-30 transition-all"
        >
          <Send className="h-4 w-4" />
        </button>
      </form>
    </div>
  );
};
