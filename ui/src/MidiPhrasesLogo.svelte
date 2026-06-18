<script>
  let { name = "MIDI Phrases", class: className = "" } = $props();
</script>

{#if name === "MIDI Phrases"}
  <span class={`midi-phrases-logo ${className}`.trim()} aria-hidden="true">
    <svg class="track-word" viewBox="0 0 80 32" role="presentation">
      <defs>
        <path id="logo-track-m" d="M5 27V5L15 18L25 5V27" />
        <path id="logo-track-i-one" d="M35 5V27" />
        <path id="logo-track-d" d="M47 27V5H54C64 5 68 9.5 68 16S64 27 54 27H47" />
        <path id="logo-track-i-two" d="M76 5V27" />
      </defs>
      <g class="track-letter">
        <use class="track-shoulder" href="#logo-track-m" />
        <use class="track-bed" href="#logo-track-m" />
        <use class="track-divider" href="#logo-track-m" transform="translate(-2 0)" />
        <use class="track-divider" href="#logo-track-m" />
        <use class="track-divider" href="#logo-track-m" transform="translate(2 0)" />
      </g>
      <g class="track-letter">
        <use class="track-shoulder" href="#logo-track-i-one" />
        <use class="track-bed" href="#logo-track-i-one" />
        <use class="track-divider" href="#logo-track-i-one" transform="translate(-2 0)" />
        <use class="track-divider" href="#logo-track-i-one" />
        <use class="track-divider" href="#logo-track-i-one" transform="translate(2 0)" />
      </g>
      <g class="track-letter">
        <use class="track-shoulder" href="#logo-track-d" />
        <use class="track-bed" href="#logo-track-d" />
        <use class="track-divider" href="#logo-track-d" transform="translate(-2 0)" />
        <use class="track-divider" href="#logo-track-d" />
        <use class="track-divider" href="#logo-track-d" transform="translate(2 0)" />
      </g>
      <g class="track-letter">
        <use class="track-shoulder" href="#logo-track-i-two" />
        <use class="track-bed" href="#logo-track-i-two" />
        <use class="track-divider" href="#logo-track-i-two" transform="translate(-2 0)" />
        <use class="track-divider" href="#logo-track-i-two" />
        <use class="track-divider" href="#logo-track-i-two" transform="translate(2 0)" />
      </g>
    </svg>

    <span class="word-gap"></span>

    <svg class="track-phrase" viewBox="0 0 176 32" role="presentation">
      <defs>
        <path id="logo-track-p" d="M5 27V5H12C19 5 22 8 22 12.5S19 20 12 20H5" />
        <path id="logo-track-h" d="M31 27V5M45 5V27M31 16H45" />
        <path id="logo-track-r" d="M54 27V5H61C68 5 71 8 71 12.5S68 20 61 20H54M62 20L72 27" />
        <path id="logo-track-a" d="M80 27L89 5L98 27M84 18H94" />
        <path id="logo-track-s-one" d="M121 7C118 4 109 4 106 9C102 16 119 15 119 21C119 28 109 29 105 25" />
        <path id="logo-track-e" d="M131 5V27M131 5H145M131 16H143M131 27H145" />
        <path id="logo-track-s-two" d="M170 7C167 4 158 4 155 9C151 16 168 15 168 21C168 28 158 29 154 25" />
      </defs>
      <g class="track-letter">
        <use class="track-shoulder" href="#logo-track-p" />
        <use class="track-bed" href="#logo-track-p" />
        <use class="track-divider" href="#logo-track-p" transform="translate(-2 0)" />
        <use class="track-divider" href="#logo-track-p" />
        <use class="track-divider" href="#logo-track-p" transform="translate(2 0)" />
      </g>
      <g class="track-letter">
        <use class="track-shoulder" href="#logo-track-h" />
        <use class="track-bed" href="#logo-track-h" />
        <use class="track-divider" href="#logo-track-h" transform="translate(-2 0)" />
        <use class="track-divider" href="#logo-track-h" />
        <use class="track-divider" href="#logo-track-h" transform="translate(2 0)" />
      </g>
      <g class="track-letter">
        <use class="track-shoulder" href="#logo-track-r" />
        <use class="track-bed" href="#logo-track-r" />
        <use class="track-divider" href="#logo-track-r" transform="translate(-2 0)" />
        <use class="track-divider" href="#logo-track-r" />
        <use class="track-divider" href="#logo-track-r" transform="translate(2 0)" />
      </g>
      <g class="track-letter">
        <use class="track-shoulder" href="#logo-track-a" />
        <use class="track-bed" href="#logo-track-a" />
        <use class="track-divider" href="#logo-track-a" transform="translate(-2 0)" />
        <use class="track-divider" href="#logo-track-a" />
        <use class="track-divider" href="#logo-track-a" transform="translate(2 0)" />
      </g>
      <g class="track-letter">
        <use class="track-shoulder" href="#logo-track-s-one" />
        <use class="track-bed" href="#logo-track-s-one" />
        <use class="track-divider" href="#logo-track-s-one" transform="translate(-2 0)" />
        <use class="track-divider" href="#logo-track-s-one" />
        <use class="track-divider" href="#logo-track-s-one" transform="translate(2 0)" />
      </g>
      <g class="track-letter">
        <use class="track-shoulder" href="#logo-track-e" />
        <use class="track-bed" href="#logo-track-e" />
        <use class="track-divider" href="#logo-track-e" transform="translate(-2 0)" />
        <use class="track-divider" href="#logo-track-e" />
        <use class="track-divider" href="#logo-track-e" transform="translate(2 0)" />
      </g>
      <g class="track-letter">
        <use class="track-shoulder" href="#logo-track-s-two" />
        <use class="track-bed" href="#logo-track-s-two" />
        <use class="track-divider" href="#logo-track-s-two" transform="translate(-2 0)" />
        <use class="track-divider" href="#logo-track-s-two" />
        <use class="track-divider" href="#logo-track-s-two" transform="translate(2 0)" />
      </g>
    </svg>
  </span>
{:else}
  <span class={className}>{name}</span>
{/if}

<style>
  .midi-phrases-logo {
    display: inline-flex;
    height: 2rem;
    align-items: flex-end;
    color: var(--color-text);
    filter: drop-shadow(0 1px 1px color-mix(in srgb, var(--color-app) 55%, transparent));
    white-space: nowrap;
  }

  .track-word,
  .track-phrase {
    display: block;
    flex: none;
    height: 2rem;
    overflow: visible;
  }

  .track-word {
    width: 5rem;
  }

  .track-phrase {
    width: 11rem;
  }

  .word-gap {
    width: 0.42rem;
  }

  .track-letter {
    fill: none;
    filter: drop-shadow(0 0 4px color-mix(in srgb, var(--color-accent) 18%, transparent));
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .track-shoulder,
  .track-bed,
  .track-divider {
    vector-effect: non-scaling-stroke;
  }

  .track-shoulder {
    stroke: var(--color-accent);
    stroke-width: 10;
  }

  .track-bed {
    stroke: var(--color-text);
    stroke-width: 8;
  }

  .track-divider {
    stroke: var(--color-app);
    stroke-width: 0.65;
    stroke-dasharray: 1.2 1.6;
    stroke-linecap: butt;
    opacity: 0.86;
  }

</style>
