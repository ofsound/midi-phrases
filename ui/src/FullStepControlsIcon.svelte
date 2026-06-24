<script>
  import {
    stepIconHeight,
    stepIconRx,
    stepIconViewBox,
    stepIconWidth,
    stepIconX,
    stepIconY,
    stepIconStrokeWidth,
  } from "./stepIconGeometry.js";

  let { class: className = "" } = $props();
</script>

<svg
  class={className}
  viewBox={stepIconViewBox}
  fill="none"
  stroke="currentColor"
  stroke-width={stepIconStrokeWidth}
  stroke-linecap="round"
  stroke-linejoin="round"
  aria-hidden="true"
>
  <rect x={stepIconX} y={stepIconY} width={stepIconWidth} height={stepIconHeight} rx={stepIconRx} />
  <path d="M5 8h14" />
  <path d="M7.5 11.5h9" />
  <path d="M7.5 14.5h5.5" />
  <path d="M5 17.5h14" />
</svg>
