<script>
  /**
   * @typedef {"start" | "end" | "custom"} HandleEdge
   */

  /**
   * @type {{
   *   edge?: HandleEdge,
   *   leftPx?: number | null,
   *   ariaLabel: string,
   *   title?: string,
   *   disabled?: boolean,
   *   ringFocusClass?: string,
   *   onPointerDown: (event: PointerEvent) => void,
   *   onPointerMove?: (event: PointerEvent) => void,
   *   onPointerUp?: (event: PointerEvent) => void,
   *   onPointerCancel?: (event: PointerEvent) => void,
   *   onKeyDown?: (event: KeyboardEvent) => void,
   * }}
   */
  let {
    edge = "end",
    leftPx = null,
    ariaLabel,
    title = "",
    disabled = false,
    ringFocusClass = "focus-visible:ring-1 focus-visible:ring-focus-ring",
    onPointerDown,
    onPointerMove,
    onPointerUp,
    onPointerCancel,
    onKeyDown,
  } = $props();

  let positionClass = $derived(
    edge === "start" ? "-left-1.5" : edge === "end" ? "-right-1.5" : "",
  );

  let customStyle = $derived(
    edge === "custom" && leftPx != null ? `left: ${leftPx}px; transform: translateX(-50%);` : "",
  );
</script>

<button
  type="button"
  data-compact-step-resize
  data-cursor="horizontal-drag"
  {ariaLabel}
  {title}
  {disabled}
  class="group absolute top-0 z-30 flex h-full w-3 touch-none items-center justify-center border-0 bg-transparent p-0 outline-none disabled:pointer-events-none disabled:opacity-50 {positionClass} {ringFocusClass}"
  style={customStyle}
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerCancel}
  onkeydown={onKeyDown}
>
  <span class="compact-step-resize-handle pointer-events-none" aria-hidden="true"></span>
</button>

<style>
  .compact-step-resize-handle {
    height: 78%;
    width: 0.2rem;
    border-radius: 9999px;
    background-color: color-mix(in srgb, var(--color-text) 70%, transparent);
    border: 1px solid color-mix(in srgb, var(--color-text) 20%, transparent);
    box-shadow: 0 0 0 1.2px var(--color-app);
    transition:
      width 75ms,
      background-color 75ms,
      border-color 75ms,
      box-shadow 75ms;
  }

  :global(.group:hover) .compact-step-resize-handle,
  :global(.group[data-mp-hover]) .compact-step-resize-handle {
    width: 0.28rem;
    background-color: var(--color-text);
    border-color: var(--color-text);
    box-shadow:
      0 0 4px color-mix(in srgb, var(--color-text) 30%, transparent),
      0 0 0 1.2px var(--color-app);
  }
</style>
