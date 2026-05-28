<script>
  let { onInsert } = $props();
</script>

<button
  type="button"
  aria-label="Insert step"
  class="group relative flex h-full w-4 shrink-0 cursor-pointer flex-col items-center justify-center border-0 bg-transparent py-2 p-0 outline-none focus-visible:ring-1 focus-visible:ring-emerald-500"
  onclick={onInsert}
>
  <span
    class="pointer-events-none min-h-0 w-0.5 flex-1 bg-emerald-400 opacity-0 transition-opacity group-hover:opacity-100 group-focus-visible:opacity-100"
    aria-hidden="true"
  ></span>
  <span
    class="pointer-events-none shrink-0 px-0.5 text-lg leading-none font-bold text-emerald-400 opacity-0 transition-opacity group-hover:opacity-100 group-focus-visible:opacity-100"
    aria-hidden="true"
  >
    +
  </span>
  <span
    class="pointer-events-none min-h-0 w-0.5 flex-1 bg-emerald-400 opacity-0 transition-opacity group-hover:opacity-100 group-focus-visible:opacity-100"
    aria-hidden="true"
  ></span>
</button>
