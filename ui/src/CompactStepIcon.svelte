<script>
  import { stepIconOuterRect, stepIconViewBox } from "./stepIconGeometry.js";

  let { class: className = "" } = $props();
</script>

<svg
  class={className}
  viewBox={stepIconViewBox}
  fill="currentColor"
  aria-hidden="true"
>
  <rect
    x={stepIconOuterRect.x}
    y={stepIconOuterRect.y}
    width={stepIconOuterRect.width}
    height={stepIconOuterRect.height}
    rx={stepIconOuterRect.rx}
  />
</svg>
