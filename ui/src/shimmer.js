/** @typedef {{ start: number, end: number, midi: number, velocity: number, row: number, step: number }} ScheduledNote */

import {defaultPulseIndex, pulseQuartersForIndex} from "./pulseLayout.js";
import {maxPercentValue} from "./percentLimits.js";
import {defaultStepTimingMultiplierIndex, stepTimingMultiplierCount, timingMultiplierAtIndex} from "./stepCellLayout.js";

export const shimmerSemitoneShift = 12;
export const minMidiNote = 0;
export const maxMidiNote = 127;

export const defaultShimmerEnabled = false;
export const defaultShimmerDelayMultiplierIndex = defaultStepTimingMultiplierIndex;
export const defaultShimmerFeedbackPercent = 10;
export const defaultShimmerMixPercent = 20;
export const minShimmerFeedbackPercent = 0;
export const maxShimmerFeedbackPercent = maxPercentValue;
export const minShimmerMixPercent = 0;
export const maxShimmerMixPercent = maxPercentValue;

/** @param {number} multiplierIndex */
export function clampShimmerDelayMultiplierIndex(multiplierIndex) {
  return Math.min(stepTimingMultiplierCount - 1, Math.max(0, Math.trunc(multiplierIndex)));
}

/** @param {number} feedbackPercent */
export function clampShimmerFeedbackPercent(feedbackPercent) {
  return Math.min(maxShimmerFeedbackPercent, Math.max(minShimmerFeedbackPercent, Math.round(feedbackPercent)));
}

/** @param {number} mixPercent */
export function clampShimmerMixPercent(mixPercent) {
  return Math.min(maxShimmerMixPercent, Math.max(minShimmerMixPercent, Math.round(mixPercent)));
}

/**
 * @param {number} sourceVelocity
 * @param {number} tap
 * @param {number} feedbackPercent
 * @param {number} mixPercent
 */
export function shimmerTapVelocity(sourceVelocity, tap, feedbackPercent, mixPercent) {
  const feedback = clampShimmerFeedbackPercent(feedbackPercent) / 100;
  const mix = clampShimmerMixPercent(mixPercent) / 100;

  return Math.round(sourceVelocity * feedback ** tap * mix);
}

/**
 * Maximum shimmer taps any source can produce (velocity-based, using peak source velocity).
 *
 * @param {number} feedbackPercent
 * @param {number} mixPercent
 * @param {number} [peakSourceVelocity]
 */
export function maxShimmerTapsPerSource(
  feedbackPercent,
  mixPercent,
  peakSourceVelocity = 100,
) {
  let taps = 0;

  for (let tap = 1; tap < 32; tap += 1) {
    if (shimmerTapVelocity(peakSourceVelocity, tap, feedbackPercent, mixPercent) <= 0) break;

    taps = tap;
  }

  return taps;
}

/**
 * Cap combination-mode expansion so shimmer can still append taps afterward.
 *
 * @param {number} totalCapacity
 * @param {object} shimmer
 * @param {boolean} [shimmer.enabled]
 * @param {number} [shimmer.feedbackPercent]
 * @param {number} [shimmer.mixPercent]
 */
export function combinedModeExpansionCap(
  totalCapacity,
  {enabled = false, feedbackPercent = defaultShimmerFeedbackPercent, mixPercent = defaultShimmerMixPercent} = {},
) {
  if (!enabled || totalCapacity <= 0) return totalCapacity;

  const maxTaps = maxShimmerTapsPerSource(feedbackPercent, mixPercent);

  if (maxTaps <= 0) return totalCapacity;

  return Math.floor(totalCapacity / (1 + maxTaps));
}

/**
 * Add delayed octave-up shimmer taps for every incoming note.
 *
 * @param {ScheduledNote[]} events
 * @param {object} params
 * @param {boolean} [params.enabled]
 * @param {number} [params.delayMultiplierIndex]
 * @param {number} [params.feedbackPercent]
 * @param {number} [params.mixPercent]
 * @param {number} [params.pulseIndex]
 * @returns {ScheduledNote[]}
 */
export function applyShimmer(events, {enabled = false, delayMultiplierIndex = defaultShimmerDelayMultiplierIndex, feedbackPercent = defaultShimmerFeedbackPercent, mixPercent = defaultShimmerMixPercent, pulseIndex = defaultPulseIndex} = {}) {
  if (!enabled || events.length === 0) return events;

  const delayQuarters = timingMultiplierAtIndex(clampShimmerDelayMultiplierIndex(delayMultiplierIndex)) * pulseQuartersForIndex(pulseIndex);

  if (delayQuarters <= 0) return events;

  /** @type {ScheduledNote[]} */
  const expanded = [...events];

  for (const event of events) {
    const duration = event.end - event.start;

    for (let tap = 1; ; tap += 1) {
      const velocity = shimmerTapVelocity(event.velocity, tap, feedbackPercent, mixPercent);

      if (velocity <= 0) break;

      const midi = event.midi + tap * shimmerSemitoneShift;

      if (midi > maxMidiNote) break;

      const start = event.start + tap * delayQuarters;

      expanded.push({
        ...event,
        start,
        end: start + duration,
        midi,
        velocity,
      });
    }
  }

  return expanded.sort((a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step);
}
