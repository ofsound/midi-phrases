// Vector artwork preserved from the original Svelte frontend.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace phrases::ui
{
    inline constexpr const char* RowDisableIcon = R"svg(<svg 
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  aria-hidden="true"
>
  <path
    d="M12 3v9"
    fill="none"
    stroke="#ffffff"
    stroke-width="3"
    stroke-linecap="round"
  />
  <path
    d="M18.36 6.64a9 9 0 1 1-12.73 0"
    fill="none"
    stroke="#ffffff"
    stroke-width="3"
    stroke-linecap="round"
  />
</svg>)svg";
    inline constexpr const char* StepGearIcon = R"svg(<svg 
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  fill="none"
  aria-hidden="true"
>
  <path
    d="M15 12C15 13.6569 13.6569 15 12 15C10.3431 15 9 13.6569 9 12C9 10.3431 10.3431 9 12 9C13.6569 9 15 10.3431 15 12Z"
    stroke="#ffffff"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
  />
  <path
    d="M12.9046 3.06005C12.6988 3 12.4659 3 12 3C11.5341 3 11.3012 3 11.0954 3.06005C10.7942 3.14794 10.5281 3.32808 10.3346 3.57511C10.2024 3.74388 10.1159 3.96016 9.94291 4.39272C9.69419 5.01452 9.00393 5.33471 8.36857 5.123L7.79779 4.93281C7.3929 4.79785 7.19045 4.73036 6.99196 4.7188C6.70039 4.70181 6.4102 4.77032 6.15701 4.9159C5.98465 5.01501 5.83376 5.16591 5.53197 5.4677C5.21122 5.78845 5.05084 5.94882 4.94896 6.13189C4.79927 6.40084 4.73595 6.70934 4.76759 7.01551C4.78912 7.2239 4.87335 7.43449 5.04182 7.85566C5.30565 8.51523 5.05184 9.26878 4.44272 9.63433L4.16521 9.80087C3.74031 10.0558 3.52786 10.1833 3.37354 10.3588C3.23698 10.5141 3.13401 10.696 3.07109 10.893C3 11.1156 3 11.3658 3 11.8663C3 12.4589 3 12.7551 3.09462 13.0088C3.17823 13.2329 3.31422 13.4337 3.49124 13.5946C3.69158 13.7766 3.96395 13.8856 4.50866 14.1035C5.06534 14.3261 5.35196 14.9441 5.16236 15.5129L4.94721 16.1584C4.79819 16.6054 4.72367 16.829 4.7169 17.0486C4.70875 17.3127 4.77049 17.5742 4.89587 17.8067C5.00015 18.0002 5.16678 18.1668 5.5 18.5C5.83323 18.8332 5.99985 18.9998 6.19325 19.1041C6.4258 19.2295 6.68733 19.2913 6.9514 19.2831C7.17102 19.2763 7.39456 19.2018 7.84164 19.0528L8.36862 18.8771C9.00393 18.6654 9.6942 18.9855 9.94291 19.6073C10.1159 20.0398 10.2024 20.2561 10.3346 20.4249C10.5281 20.6719 10.7942 20.8521 11.0954 20.94C11.3012 21 11.5341 21 12 21C12.4659 21 12.6988 21 12.9046 20.94C13.2058 20.8521 13.4719 20.6719 13.6654 20.4249C13.7976 20.2561 13.8841 20.0398 14.0571 19.6073C14.3058 18.9855 14.9961 18.6654 15.6313 18.8773L16.1579 19.0529C16.605 19.2019 16.8286 19.2764 17.0482 19.2832C17.3123 19.2913 17.5738 19.2296 17.8063 19.1042C17.9997 18.9999 18.1664 18.8333 18.4996 18.5001C18.8328 18.1669 18.9994 18.0002 19.1037 17.8068C19.2291 17.5743 19.2908 17.3127 19.2827 17.0487C19.2759 16.8291 19.2014 16.6055 19.0524 16.1584L18.8374 15.5134C18.6477 14.9444 18.9344 14.3262 19.4913 14.1035C20.036 13.8856 20.3084 13.7766 20.5088 13.5946C20.6858 13.4337 20.8218 13.2329 20.9054 13.0088C21 12.7551 21 12.4589 21 11.8663C21 11.3658 21 11.1156 20.9289 10.893C20.866 10.696 20.763 10.5141 20.6265 10.3588C20.4721 10.1833 20.2597 10.0558 19.8348 9.80087L19.5569 9.63416C18.9478 9.26867 18.6939 8.51514 18.9578 7.85558C19.1262 7.43443 19.2105 7.22383 19.232 7.01543C19.2636 6.70926 19.2003 6.40077 19.0506 6.13181C18.9487 5.94875 18.7884 5.78837 18.4676 5.46762C18.1658 5.16584 18.0149 5.01494 17.8426 4.91583C17.5894 4.77024 17.2992 4.70174 17.0076 4.71872C16.8091 4.73029 16.6067 4.79777 16.2018 4.93273L15.6314 5.12287C14.9961 5.33464 14.3058 5.0145 14.0571 4.39272C13.8841 3.96016 13.7976 3.74388 13.6654 3.57511C13.4719 3.32808 13.2058 3.14794 12.9046 3.06005Z"
    stroke="#ffffff"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
  />
</svg>)svg";
    inline constexpr const char* StepSkipIcon = R"svg(<svg 
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  aria-hidden="true"
>
  <g transform="translate(12 12) scale(0.68) translate(-12 -12)">
    <path
      fill="#ffffff"
      d="M3 9 10 9 10 4 21 12 10 20 10 15 3 15Z"
    />
  </g>
</svg>)svg";
    inline constexpr const char* RowReverseOrderIcon = R"svg(<svg 
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  aria-hidden="true"
  fill="none"
  stroke="#ffffff"
  stroke-width="2.5"
  stroke-linecap="round"
  stroke-linejoin="round"
>
  <path d="M5 6.5 H10.5 M7.5 4 10.5 6.5 7.5 9" />
  <path d="M19 17.5 H13.5 M16.5 15 13.5 17.5 16.5 20" />
</svg>)svg";
    inline constexpr const char* RowRandomizeOrderIcon = R"svg(<svg 
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  aria-hidden="true"
>
  <rect
    x="4"
    y="4"
    width="16"
    height="16"
    rx="3"
    fill="none"
    stroke="#ffffff"
    stroke-width="2.25"
  />
  <circle cx="8.5" cy="8.5" r="1.3" fill="#ffffff" />
  <circle cx="15.5" cy="8.5" r="1.3" fill="#ffffff" />
  <circle cx="12" cy="12" r="1.3" fill="#ffffff" />
  <circle cx="8.5" cy="15.5" r="1.3" fill="#ffffff" />
  <circle cx="15.5" cy="15.5" r="1.3" fill="#ffffff" />
</svg>)svg";
    inline constexpr const char* RowRandomizeOctaveIcon = R"svg(<svg 
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  aria-hidden="true"
  fill="none"
  stroke="#ffffff"
  stroke-width="2.5"
  stroke-linecap="round"
  stroke-linejoin="round"
>
  <path d="M7 11 V5.5 M4 7.5 7 4.5 10 7.5" />
  <path d="M17 13 V18.5 M14 16 17 19 20 16" />
</svg>)svg";
    inline constexpr const char* RowRandomizeLengthIcon = R"svg(<svg 
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  aria-hidden="true"
>
  <rect
    x="4"
    y="4"
    width="16"
    height="16"
    rx="3"
    fill="none"
    stroke="#ffffff"
    stroke-width="2.25"
  />
  <text
    x="12"
    y="14.4"
    text-anchor="middle"
    dominant-baseline="middle"
    fill="#ffffff"
    font-family="ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace"
    font-size="6.5"
    font-weight="800"
  >
    n*x
  </text>
</svg>)svg";
    inline constexpr const char* ShimmerIcon = R"svg(<svg 
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  aria-hidden="true"
  fill="none"
  stroke="#ffffff"
  stroke-linecap="round"
  stroke-linejoin="round"
>
  <path
    d="M4 7.5 C7 5 10 10 13 7.5 S19 5 20 7.5"
    stroke-width="1.45"
    opacity="0.38"
  />
  <path
    d="M4 12 C7 9.5 10 14.5 13 12 S19 9.5 20 12"
    stroke-width="1.85"
    opacity="0.68"
  />
  <path
    d="M4 16.5 C7 14 10 19 13 16.5 S19 14 20 16.5"
    stroke-width="2.25"
    opacity="1"
  />
</svg>)svg";
    inline constexpr const char* FullscreenIcon = R"svg(<svg 
  viewBox="0 0 20 20"
  fill="none"
  stroke="#ffffff"
  stroke-width="1.8"
  stroke-linecap="round"
  stroke-linejoin="round"
  aria-hidden="true"
>
  <path d="M7.25 3.25H3.75V6.75" />
  <path d="M3.75 3.75L8 8" />
  <path d="M12.75 3.25H16.25V6.75" />
  <path d="M16.25 3.75L12 8" />
  <path d="M7.25 16.75H3.75V13.25" />
  <path d="M3.75 16.25L8 12" />
  <path d="M12.75 16.75H16.25V13.25" />
  <path d="M16.25 16.25L12 12" />
</svg>)svg";
    inline constexpr const char* ThemeMoonIcon = R"svg(<svg 
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  fill="none"
  stroke="#ffffff"
  stroke-width="2"
  stroke-linecap="round"
  stroke-linejoin="round"
  aria-hidden="true"
>
  <path d="M20.354 15.354A9 9 0 018.646 3.646 9.003 9.003 0 0012 21a9.003 9.003 0 008.354-5.646z" />
</svg>)svg";
    inline constexpr const char* ThemeSunIcon = R"svg(<svg 
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  fill="none"
  stroke="#ffffff"
  stroke-width="2"
  stroke-linecap="round"
  stroke-linejoin="round"
  aria-hidden="true"
>
  <path d="M12 3v1m0 16v1m9-9h-1M4 12H3m15.364 6.364l-.707-.707M6.343 6.343l-.707-.707m12.728 0l-.707.707M6.343 17.657l-.707.707M16 12a4 4 0 11-8 0 4 4 0 018 0z" />
</svg>)svg";
    inline constexpr const char* ThemeAltIcon = R"svg(<svg 
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  fill="none"
  stroke="#ffffff"
  stroke-width="2"
  stroke-linecap="round"
  stroke-linejoin="round"
  aria-hidden="true"
>
  <path d="M12 4.25L5.25 19.25M12 4.25L18.75 19.25M8.25 14.25h7.5" />
  <path d="M10.25 10.25h3.5" stroke-width="1.5" />
</svg>)svg";
    inline constexpr const char* RemoveXIcon = R"svg(<svg  viewBox="0 0 10 10" aria-hidden="true">
  <path
    d="M2 2 L8 8 M8 2 L2 8"
    fill="none"
    stroke="#ffffff"
    stroke-width="1.75"
    stroke-linecap="round"
  />
</svg>)svg";
    inline constexpr const char* ScaleModeIcon = R"svg(<svg 
  viewBox="0 0 24 24"
  fill="none"
  stroke="#ffffff"
  stroke-width="2"
  stroke-linecap="round"
  stroke-linejoin="round"
  aria-hidden="true"
>
  <path d="M9 18V5l12-2v13" />
  <circle cx="6" cy="18" r="3" />
  <circle cx="18" cy="16" r="3" />
</svg>)svg";
    inline constexpr const char* ColorsPaletteIcon = R"svg(<svg 
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  fill="#ffffff"
  aria-hidden="true"
>
  <path
    d="M19.54 5.08A10.61 10.61 0 0 0 11.91 2a10 10 0 0 0-.05 20 2.58 2.58 0 0 0 2.53-1.89 2.52 2.52 0 0 0-.57-2.28.5.5 0 0 1 .37-.83h1.65A6.15 6.15 0 0 0 22 11.33a8.48 8.48 0 0 0-2.46-6.25zM15.88 15h-1.65a2.49 2.49 0 0 0-1.87 4.15.49.49 0 0 1 .12.49c-.05.21-.28.34-.59.36a8 8 0 0 1-7.82-9.11A8.1 8.1 0 0 1 11.92 4H12a8.47 8.47 0 0 1 6.1 2.48 6.5 6.5 0 0 1 1.9 4.77A4.17 4.17 0 0 1 15.88 15z"
  />
  <circle cx="12" cy="6.5" r="1.5" />
  <path d="M15.25 7.2a1.5 1.5 0 1 0 2.05.55 1.5 1.5 0 0 0-2.05-.55z" />
  <path d="M8.75 7.2a1.5 1.5 0 1 0 .55 2.05 1.5 1.5 0 0 0-.55-2.05z" />
  <path d="M6.16 11.26a1.5 1.5 0 1 0 2.08.4 1.49 1.49 0 0 0-2.08-.4z" />
</svg>)svg";
    inline constexpr const char* crossMod = R"svg(<svg viewBox="0 0 24 24" fill="none">
    <!-- Carrier row timing; pitch, velocity, and duration cross from other rows. -->
    <circle cx="4" cy="7" r="1.6" fill="#ffffff" />
    <circle cx="4" cy="12" r="1.6" fill="#ffffff" />
    <circle cx="4" cy="17" r="1.6" fill="#ffffff" />
    <circle cx="20" cy="7" r="1.6" fill="#ffffff" />
    <circle cx="20" cy="12" r="1.6" fill="#ffffff" />
    <circle cx="20" cy="17" r="1.6" fill="#ffffff" />
    <path
      d="M5.6 7 C10 7 14 9.5 18.4 12"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M5.6 12 C10 12 14 14.5 18.4 17"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M5.6 17 C10 17 14 14.5 18.4 7"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
  </svg>)svg";
    inline constexpr const char* canon = R"svg(<svg viewBox="0 0 24 24" fill="none">
    <!-- Leader imitated by a delayed follower row. -->
    <path
      d="M4 8 H10.5 C13 8 14.5 11 17 11 H20"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M4 14 H10.5 C13 14 14.5 17 17 17 H20"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
      opacity="0.62"
    />
    <circle cx="4" cy="8" r="1.55" fill="#ffffff" />
    <circle cx="20" cy="11" r="1.55" fill="#ffffff" />
    <circle cx="4" cy="14" r="1.55" fill="#ffffff" opacity="0.62" />
    <circle cx="20" cy="17" r="1.55" fill="#ffffff" opacity="0.62" />
    <path d="M10.2 5.8 V18.2" stroke="#ffffff" stroke-width="1.1" stroke-linecap="round" opacity="0.35" />
    <path d="M13.8 5.8 V18.2" stroke="#ffffff" stroke-width="1.1" stroke-linecap="round" opacity="0.35" />
  </svg>)svg";
    inline constexpr const char* retroInv = R"svg(<svg viewBox="0 0 24 24" fill="none">
    <!-- Reversed contour reflected around the following row. -->
    <path
      d="M4 7 C8 7 10 12 12 12 C14 12 16 17 20 17"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M4 17 C8 17 10 12 12 12 C14 12 16 7 20 7"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
      opacity="0.58"
    />
    <circle cx="4" cy="7" r="1.5" fill="#ffffff" />
    <circle cx="4" cy="17" r="1.5" fill="#ffffff" opacity="0.58" />
    <circle cx="20" cy="7" r="1.5" fill="#ffffff" opacity="0.58" />
    <circle cx="20" cy="17" r="1.5" fill="#ffffff" />
    <path d="M12 5.25 V18.75" stroke="#ffffff" stroke-width="1.1" stroke-linecap="round" opacity="0.4" />
    <path
      d="M9.25 10.15 L12 12 L14.75 10.15"
      stroke="#ffffff"
      stroke-width="1.1"
      stroke-linecap="round"
      stroke-linejoin="round"
      opacity="0.45"
    />
  </svg>)svg";
    inline constexpr const char* hocket = R"svg(<svg viewBox="0 0 24 24" fill="none">
    <!-- Long gestures broken into alternating row handoffs. -->
    <path d="M4 6 H7.25" stroke="#ffffff" stroke-width="1.35" stroke-linecap="round" opacity="0.7" />
    <path d="M4 12 H7.25" stroke="#ffffff" stroke-width="1.35" stroke-linecap="round" opacity="0.7" />
    <path d="M4 18 H7.25" stroke="#ffffff" stroke-width="1.35" stroke-linecap="round" opacity="0.7" />
    <path
      d="M7.25 6 C10 6 10 12 12.75 12 C15.5 12 15.5 18 18.25 18"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M7.25 18 C10 18 10 12 12.75 12 C15.5 12 15.5 6 18.25 6"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
      opacity="0.55"
    />
    <circle cx="4" cy="6" r="1.45" fill="#ffffff" opacity="0.7" />
    <circle cx="4" cy="12" r="1.45" fill="#ffffff" opacity="0.7" />
    <circle cx="4" cy="18" r="1.45" fill="#ffffff" opacity="0.7" />
    <path d="M11.7 9.75 V14.25" stroke="#ffffff" stroke-width="1.15" stroke-linecap="round" opacity="0.4" />
    <path d="M14.3 9.75 V14.25" stroke="#ffffff" stroke-width="1.15" stroke-linecap="round" opacity="0.4" />
    <circle cx="20" cy="6" r="1.45" fill="#ffffff" />
    <circle cx="20" cy="12" r="1.45" fill="#ffffff" />
    <circle cx="20" cy="18" r="1.45" fill="#ffffff" />
  </svg>)svg";
    inline constexpr const char* tendril = R"svg(<svg viewBox="0 0 24 24" fill="none">
    <!-- A source curls toward a neighboring row, then resolves as a small phraselet. -->
    <circle cx="4" cy="12" r="1.55" fill="#ffffff" />
    <path
      d="M5.55 12 C8.2 11.9 9.25 8.4 11.75 8.7 C14.1 8.95 14.15 13.25 16.55 13.45 C18.05 13.58 18.9 12.25 20 10.75"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M8.2 15.6 C10.1 17.55 13.35 17.35 15.2 15.25"
      stroke="#ffffff"
      stroke-width="1.2"
      stroke-linecap="round"
      stroke-linejoin="round"
      opacity="0.5"
    />
    <circle cx="11.75" cy="8.7" r="1.25" fill="#ffffff" opacity="0.72" />
    <circle cx="16.55" cy="13.45" r="1.35" fill="#ffffff" />
    <circle cx="20" cy="10.75" r="1.25" fill="#ffffff" opacity="0.58" />
  </svg>)svg";
    inline constexpr const char* snare = R"svg(<svg viewBox="0 0 24 24" fill="none">
    <!-- Row output locked to adjacent drum notes. -->
    <path
      d="M5.2 8.25 H18.8 L17.75 17.25 H6.25 Z"
      stroke="#ffffff"
      stroke-width="1.35"
      stroke-linejoin="round"
    />
    <path
      d="M6.45 10.45 H17.55"
      stroke="#ffffff"
      stroke-width="1.1"
      stroke-linecap="round"
      opacity="0.62"
    />
    <path
      d="M8.25 14.9 L15.75 11.65"
      stroke="#ffffff"
      stroke-width="1.05"
      stroke-linecap="round"
      opacity="0.62"
    />
    <path
      d="M8.25 11.65 L15.75 14.9"
      stroke="#ffffff"
      stroke-width="1.05"
      stroke-linecap="round"
      opacity="0.62"
    />
    <path
      d="M8 6.3 L4.6 3.85"
      stroke="#ffffff"
      stroke-width="1.15"
      stroke-linecap="round"
    />
    <path
      d="M16 6.3 L19.4 3.85"
      stroke="#ffffff"
      stroke-width="1.15"
      stroke-linecap="round"
    />
    <circle cx="4.25" cy="3.6" r="1.05" fill="#ffffff" />
    <circle cx="19.75" cy="3.6" r="1.05" fill="#ffffff" />
  </svg>)svg";
    inline constexpr const char* echo = R"svg(<svg viewBox="0 0 24 24" fill="none">
    <!-- One carrier event multiplied into staggered, fading repeats. -->
    <path
      d="M4 18 H20"
      stroke="#ffffff"
      stroke-width="1.2"
      stroke-linecap="round"
      opacity="0.35"
    />
    <path d="M5.5 13 V18" stroke="#ffffff" stroke-width="1.45" stroke-linecap="round" />
    <path d="M9.5 12 V18" stroke="#ffffff" stroke-width="1.45" stroke-linecap="round" opacity="0.72" />
    <path d="M13.5 10.75 V18" stroke="#ffffff" stroke-width="1.45" stroke-linecap="round" opacity="0.5" />
    <path d="M17.5 9.5 V18" stroke="#ffffff" stroke-width="1.45" stroke-linecap="round" opacity="0.32" />
  </svg>)svg";
    inline constexpr const char* weave = R"svg(<svg viewBox="0 0 24 24" fill="none">
    <!-- Competing same-time events woven into one weighted winner. -->
    <circle cx="4" cy="4.5" r="1.6" fill="#ffffff" />
    <circle cx="4" cy="8.25" r="1.6" fill="#ffffff" />
    <circle cx="4" cy="12" r="1.6" fill="#ffffff" />
    <circle cx="4" cy="15.75" r="1.6" fill="#ffffff" />
    <circle cx="4" cy="19.5" r="1.6" fill="#ffffff" />
    <path
      d="M5.6 4.5 C9 4.5 11 8.25 13.5 12"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M5.6 8.25 C8.75 8.75 11 10.25 13.5 12"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M5.6 12 H13.5"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M5.6 15.75 C8.75 13.25 11 13.75 13.5 12"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M5.6 19.5 C9 19.5 11 15.75 13.5 12"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M13.5 12 H18.4"
      stroke="#ffffff"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <circle cx="20" cy="12" r="1.6" fill="#ffffff" />
  </svg>)svg";
    inline constexpr const char* SaplingIcon = R"svg(<svg
  
  viewBox="0 0 24 24"
  fill="none"
  stroke="#ffffff"
  stroke-width="2"
  stroke-linecap="round"
  stroke-linejoin="round"
  aria-hidden="true"
>
  <path d="M12 21v-8" />
  <path d="M12 13c0-4 2.5-7 7-8 0 4.5-2.5 7-7 8Z" />
  <path d="M12 13c0-3.5-2.5-6-7-6 0 4 2.5 6 7 6Z" />
  <path d="M5 21h14" />
</svg>)svg";
    inline constexpr const char* RowEditPencilIcon = R"svg(<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="2 2 20 20"
  
  aria-hidden="true"
  fill="none"
  stroke="#ffffff"
  stroke-width="2.25"
  stroke-linecap="round"
  stroke-linejoin="round"
>
  <path d="M16.5 3.5a2.121 2.121 0 0 1 3 3L7 19l-4 1 1-4L16.5 3.5z" />
</svg>)svg";
    inline constexpr const char* StepMuteIcon = R"svg(<svg viewBox="0 0 24 24" fill="#ffffff"><path d="M3 9H7L13 4V20L7 15H3Z"/><path d="M16 8L22 16M22 8L16 16" fill="none" stroke="#ffffff" stroke-width="2"/></svg>)svg";
    inline constexpr const char* logoPaths[] = {
        "M5 27V5L15 18L25 5V27",
        "M35 5V27",
        "M47 27V5H54C64 5 68 9.5 68 16S64 27 54 27H47",
        "M76 5V27",
        "M5 27V5H12C19 5 22 8 22 12.5S19 20 12 20H5",
        "M31 27V5M45 5V27M31 16H45",
        "M54 27V5H61C68 5 71 8 71 12.5S68 20 61 20H54M62 20L72 27",
        "M80 27L89 5L98 27M84 18H94",
        "M121 7C118 4 109 4 106 9C102 16 119 15 119 21C119 28 109 29 105 25",
        "M131 5V27M131 5H145M131 16H143M131 27H145",
        "M170 7C167 4 158 4 155 9C151 16 168 15 168 21C168 28 158 29 154 25"
    };
    inline constexpr auto RowShiftOrderIcon = R"SVG(<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  
  aria-hidden="true"
  fill="none"
  stroke="#ffffff"
  stroke-width="2.5"
  stroke-linecap="round"
  stroke-linejoin="round"
>
  <path d="M16 5.5 19 8.5 16 11.5" />
  <path d="M5 8.5 H19" />
  <path d="M8 18.5 5 15.5 8 12.5" />
  <path d="M19 15.5 H5" />
</svg>
)SVG";
    inline constexpr auto UndoIcon = R"SVG(<svg viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M9 14 4 9l5-5"/><path d="M4 9h10a6 6 0 0 1 0 12h-2"/></svg>)SVG";
    inline constexpr auto RedoIcon = R"SVG(<svg viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m15 14 5-5-5-5"/><path d="M20 9H10a6 6 0 0 0 0 12h2"/></svg>)SVG";
}
