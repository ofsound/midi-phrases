#pragma once
#include "../PluginProcessor.h"
#include <bitset>
#include <deque>

namespace phrases::ui
{
    struct Step
    {
        int note = 60, timing = 3, velocity = 100, probability = 100, cycle = 1, mask = 1;
        double duration = 1;
        bool muted = false, skipped = false;
        bool operator== (const Step& other) const
        {
            return note == other.note && timing == other.timing && velocity == other.velocity
                   && probability == other.probability && cycle == other.cycle && mask == other.mask
                   && std::abs (duration - other.duration) < 1.0e-9 && muted == other.muted && skipped == other.skipped;
        }
    };
    struct Row
    {
        std::vector<Step> steps;
        int offset = 8, channel = 1;
        bool muted = false;
        bool operator== (const Row&) const = default;
    };
    using Rows = std::array<Row, 4>;
    using Selection = std::array<std::bitset<64>, 4>;
    Rows readRows (const PluginProcessor&);
    void writeRow (PluginProcessor&, int row, const Row&);
    bool stepActive (const Row&, size_t step, double beat, int pulse, int swing, int subdivision);
    int scaleNote (int note, int degrees, int root, int mode);
    int snapNote (int note, int root, int mode);
    juce::String noteName (int note);
    inline const juce::StringArray scaleNames { "Chromatic", "Ionian", "Aeolian", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian", "Harmonic Minor", "Melodic Minor", "Maj Pentatonic", "Min Pentatonic", "Blues", "Whole Tone", "Whole-Half", "Half-Whole" };

    class Controller
    {
    public:
        explicit Controller (PluginProcessor& processor) : p (processor), rows (readRows (p)), knownGeneration (p.getDocumentGeneration()) {}
        PluginProcessor& p;
        Rows rows;
        Selection selection {};
        bool begin();
        bool commit();
        void cancel();
        bool undo();
        bool redo();
        bool canUndo() const { return !undoHistory.empty(); }
        bool canRedo() const { return !redoHistory.empty(); }
        bool refresh();
        void documentChanged();
        void selectAll (int row = -1);
        void clearSelection() { selection = {}; }
        bool hasSelection() const;
        void editSelected (const std::function<void (Step&)>&, bool allIfEmpty = false);
        void removeSelected();
        void moveSelected (int sourceRow, int sourceStep, int targetRow, int insertion, bool copy);
        bool gestureActive() const { return gesture && generation == p.getDocumentGeneration() && gesturePattern == p.getViewPatternSlot(); }

    private:
        struct Entry
        {
            juce::MemoryBlock before, after;
        };
        std::deque<Entry> undoHistory, redoHistory;
        juce::MemoryBlock before;
        int gesturePattern = -1;
        uint64_t generation = 0, knownGeneration = 0;
        bool gesture = false;
        juce::MemoryBlock state();
        bool restore (std::deque<Entry>&, std::deque<Entry>&, bool forward);
    };
}
