#pragma once
#include "PhrasesModel.h"
#include "PhrasesStyle.h"
namespace phrases::ui
{
    struct PreviewNote
    {
        double start = 0, end = 0;
        int note = 60, velocity = 100, row = 0;
        int channel = 1, step = -1;
    };
    // Uses an isolated processor instance: the preview executes the same scheduler as
    // audio, including all combinations and processing, without touching the live engine.
    std::vector<PreviewNote> renderPreview (PluginProcessor&, double endQuarter, double firstQuarter = 0);
    class Preview : public juce::Component
    {
    public:
        Preview (Controller&, Palette&);
        ~Preview() override;
        std::function<void()> changed;
        double visibleQuarters = 8, firstQuarter = 0;
        int lowNote = 48, highNote = 72;
        void refresh();
        void fit();
        void updatePlayback();
        const PlaybackSteps& stepPlayback() const { return playingSteps; }
        const std::vector<PreviewNote>& playbackNotes() const { return liveNotes; }
        const std::vector<PreviewNote>& upcomingNotes() const { return useExtraForecast ? extraForecast : notes; }
        double playheadBeat() const;
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
        std::vector<PreviewNote> extraForecast;
        std::vector<PreviewNote> liveNotes;
        std::vector<size_t> openLiveNotes;
        std::vector<PluginProcessor::PlaybackEvent> playbackEvents;
        PluginProcessor::PlaybackEvent playbackPosition;
        PlaybackSteps playingSteps {};
        double liveFirstQuarter = 0;
        double forecastStart = 0, forecastEnd = 0;
        bool useExtraForecast = false;
        bool followPlayback = true;
        static constexpr float pitchGutterWidth = 44.0f;
        std::optional<double> manualPixelsPerQuarter;
        int dragKind = 0;
        double initialStart = 0, initialEnd = 8;
        juce::Point<float> origin;
        void fitPitch();
        void updateForecast();
        bool isLive() const;
        double viewStart() const;
        double fittedPixelsPerQuarter() const;
        double pixelsPerQuarter() const;
        float beatX (double) const;
        double xBeat (float) const;
        float pitchLaneHeight() const;
        Rect noteBounds (double, double, int) const;
    };
}
