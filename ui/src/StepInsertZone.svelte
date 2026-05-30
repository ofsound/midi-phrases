<script>
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  let { onInsert, accent = emeraldRowAccent, muted = false } = $props();
</script>

<button
  type="button"
  aria-label="Insert step"
  class="group relative flex h-full w-4 shrink-0 cursor-pointer flex-col items-center justify-center border-0 bg-transparent py-2 p-0 outline-none {accent.insertZoneFocus}"
  onclick={onInsert}
>
  <span
    class="pointer-events-none min-h-0 w-0.5 flex-1 {muted
      ? 'bg-zinc-600'
      : accent.bgAccent} opacity-0 transition-opacity group-hover:opacity-100 group-focus-visible:opacity-100"
    aria-hidden="true"
  ></span>
  <span
    class="pointer-events-none shrink-0 px-0.5 text-lg leading-none font-bold {muted
      ? 'text-zinc-500'
      : accent.textAccent} opacity-0 transition-opacity group-hover:opacity-100 group-focus-visible:opacity-100"
    aria-hidden="true"
  >
    +
  </span>
  <span
    class="pointer-events-none min-h-0 w-0.5 flex-1 {muted
      ? 'bg-zinc-600'
      : accent.bgAccent} opacity-0 transition-opacity group-hover:opacity-100 group-focus-visible:opacity-100"
    aria-hidden="true"
  ></span>
</button>
