import { useLanguageStore, Language } from '@/store/languageStore';
import { translations, TranslationDictionary } from '@/lib/translations';

export function useTranslation(): {
  lang: Language;
  setLanguage: (lang: Language) => void;
  toggleLanguage: () => void;
  t: TranslationDictionary;
} {
  const { language, setLanguage, toggleLanguage } = useLanguageStore();
  const t = translations[language] || translations.uk;

  return {
    lang: language,
    setLanguage,
    toggleLanguage,
    t,
  };
}
