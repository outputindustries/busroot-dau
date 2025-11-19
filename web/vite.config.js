// vite.config.js
import { defineConfig } from "vite";

export default defineConfig({
  base: "/busroot-dau/",
  define: {
    __FIRMWARE_VERSION__: JSON.stringify(process.env.FIRMWARE_VERSION || "unknown"),
  },
});
