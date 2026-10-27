#pragma once
#include "PhrasesModel.h"
#include "PhrasesStyle.h"

namespace phrases::ui
{
    class Grid : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        Grid (Controller&, Palette&);
        std::function<void (int, int)> inspect;
        std::function<void (int)> editRow, seedRow, recordRow;
        std::function<void()> changed;
        bool compact = false;
        int inspectedRow = -1, inspectedStep = -1;
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        bool keyPressed (const juce::KeyPress&) override;
        Rect stepBounds (int row, int step) const;
        void updateLayout();
        void cancelGesture();
        void presentFlip (int row, int step);
        bool flipsActive() const;
        float flipShown (int row, int step);

    private:
        Controller& c;
        Palette& p;
        std::array<std::vector<Rect>, 4> cells;
        std::array<Rect, 4> addBounds;
        std::array<float, 4> scroll {};
        std::array<std::unique_ptr<juce::Drawable>, 6> icons;
        std::array<juce::Colour, 6> iconInks {juce::Colours::white, juce::Colours::white, juce::Colours::white, juce::Colours::white, juce::Colours::white, juce::Colours::white};
        enum class Part { none,
            header,
            power,
            offset,
            record,
            rowEdit,
            seed,
            add,
            move,
            duration,
            note,
            velocity,
            skip,
            mute,
            gear,
            remove,
            resize };
        struct Hit
        {
            Part part = Part::none;
            int row = -1, step = -1;
        };
        Hit hit (juce::Point<float>) const;
        Hit down;
        Rows baseline;
        Selection selectionBefore;
        juce::Point<float> origin, pointer;
        uint64_t downGeneration = 0;
        int downPattern = -1;
        bool dragging = false, marquee = false, compactEdited = false;
        float unit = 76.5f;
        struct FlipAnim
        {
            float from = 0, to = 0;
            double startMs = 0;
        };
        std::array<std::array<FlipAnim, 64>, 4> flipAnims;
        Hit hover;
        void drawIcon (juce::Graphics&, size_t, Rect, juce::Colour);
        void drawStepBack (juce::Graphics&, Rect, const Step&, juce::Colour, bool compactCard);
        void finish();
    };
}
