<script>
  let { onInsert } = $props();
</script>

<button
  type="button"
  aria-label="Insert step"
  class="group relative flex h-full w-4 shrink-0 cursor-pointer items-center justify-center border-0 bg-transparent p-0 outline-none focus-visible:ring-1 focus-visible:ring-emerald-500"
  onclick={onInsert}
>
  <span
    class="pointer-events-none absolute inset-y-2 left-1/2 w-0.5 -translate-x-1/2 bg-emerald-400 opacity-0 transition-opacity group-hover:opacity-100 group-focus-visible:opacity-100"
    aria-hidden="true"
  ></span>
  <span
    class="pointer-events-none relative z-10 flex h-5 w-5 items-center justify-center rounded-full border border-emerald-400 bg-zinc-900 text-sm leading-none font-medium text-emerald-400 opacity-0 transition-opacity group-hover:opacity-100 group-focus-visible:opacity-100"
    aria-hidden="true"
  >
    +
  </span>
</button>
