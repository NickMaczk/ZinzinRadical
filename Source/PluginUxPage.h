#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <functional>

class WavePreviewComponent  : public juce::Component,
                              public juce::FileDragAndDropTarget
{
public:
    WavePreviewComponent();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    bool loadWaveformFromWavFile (const juce::File& file);
    void moveSelectedOnset (int delta);

    juce::Rectangle<float> waveArea;
    std::array<juce::Rectangle<float>, 2> navButtons;

    std::vector<float> waveformPeaks;
    juce::String fileName;
    bool sampleLoaded = false;
    int selectedOnsetIndex = 3;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavePreviewComponent)
};

class XYPadComponent  : public juce::Component
{
public:
    XYPadComponent();

    void setPointColour (juce::Colour newColour);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

private:
    juce::Point<float> toPixelPosition (juce::Point<float> normalised) const;
    juce::Point<float> toNormalisedPosition (juce::Point<float> pixel) const;
    bool isPointAt (juce::Point<float> pixel) const;
    void movePointTo (juce::Point<float> pixel);

    juce::Point<float> point { 0.5f, 0.5f };
    juce::Colour pointColour { juce::Colour::fromHSV (0.0f, 0.75f, 0.95f, 0.95f) };
    bool pointHovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XYPadComponent)
};

class PadGridComponent  : public juce::Component
{
public:
    PadGridComponent();

    std::function<void (int)> onSelectedPadChanged;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

private:
    void updatePadBounds();
    int findPadAt (juce::Point<float> p) const;
    void startPadRename (int index);
    void finishPadRename (bool shouldCommit);

    std::array<juce::Rectangle<float>, 8> padBounds;
    std::array<juce::Rectangle<float>, 8> padRenameButtons;
    std::array<juce::String, 8> padLabels;

    juce::TextEditor padRenameEditor;

    int selectedPad = 0;
    int hoveredPad = -1;
    int hoveredPadRenameButton = -1;
    int renamingPad = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadGridComponent)
};

class SimpleRotaryComponent  : public juce::Component
{
public:
    SimpleRotaryComponent();

    void setValue (float newValue);
    void setBipolar (bool shouldBeBipolar);
    void setAccentColour (juce::Colour newColour);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

private:
    float value = 0.5f;
    float dragStartY = 0.0f;
    float dragStartValue = 0.5f;
    bool bipolar = false;
    juce::Colour accentColour { juce::Colours::white };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleRotaryComponent)
};

class LowerSectionComponent  : public juce::Component
{
public:
    LowerSectionComponent();

    void setRotaryColour (juce::Colour newColour);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

private:
    void drawStepper (juce::Graphics& g, juce::Rectangle<float> bounds, bool hovered);

    std::array<SimpleRotaryComponent, 10> rotaries;
    std::array<juce::Rectangle<float>, 2> steppers;
    int hoveredStepper = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LowerSectionComponent)
};

class PluginUxPage  : public juce::Component
{
public:
    PluginUxPage();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Image guideImage;

    WavePreviewComponent wavePreview;
    XYPadComponent xyPad;
    PadGridComponent padGrid;
    LowerSectionComponent lowerSection;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginUxPage)
};
