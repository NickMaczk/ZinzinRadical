#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <vector>

class MainComponent;

class PadPoint  : public juce::Component
{
public:
    PadPoint (MainComponent& owner, int pointIndex);

    void paint (juce::Graphics& g) override;
    bool hitTest (int x, int y) override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    void setRotaryMode (bool shouldShowRotary);
    void setRotaryValue (float newValue);
    void setSelectionPulse (float newPulse);
    void setSelectedHighlight (bool shouldBeSelected);
    void setDevianceValue (float newValue);
    void setInstabilityValue (float newValue);
    void setVolumeValue (float newValue);
    void setMotionPhase (float newPhase);
    float getRotaryValue() const;

private:
    MainComponent& owner;
    int index = 0;

    bool rotaryMode = false;
    bool selectedHighlight = false;
    float selectionPulse = 0.0f;
    float rotaryValue = 0.35f;
    float devianceValue = 0.0f;
    float instabilityValue = 0.0f;
    float volumeValue = 1.0f;
    float motionPhase = 0.0f;

    float dragStartY = 0.0f;
    float dragStartValue = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadPoint)
};

class MainComponent  : public juce::Component,
                       public juce::FileDragAndDropTarget,
                       private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    friend class PadPoint;

    void timerCallback() override;

    float getUiScale() const;
    juce::Point<float> toBasePoint (juce::Point<float> point) const;

    bool hasActiveDevianceMotion() const;
    bool hasActiveSelectionPulse() const;

    void updateLayout();
    void activateXYPad();
    void changeSelectedOnset (int direction);
    bool loadWaveformFromWavFile (const juce::File& file);

    void updatePointModes();
    void layoutPadPoints();
    void drawPointConnections (juce::Graphics& g);
    void movePadPoint (int index, juce::Point<float> parentPosition);
    void setPointRotaryValue (int index, float value);
    void selectPadFromPoint (int index, bool triggerPulse);
    void triggerSelectionPulse (int index);
    void startPadRename (int index);
    void finishPadRename (bool shouldCommit);

    void drawEmptyButton (juce::Graphics& g,
                          juce::Rectangle<float> bounds,
                          bool selected,
                          bool hovered,
                          juce::Colour selectedColour = juce::Colour::fromRGB (235, 58, 58));

    void drawMultiDirectionCross (juce::Graphics& g, juce::Rectangle<float> bounds);

    int findClickedButton (const std::array<juce::Rectangle<float>, 8>& buttons,
                           juce::Point<float> point) const;

    int findClickedButton (const std::array<juce::Rectangle<float>, 2>& buttons,
                           juce::Point<float> point) const;

    int findClickedButton (const std::array<juce::Rectangle<float>, 16>& buttons,
                           juce::Point<float> point) const;

    std::array<juce::Rectangle<float>, 8> topButtons;
    std::array<juce::Rectangle<float>, 8> padButtons;
    std::array<juce::Rectangle<float>, 8> padRenameButtons;
    std::array<juce::Rectangle<float>, 16> padUtilityButtons;
    std::array<std::unique_ptr<PadPoint>, 8> xyPoints;

    std::array<juce::Point<float>, 8> pointPositions;
    std::array<juce::Point<float>, 8> pointTargets;
    std::array<std::array<float, 8>, 8> pointRotaryValues {};
    std::array<float, 8> padSelectionPulse {};
    std::array<float, 8> pointSelectionPulse {};
    std::array<juce::String, 8> padLabels {};
    juce::TextEditor padRenameEditor;
    int renamingPad = -1;

    juce::Rectangle<float> xyPad;
    juce::Rectangle<float> samplePlayer;
    std::array<juce::Rectangle<float>, 2> sampleButtons;

    std::vector<float> waveformPeaks;
    juce::String sampleFileName;
    bool sampleLoaded = false;

    juce::Image guideImage;

    int selectedTopButton = 0;
    int selectedPadButton = 0;

    int hoveredTopButton = -1;
    int hoveredPadButton = -1;
    int hoveredSampleButton = -1;
    int selectedSampleButton = -1;
    int selectedOnsetIndex = 3;
    int hoveredPadUtilityButton = -1;
    int hoveredPadRenameButton = -1;
    int pressedPadUtilityButton = -1;

    std::array<bool, 8> padLocks {};

    bool draggingPadExport = false;
    juce::Point<float> exportDragPosition;

    bool xyPadActivated = false;
    float xyAnimationProgress = 0.0f;
    float devianceMotionPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
