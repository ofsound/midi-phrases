#pragma once
#include "PhrasesModel.h"
#include "PhrasesStyle.h"
namespace phrases::ui
{
    struct PreviewNote
    {
        double start = 0, end = 0;
        int note = 60, velocity = 100, row = 0;
    };
    // Uses an isolated processor instance: the preview executes the same scheduler as
    // audio, including all combinations and processing, without touching the live engine.
    std::vector<PreviewNote> renderPreview (PluginProcessor&, double quarters);
    class Preview : public juce::Component
    {
    public:
        Preview (Controller& controller, Palette& palette) : c (controller), p (palette) { setWantsKeyboardFocus (true); }
        std::function<void()> changed;
        double visibleQuarters = 8, firstQuarter = 0, pixelsPerQuarter = 28;
        int lowNote = 48, highNote = 72;
        void refresh();
        void fit();
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        const std::vector<PreviewNote>& events() const { return notes; }

    private:
        Controller& c;
        Palette& p;
        std::vector<PreviewNote> notes;
        int dragKind = 0;
        double initialStart = 0, initialEnd = 8;
        juce::Point<float> origin;
        float beatX (double) const;
        double xBeat (float) const;
        float pitchLaneHeight() const;
        Rect noteBounds (double, double, int) const;
    };
}
