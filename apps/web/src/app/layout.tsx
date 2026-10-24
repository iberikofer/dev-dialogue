import type { Metadata } from 'next';
import localFont from 'next/font/local';
import './globals.css';
import { Navbar } from '@/components/Navbar';
import { BackendStatusBanner } from '@/components/BackendStatusBanner';
import { Footer } from '@/components/Footer';
import { AuthProvider } from '@/components/AuthProvider';

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
  icons: {
    icon: [
      { url: '/logo.png', type: 'image/png' },
    ],
    shortcut: '/logo.png',
    apple: '/logo.png',
  },
};


export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="uk" className="dark">
      <body
        suppressHydrationWarning
        className={`${geistSans.variable} ${geistMono.variable} font-sans antialiased bg-[#09090b] text-zinc-100 min-h-screen flex flex-col`}
      >
        <AuthProvider>
          <Navbar />
          <BackendStatusBanner />
          <main className="flex-1">{children}</main>
          <Footer />
        </AuthProvider>
      </body>
    </html>
  );
}
