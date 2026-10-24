/** @type {import('next').NextConfig} */
const nextConfig = {
  // Prevent aggressive double-mount race conditions during HMR on Windows
  reactStrictMode: false,
  webpack: (config, { dev }) => {
    if (dev) {
      // Aggregate rapid file writes and poll to prevent dropped file events on Windows NTFS
      config.watchOptions = {
        poll: 800,
        aggregateTimeout: 300,
      };
    }
    return config;
  },
};

export default nextConfig;

