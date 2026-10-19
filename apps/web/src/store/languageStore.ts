import { create } from 'zustand';
import { persist } from 'zustand/middleware';

export type Language = 'uk' | 'en';

interface LanguageState {
  language: Language;
  setLanguage: (lang: Language) => void;
  toggleLanguage: () => void;
}

export const useLanguageStore = create<LanguageState>()(
  persist(
    (set) => ({
      language: 'uk',
      setLanguage: (language: Language) => set({ language }),
      toggleLanguage: () => set((state) => ({ language: state.language === 'uk' ? 'en' : 'uk' })),
    }),
    {
      name: 'devdialogue_language',
    }
  )
);
