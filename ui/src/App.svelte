<script>
  let pluginName = "MIDI Phrases";
  let version = "0.0.1";

  if (window.__JUCE__?.initialisationData?.pluginName?.[0]) {
    pluginName = window.__JUCE__.initialisationData.pluginName[0];
  }

  if (window.__JUCE__?.initialisationData?.version?.[0]) {
    version = window.__JUCE__.initialisationData.version[0];
  }
</script>

<main class="flex min-h-screen flex-col items-center justify-center gap-6 p-8">
  <div class="text-center">
    <p class="text-xs font-medium uppercase tracking-widest text-emerald-400">ofsound</p>
    <h1 class="mt-2 text-3xl font-semibold tracking-tight">{pluginName}</h1>
    <p class="mt-1 text-sm text-zinc-400">v{version} · Svelte + Tailwind UI</p>
  </div>

  <p class="max-w-md text-center text-sm leading-relaxed text-zinc-400">
  Phrase interleaving UI will live here. In Debug, edit <code class="text-emerald-300">ui/src</code>
  and run <code class="text-emerald-300">npm run dev</code> for hot reload at localhost:5173.
  </p>
</main>
