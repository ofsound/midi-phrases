#pragma once
#include "PhrasesModel.h"
#include "PhrasesStyle.h"

namespace phrases::ui
{
    class CycleEditor : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        CycleEditor (Controller&, Palette&);
        std::function<void()> changed;
        void setInspectedStep (int row, int step);
        void paint (juce::Graphics&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        bool keyPressed (const juce::KeyPress&) override;

    private:
        static constexpr int beatCount = 8;
        Controller& controller;
        Palette& palette;
        int row = -1, step = -1;
        int pressedBeat = -1, dropBeat = -1, hoveredBeat = -1, lastLength = -1;
        bool resizing = false, movingBeat = false, handleHovered = false;
        Rows baseline;
        Selection selection;
        const Step* inspected() const;
        Rect beatBounds (int) const;
        Rect handleBounds (int cycle) const;
        int beatAt (juce::Point<float>) const;
        bool beginEdit();
        bool canEdit() const;
        void resizeCycle (int);
        void finishEdit();
        void cancelEdit();
        void resetPointer();
    };
}
