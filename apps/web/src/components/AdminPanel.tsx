'use client';

import React, { useState, useEffect } from 'react';
import { ShieldAlert, Plus, Trash2, Filter } from 'lucide-react';
import { QuestionBankItem, InterviewDirection, InterviewLevel } from '@dev-dialogue/api-contracts';
import { api } from '@/lib/api-client';
import { useAuthStore } from '@/store/authStore';

export const AdminPanel: React.FC = () => {
  const { user, isAdminViewActive } = useAuthStore();
  const [questions, setQuestions] = useState<QuestionBankItem[]>([]);
  const [directionFilter, setDirectionFilter] = useState<string>('');
  const [showAddModal, setShowAddModal] = useState(false);

  // New question form state
  const [newDirection, setNewDirection] = useState<InterviewDirection>('frontend');
  const [newLevel, setNewLevel] = useState<InterviewLevel>('junior');
  const [newTopic, setNewTopic] = useState('');
  const [newText, setNewText] = useState('');
  const [submitting, setSubmitting] = useState(false);

  const isRealAdmin = user?.role === 'admin';
  const hasAccess = isRealAdmin && isAdminViewActive;

  useEffect(() => {
    if (!hasAccess) return;

    async function loadQuestions() {
      try {
        const data = await api.admin.getQuestions(
          directionFilter ? { direction: directionFilter } : undefined
        );
        setQuestions(data);
      } catch {
        // Fallback demo questions
        setQuestions([
          {
            id: 'q-1',
            direction: 'frontend',
            level: 'junior',
            stage: 'hard_skills',
            topic: 'React Hooks & State',
            questionText: 'Поясніть різницю між controlled та uncontrolled компонентами у формах.',
            isActive: true,
            createdBy: null,
            createdAt: new Date().toISOString(),
          },
          {
            id: 'q-2',
            direction: 'backend',
            level: 'junior',
            stage: 'hard_skills',
            topic: 'Networking & Protocols',
            questionText: 'Поясніть різницю між TCP та UDP. У яких сценаріях UDP є кращим вибором?',
            isActive: true,
            createdBy: null,
            createdAt: new Date().toISOString(),
          },
        ]);
      }
    }

    loadQuestions();
  }, [hasAccess, directionFilter]);

  if (!hasAccess) {
    return (
      <div className="mx-auto max-w-2xl px-4 py-20 text-center">
        <div className="mx-auto flex h-14 w-14 items-center justify-center rounded-2xl bg-amber-500/10 text-amber-400 border border-amber-500/20 mb-4">
          <ShieldAlert className="h-7 w-7" />
        </div>
        <h2 className="text-2xl font-bold text-white">Доступ обмежено</h2>
        <p className="mt-2 text-sm text-slate-400">
          Для перегляду та керування банком питань необхідно мати роль адміністратора та активний
          режим перегляду адміна.
        </p>
      </div>
    );
  }

  const handleCreateQuestion = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!newTopic.trim() || !newText.trim() || submitting) return;

    setSubmitting(true);
    try {
      const created = await api.admin.createQuestion({
        direction: newDirection,
        stage: 'hard_skills',
        level: newLevel,
        topic: newTopic.trim(),
        questionText: newText.trim(),
      });
      setQuestions((prev) => [created, ...prev]);
      setShowAddModal(false);
      setNewTopic('');
      setNewText('');
    } catch {
      // Local addition fallback
      const localItem: QuestionBankItem = {
        id: 'q-' + Math.random().toString(36).substring(2, 7),
        direction: newDirection,
        level: newLevel,
        stage: 'hard_skills',
        topic: newTopic.trim(),
        questionText: newText.trim(),
        isActive: true,
        createdBy: user?.id || null,
        createdAt: new Date().toISOString(),
      };
      setQuestions((prev) => [localItem, ...prev]);
      setShowAddModal(false);
      setNewTopic('');
      setNewText('');
    } finally {
      setSubmitting(false);
    }
  };

  const handleDelete = async (id: string) => {
    try {
      await api.admin.deleteQuestion(id);
    } catch {
      // Fallback
    }
    setQuestions((prev) => prev.filter((q) => q.id !== id));
  };

  return (
    <div className="mx-auto max-w-6xl px-4 py-8">
      {/* Header */}
      <div className="flex flex-col gap-4 sm:flex-row sm:items-center sm:justify-between border-b border-slate-800 pb-6 mb-8">
        <div>
          <div className="inline-flex items-center gap-1.5 rounded-full bg-indigo-500/10 px-3 py-1 text-xs font-semibold text-indigo-400 border border-indigo-500/20 mb-2">
            <ShieldAlert className="h-3.5 w-3.5" />
            Адміністративна панель
          </div>
          <h1 className="text-2xl font-bold tracking-tight text-white sm:text-3xl">
            Керування банком питань
          </h1>
          <p className="mt-1 text-sm text-slate-400">
            Додавайте та редагуйте теми і базові запитання, на основі яких AI будує сценарії
            інтерв&apos;ю.
          </p>
        </div>

        <button
          onClick={() => setShowAddModal(true)}
          className="inline-flex items-center gap-2 rounded-xl bg-indigo-600 px-5 py-2.5 text-sm font-semibold text-white shadow-lg shadow-indigo-600/20 hover:bg-indigo-500 transition"
        >
          <Plus className="h-4 w-4" />
          <span>Додати питання</span>
        </button>
      </div>

      {/* Filter Toolbar */}
      <div className="flex items-center gap-3 mb-6">
        <Filter className="h-4 w-4 text-slate-400" />
        <span className="text-xs font-semibold uppercase tracking-wider text-slate-400">
          Фільтр напрямку:
        </span>
        <div className="flex flex-wrap gap-2">
          {['', 'frontend', 'backend', 'devops', 'qa', 'pm'].map((dir) => (
            <button
              key={dir}
              onClick={() => setDirectionFilter(dir)}
              className={`rounded-lg px-3 py-1 text-xs font-medium transition ${
                directionFilter === dir
                  ? 'bg-indigo-600 text-white'
                  : 'bg-slate-800 text-slate-300 hover:bg-slate-700'
              }`}
            >
              {dir ? dir.toUpperCase() : 'Всі напрямки'}
            </button>
          ))}
        </div>
      </div>

      {/* Questions Table */}
      <div className="rounded-2xl border border-slate-800 bg-slate-900/60 overflow-hidden shadow-lg">
        {questions.length === 0 ? (
          <div className="p-12 text-center text-slate-400 text-sm">
            Питань за обраним фільтром не знайдено.
          </div>
        ) : (
          <div className="overflow-x-auto">
            <table className="w-full text-left text-sm text-slate-300">
              <thead className="bg-slate-950/60 text-xs font-semibold uppercase tracking-wider text-slate-400 border-b border-slate-800">
                <tr>
                  <th className="px-6 py-3">Напрямок</th>
                  <th className="px-6 py-3">Рівень</th>
                  <th className="px-6 py-3">Тема</th>
                  <th className="px-6 py-3">Текст питання</th>
                  <th className="px-6 py-3 text-right">Дії</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-800/60">
                {questions.map((q) => (
                  <tr key={q.id} className="hover:bg-slate-850/40 transition">
                    <td className="px-6 py-4 whitespace-nowrap">
                      <span className="rounded-md bg-blue-500/10 px-2 py-0.5 text-xs font-semibold text-blue-400 border border-blue-500/20 uppercase">
                        {q.direction}
                      </span>
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-xs capitalize text-slate-300">
                      {q.level}
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap font-medium text-white">
                      {q.topic}
                    </td>
                    <td className="px-6 py-4 text-xs text-slate-300 max-w-md">
                      <div className="line-clamp-2">{q.questionText}</div>
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-right">
                      <button
                        onClick={() => handleDelete(q.id)}
                        className="text-slate-500 hover:text-red-400 transition p-1"
                        title="Видалити"
                      >
                        <Trash2 className="h-4 w-4" />
                      </button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}
      </div>

      {/* Add Modal */}
      {showAddModal && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 backdrop-blur-sm p-4">
          <div className="w-full max-w-lg rounded-3xl border border-slate-800 bg-slate-900 p-6 shadow-2xl">
            <h3 className="text-lg font-bold text-white mb-4">Додати нове питання в банк</h3>
            <form onSubmit={handleCreateQuestion} className="space-y-4">
              <div className="grid grid-cols-2 gap-3">
                <div>
                  <label className="block text-xs font-semibold text-slate-400 mb-1">
                    Напрямок
                  </label>
                  <select
                    value={newDirection}
                    onChange={(e) => setNewDirection(e.target.value as InterviewDirection)}
                    className="w-full rounded-xl border border-slate-700 bg-slate-800 p-2.5 text-xs text-white"
                  >
                    <option value="frontend">Frontend</option>
                    <option value="backend">Backend</option>
                    <option value="devops">DevOps</option>
                    <option value="qa">QA</option>
                    <option value="pm">Project Manager</option>
                  </select>
                </div>
                <div>
                  <label className="block text-xs font-semibold text-slate-400 mb-1">Рівень</label>
                  <select
                    value={newLevel}
                    onChange={(e) => setNewLevel(e.target.value as InterviewLevel)}
                    className="w-full rounded-xl border border-slate-700 bg-slate-800 p-2.5 text-xs text-white"
                  >
                    <option value="trainee">Trainee</option>
                    <option value="junior">Junior</option>
                    <option value="middle">Middle</option>
                  </select>
                </div>
              </div>

              <div>
                <label className="block text-xs font-semibold text-slate-400 mb-1">
                  Тема (Topic)
                </label>
                <input
                  type="text"
                  required
                  placeholder="наприклад: React Lifecycle або TCP/UDP"
                  value={newTopic}
                  onChange={(e) => setNewTopic(e.target.value)}
                  className="w-full rounded-xl border border-slate-700 bg-slate-800 p-2.5 text-xs text-white"
                />
              </div>

              <div>
                <label className="block text-xs font-semibold text-slate-400 mb-1">
                  Текст запитання
                </label>
                <textarea
                  required
                  rows={3}
                  placeholder="Введіть формулювання технічного питання..."
                  value={newText}
                  onChange={(e) => setNewText(e.target.value)}
                  className="w-full rounded-xl border border-slate-700 bg-slate-800 p-2.5 text-xs text-white resize-none"
                />
              </div>

              <div className="flex justify-end gap-3 pt-2">
                <button
                  type="button"
                  onClick={() => setShowAddModal(false)}
                  className="rounded-xl border border-slate-700 px-4 py-2 text-xs font-semibold text-slate-300 hover:bg-slate-800 transition"
                >
                  Скасувати
                </button>
                <button
                  type="submit"
                  disabled={submitting}
                  className="rounded-xl bg-indigo-600 px-5 py-2 text-xs font-semibold text-white hover:bg-indigo-500 transition"
                >
                  {submitting ? 'Збереження...' : 'Зберегти'}
                </button>
              </div>
            </form>
          </div>
        </div>
      )}
    </div>
  );
};
