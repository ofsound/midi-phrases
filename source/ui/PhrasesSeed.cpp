#include "PhrasesSeed.h"
#include <cmath>
#include <numeric>
namespace phrases::ui
{
    namespace
    {
        int roundJS (double x) { return static_cast<int> (std::floor (x + 0.5)); }
        struct Random
        {
            uint32_t state;
            double next()
            {
                state += 0x6d2b79f5u;
                auto v = (state ^ (state >> 15)) * (state | 1u);
                v ^= v + (v ^ (v >> 7)) * (v | 61u);
                return static_cast<double> (v ^ (v >> 14)) / 4294967296.0;
            }
            int integer (int low, int high) { return static_cast<int> (std::floor (next() * (high - low + 1))) + low; }
        };
        uint32_t rowSeed (uint32_t seed, int row) { return seed + static_cast<uint32_t> (row + 1) * 0x9e3779b9u; }
        uint32_t aspectSeed (int value, int seed, int row, int salt)
        {
            return value > 0 ? static_cast<uint32_t> (value) : std::clamp (rowSeed (static_cast<uint32_t> (seed), row) + static_cast<uint32_t> (salt) * 0x85ebca6bu, 1u, 2147483647u);
        }
        double weight (int step, int phase, int count)
        {
            if ((step + phase) % 4 == 0)
                return 1;
            return count <= 3 ? 0.35 + 0.3 * ((step + phase * 2) % 3) / 2.0 : 0.2;
        }
        struct Contour
        {
            double center, wave;
            int velocity, phase;
        };
    }
    Rows generate (const std::array<PluginProcessor::SeedingRowState, 4>& settings, int rhythm, int root, int mode)
    {
        rhythm = juce::jlimit (0, 7, rhythm);
        uint32_t master = 0;
        for (int r = 0; r < 4; ++r)
            master ^= rowSeed (static_cast<uint32_t> (settings[static_cast<size_t> (r)].seed), r);
        if (master == 0)
            master = 1;
        Random layout { master };
        std::array<int, 4> order { 0, 1, 2, 3 };
        for (int i = 3; i > 0; --i)
            std::swap (order[static_cast<size_t> (i)], order[static_cast<size_t> (layout.integer (0, i))]);
        std::array<Contour, 4> contours;
        for (size_t r = 0; r < 4; ++r)
        {
            const int a = order[r];
            contours[r] = { a - 1.5 + (layout.next() * 2 - 1) * 0.35,
                std::max (1.0, 1.25 + a * 0.28 + (layout.next() * 2 - 1) * 0.1),
                roundJS (78 + a * 6 + (layout.next() * 2 - 1) * 4),
                a };
        }
        constexpr double penalties[] { 0, .02, .05, .07, .09, .12, .14, .16 };
        constexpr double timingAccent[] { 1, .9375, .875, .8125, .75, .6875, .5625, .5 };
        Rows result;
        for (int r = 0; r < 4; ++r)
        {
            const auto& s = settings[static_cast<size_t> (r)];
            const auto& c = contours[static_cast<size_t> (r)];
            Random repetition { rowSeed (aspectSeed (s.repetitionSeed, s.seed, r, 12), r) };
            Random complexity { rowSeed (aspectSeed (s.complexitySeed, s.seed, r, 13), r) };
            Random randomness { rowSeed (aspectSeed (s.randomnessSeed, s.seed, r, 14), r) };
            Random timing { rowSeed (aspectSeed (s.timingVarianceSeed, s.seed, r, 11), r) };
            const int count = juce::jlimit (2, 16, s.phraseLength), motifCount = s.symmetry ? (count + 1) / 2 : count;
            const int half = std::max (2, s.rangeSemitones / 2);
            const double comp = s.complexity / 100.0, rep = s.repetition / 100.0, rnd = s.randomness / 100.0;
            const int center = roundJS (c.center * std::max (1.0, s.rangeSemitones / 6.0));
            const double phase = comp > 0 ? complexity.next() * juce::MathConstants<double>::pi * .5 * comp : 0;
            const double wave = comp > 0 ? (complexity.next() * 2 - 1) * .22 * comp : 0;
            std::vector<int> motif;
            int previous = center;
            for (int i = 0; i < motifCount; ++i)
            {
                const bool reusePrevious = i > 0 && repetition.next() < rep;
                const bool reuseEarlier = i > 3 && repetition.next() < rep * .28;
                if (reusePrevious)
                {
                    motif.push_back (previous);
                    continue;
                }
                if (reuseEarlier)
                {
                    previous = motif[static_cast<size_t> (std::max (0, i - repetition.integer (2, std::min (6, i))))];
                    motif.push_back (previous);
                    continue;
                }
                const double periodic = std::sin (static_cast<double> (i) / std::max (1, motifCount - 1) * juce::MathConstants<double>::pi * c.wave * (1 + wave) + phase);
                const int directed = roundJS (periodic * half * (.35 + comp * .45));
                const int wobble = comp > 0 ? roundJS ((complexity.next() * 2 - 1) * half * comp * .12) : 0;
                const int leap = std::max (0, roundJS (rnd * half * .9));
                const int push = leap > 0 ? randomness.integer (-leap, leap) : 0;
                previous = juce::jlimit (-half, half, center + directed + wobble + push);
                motif.push_back (previous);
            }
            if (s.symmetry)
                for (int i = count - motifCount - 1; i >= 0; --i)
                    motif.push_back (motif[static_cast<size_t> (i)]);
            std::vector<int> lengths (static_cast<size_t> (count), s.timingMeanMultiplierIndex);
            const double variance = s.timingVariance / 100.0;
            if (variance > 0)
            {
                const int target = juce::jlimit (1, 15, s.timingMeanMultiplierIndex);
                const int spread = std::max (1, roundJS (variance * 4));
                const int accent = juce::jlimit (1, 15, roundJS (timingAccent[rhythm] * 4 - 1));
                std::bitset<16> locked;
                for (int i = 0; i < count; ++i)
                {
                    const int base = juce::jlimit (1, 15, target + roundJS ((timing.next() * 2 - 1) * spread));
                    lengths[static_cast<size_t> (i)] = juce::jlimit (1, 15, roundJS (base + (accent - base) * weight (i, c.phase, count) * rhythm / 7.0 * variance));
                }
                if (target > 1 && target < 15)
                {
                    const int sym = std::max (1, std::min ({ spread, target - 1, 15 - target }));
                    const int first = timing.integer (0, count - 1), second = (first + 1 + timing.integer (0, count - 2)) % count;
                    lengths[static_cast<size_t> (first)] = target - sym;
                    lengths[static_cast<size_t> (second)] = target + sym;
                    locked.set (static_cast<size_t> (first));
                    locked.set (static_cast<size_t> (second));
                }
                std::vector<int> ascending (static_cast<size_t> (count));
                std::iota (ascending.begin(), ascending.end(), 0);
                std::stable_sort (ascending.begin(), ascending.end(), [&] (int a, int b) { return weight (a, c.phase, count) < weight (b, c.phase, count); });
                int sum = std::accumulate (lengths.begin(), lengths.end(), 0);
                for (int guard = count * 15; sum != target * count && guard > 0; --guard)
                {
                    const int direction = sum < target * count ? 1 : -1;
                    bool changed = false;
                    for (int j = 0; j < count; ++j)
                    {
                        const auto i = static_cast<size_t> (ascending[static_cast<size_t> (direction > 0 ? j : count - j - 1)]);
                        if (locked[i] || lengths[i] + direction < 1 || lengths[i] + direction > 15)
                            continue;
                        lengths[i] += direction;
                        sum += direction;
                        changed = true;
                        break;
                    }
                    if (!changed)
                        break;
                }
            }
            auto& row = result[static_cast<size_t> (r)];
            row.channel = r + 1;
            row.steps.resize (static_cast<size_t> (count));
            const int centerMidi = snapNote (s.centerMidi < 0 ? 60 + root : s.centerMidi, root, mode);
            for (int i = 0; i < count; ++i)
            {
                auto& step = row.steps[static_cast<size_t> (i)];
                step.note = snapNote (centerMidi + motif[static_cast<size_t> (i)], root, mode);
                step.timing = lengths[static_cast<size_t> (i)];
                const double accent = ((i + c.phase) % 4 == 0 ? .95 : .82) + (randomness.next() * .18 - .09) * rnd;
                step.duration = roundJS (juce::jlimit (.35, 1.0, accent - penalties[rhythm] * weight (i, c.phase, count)) * 100) / 100.0;
            }
            const int swing = roundJS (10 + comp * 28), noise = roundJS (rnd * 12);
            for (int i = 0; i < count; ++i)
                row.steps[static_cast<size_t> (i)].velocity = juce::jlimit (28, 127, roundJS (c.velocity + std::sin ((i + c.phase) * 1.7) * swing + (noise > 0 ? randomness.integer (-noise, noise) : 0)));
            for (int i = 0; i < count; ++i)
                row.steps[static_cast<size_t> (i)].skipped = comp < .3 && complexity.next() < .08 && i % 4 != c.phase % 4;
            for (int i = 0; i < count; ++i)
                row.steps[static_cast<size_t> (i)].probability = rnd > 0 && randomness.next() < rnd * .5 && i % 4 != 0 ? juce::jlimit (20, 100, roundJS (100 - rnd * randomness.integer (18, 42))) : 100;
        }
        Random offsetRandom { master };
        constexpr double fractions[] { 0, .25, .375, .5 };
        if (rhythm > 0)
            for (size_t r = 0; r < 4; ++r)
            {
                double length = 0;
                for (const auto& s : result[r].steps)
                    if (!s.skipped)
                        length += (s.timing + 1) * .25;
                const auto fraction = fractions[order[r]];
                const double jitter = fraction <= 0 ? 0 : (offsetRandom.next() * 2 - 1) * .125;
                result[r].offset = roundJS (juce::jlimit (0.0, 2.0, fraction * std::max (.25, length) * rhythm / 7.0 + jitter) * 4) + 8;
            }
        return result;
    }
}
