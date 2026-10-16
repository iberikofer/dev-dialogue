import type { Metadata } from 'next';
import localFont from 'next/font/local';
import './globals.css';
import { Navbar } from '@/components/Navbar';

const geistSans = localFont({
  src: './fonts/GeistVF.woff',
  variable: '--font-geist-sans',
  weight: '100 900',
});
const geistMono = localFont({
  src: './fonts/GeistMonoVF.woff',
  variable: '--font-geist-mono',
  weight: '100 900',
});

export const metadata: Metadata = {
  title: 'Dev Dialogue — AI Технічний Симулятор Співбесід',
  description:
    'Платформа інтерактивної підготовки до технічних співбесід для Frontend, Backend, DevOps, QA та PM з AI-зворотним звʼязком.',
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="uk" className="dark">
      <body
        className={`${geistSans.variable} ${geistMono.variable} font-sans antialiased bg-[#09090b] text-zinc-100 min-h-screen flex flex-col`}
      >
        <Navbar />
        <main className="flex-1">{children}</main>
        <footer className="border-t border-white/[0.08] bg-zinc-950/80 py-6 text-center text-xs text-zinc-500 font-mono">
          Dev Dialogue &copy; 2026 &middot; Платформа розроблена для олімпіади ВНТУ ІПЗ ФІТКІ
        </footer>
      </body>
    </html>
  );
}
