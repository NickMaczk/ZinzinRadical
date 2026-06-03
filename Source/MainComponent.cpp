#include "MainComponent.h"

#include <cstring>
#include <cstdint>

namespace
{
    constexpr float pointComponentSize = 280.0f;
    constexpr float pointDotSize = 18.0f;
    constexpr float rotarySize = 48.0f;

    juce::File findGuideImageFile()
    {
        auto dir = juce::File::getCurrentWorkingDirectory();

        for (int i = 0; i < 8; ++i)
        {
            auto candidate = dir.getChildFile ("Assets").getChildFile ("guide.png");

            if (candidate.existsAsFile())
                return candidate;

            dir = dir.getParentDirectory();
        }

        return {};
    }

    bool hasTag (const char* data, size_t size, size_t offset, const char* tag)
    {
        return offset + 4 <= size && std::memcmp (data + offset, tag, 4) == 0;
    }

    uint16_t readU16LE (const char* data, size_t offset)
    {
        return (uint16_t) ((uint8_t) data[offset] | ((uint8_t) data[offset + 1] << 8));
    }

    uint32_t readU32LE (const char* data, size_t offset)
    {
        return (uint32_t) ((uint8_t) data[offset]
             | ((uint8_t) data[offset + 1] << 8)
             | ((uint8_t) data[offset + 2] << 16)
             | ((uint8_t) data[offset + 3] << 24));
    }

    float readSampleValue (const char* p, uint16_t audioFormat, uint16_t bitsPerSample)
    {
        if (audioFormat == 3 && bitsPerSample == 32)
        {
            float v = 0.0f;
            std::memcpy (&v, p, sizeof (float));
            return juce::jlimit (-1.0f, 1.0f, v);
        }

        if (audioFormat != 1)
            return 0.0f;

        if (bitsPerSample == 16)
        {
            const auto v = (int16_t) ((uint8_t) p[0] | ((uint8_t) p[1] << 8));
            return (float) v / 32768.0f;
        }

        if (bitsPerSample == 24)
        {
            int32_t v = ((uint8_t) p[0])
                      | ((uint8_t) p[1] << 8)
                      | ((uint8_t) p[2] << 16);

            if (v & 0x800000)
                v |= ~0xFFFFFF;

            return (float) v / 8388608.0f;
        }

        if (bitsPerSample == 32)
        {
            const int32_t v = (int32_t) readU32LE (p, 0);
            return (float) v / 2147483648.0f;
        }

        return 0.0f;
    }

    juce::Colour getTopButtonColour (int index)
    {
        switch (index)
        {
            case 1:  return juce::Colour::fromRGB (255, 43, 214);  // pitch / hot pink
            case 2:  return juce::Colour::fromRGB (255, 91, 31);   // length / sanguine orange
            case 3:  return juce::Colour::fromRGB (0, 229, 255);   // super / electric cyan
            case 4:  return juce::Colour::fromRGB (155, 92, 255);  // deviance / violet
            case 5:  return juce::Colour::fromRGB (178, 255, 36);  // instability / acid lime
            case 6:  return juce::Colour::fromRGB (56, 130, 255);  // pan / electric blue
            case 7:  return juce::Colour::fromRGB (255, 218, 46);  // vol / hot yellow
            default: return juce::Colour::fromRGB (235, 58, 58);
        }
    }

    juce::Colour getPadColour (int index)
    {
        switch (index)
        {
            case 0:  return juce::Colour::fromRGB (0, 229, 255);
            case 1:  return juce::Colour::fromRGB (255, 43, 214);
            case 2:  return juce::Colour::fromRGB (255, 91, 31);
            case 3:  return juce::Colour::fromRGB (178, 255, 36);
            case 4:  return juce::Colour::fromRGB (155, 92, 255);
            case 5:  return juce::Colour::fromRGB (56, 130, 255);
            case 6:  return juce::Colour::fromRGB (255, 218, 46);
            case 7:  return juce::Colour::fromRGB (255, 58, 92);
            default: return juce::Colours::white;
        }
    }
}

PadPoint::PadPoint (MainComponent& ownerRef, int pointIndex)
    : owner (ownerRef),
      index (pointIndex)
{
    setInterceptsMouseClicks (true, false);
}

void PadPoint::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const auto activeColour = getTopButtonColour (owner.selectedTopButton);
    const auto padColour = getPadColour (index);

    const float dotMotionRadius = juce::jmap (devianceValue, 0.0f, 1.0f, 0.0f, 80.0f);

    const auto smoothOffset = juce::Point<float> (
        std::sin (motionPhase * 1.17f + (float) index * 1.91f) * dotMotionRadius,
        std::cos (motionPhase * 0.83f + (float) index * 2.37f) * dotMotionRadius);

    auto random01 = [] (float seed)
    {
        auto v = std::sin (seed) * 43758.5453f;
        return v - std::floor (v);
    };

    const float jumpSpeed = juce::jmap (instabilityValue, 0.0f, 1.0f, 0.8f, 8.0f);
    const float jumpTime = motionPhase * jumpSpeed + (float) index * 7.13f;
    const float jumpStep = std::floor (jumpTime);
    const float jumpBlend = juce::jlimit (0.0f, 1.0f, jumpTime - jumpStep);

    const auto smoothStep = jumpBlend * jumpBlend * (3.0f - 2.0f * jumpBlend);

    auto randomPoint = [&] (float seed)
    {
        const float angle = random01 (seed + 11.3f) * juce::MathConstants<float>::twoPi;
        const float radius = std::sqrt (random01 (seed + 91.7f)) * dotMotionRadius;

        return juce::Point<float> (std::cos (angle) * radius,
                                   std::sin (angle) * radius);
    };

    const auto jumpA = randomPoint (jumpStep + (float) index * 19.0f);
    const auto jumpB = randomPoint (jumpStep + 1.0f + (float) index * 19.0f);
    auto erraticOffset = jumpA + (jumpB - jumpA) * smoothStep;

    const float bump = std::sin (motionPhase * 22.0f + (float) index) * dotMotionRadius * 0.18f * instabilityValue;
    erraticOffset += juce::Point<float> (bump, -bump * 0.6f);

    const auto dotOffset = smoothOffset + (erraticOffset - smoothOffset) * instabilityValue;

    if (rotaryMode)
    {
        const auto rotaryArea = juce::Rectangle<float> (rotarySize, rotarySize)
                                    .withCentre (centre);

        if (owner.selectedTopButton == 1 || owner.selectedTopButton == 6)
        {
            const float radius = rotarySize * 0.5f;
            const bool isPitch = owner.selectedTopButton == 1;

            if (isPitch)
            {
                const int dashCount = 24;
                const float dashRatio = 0.48f;

                g.setColour (juce::Colours::white.withAlpha (0.22f));

                for (int i = 0; i < dashCount; ++i)
                {
                    const float a0 = juce::MathConstants<float>::twoPi * ((float) i / (float) dashCount);
                    const float a1 = juce::MathConstants<float>::twoPi * (((float) i + dashRatio) / (float) dashCount);

                    g.drawLine (centre.x + std::cos (a0) * radius,
                                centre.y + std::sin (a0) * radius,
                                centre.x + std::cos (a1) * radius,
                                centre.y + std::sin (a1) * radius,
                                2.0f);
                }
            }
            else
            {
                g.setColour (juce::Colours::white.withAlpha (0.18f));
                g.drawEllipse (rotaryArea, 2.0f);
            }

            const float bipolarValue = (rotaryValue - 0.5f) * 2.0f;
            const float top = 0.0f;

            if (std::abs (bipolarValue) > 0.01f)
            {
                juce::Path valueArc;

                if (bipolarValue > 0.0f)
                    valueArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                                            top, top + bipolarValue * juce::MathConstants<float>::pi, true);
                else
                    valueArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                                            top + bipolarValue * juce::MathConstants<float>::pi, top, true);

                g.setColour (activeColour);
                g.strokePath (valueArc, juce::PathStrokeType (3.2f));
            }

            g.setColour (activeColour);
            g.fillEllipse (centre.x - 2.0f, centre.y - radius - 2.0f, 4.0f, 4.0f);

            if (isPitch)
            {
                g.setColour (juce::Colours::white.withAlpha (0.58f));
                g.setFont (juce::FontOptions (9.5f));
                g.drawFittedText ("-6", rotaryArea.translated (-18.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft, 1);
                g.drawFittedText ("+6", rotaryArea.translated (18.0f, 0.0f).toNearestInt(), juce::Justification::centredRight, 1);
            }
        }
        else
        {
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.drawEllipse (rotaryArea, 2.0f);

            juce::Path valueArc;
            const float start = -juce::MathConstants<float>::halfPi;
            const float end = start + juce::MathConstants<float>::twoPi * rotaryValue;

            valueArc.addCentredArc (centre.x,
                                    centre.y,
                                    rotarySize * 0.5f,
                                    rotarySize * 0.5f,
                                    0.0f,
                                    start,
                                    end,
                                    true);

            g.setColour (activeColour);
            g.strokePath (valueArc, juce::PathStrokeType (3.0f));
        }
    }

    const float visualDotSize = juce::jmap (volumeValue, 0.0f, 1.0f, 8.0f, pointDotSize);

    const auto devianceBaseArea = juce::Rectangle<float> (pointDotSize, pointDotSize)
                                      .withCentre (centre);

    if (devianceValue > 0.01f)
    {
        const float haloExpand = juce::jmap (devianceValue, 0.0f, 1.0f, 20.0f, 112.0f);
        const auto haloArea = devianceBaseArea.expanded (haloExpand);

        g.setColour (getTopButtonColour (4).withAlpha (0.055f));
        g.fillEllipse (haloArea);

        g.setColour (getTopButtonColour (4).withAlpha (0.22f));
        g.drawEllipse (haloArea, 1.0f);
    }

    const auto ghostDotArea = juce::Rectangle<float> (visualDotSize, visualDotSize)
                                  .withCentre (centre + dotOffset);

    const auto dotArea = juce::Rectangle<float> (visualDotSize, visualDotSize)
                             .withCentre (centre);

    if (devianceValue > 0.01f)
    {
        g.setColour (padColour.withAlpha (0.28f));
        g.fillEllipse (ghostDotArea);

        g.setColour (padColour.withAlpha (0.58f));
        g.drawEllipse (ghostDotArea, 1.2f);
    }

    if (selectedHighlight)
    {
        const auto selectedArea = dotArea.expanded (10.0f);

        g.setColour (juce::Colours::white.withAlpha (0.13f));
        g.fillEllipse (selectedArea);

        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawEllipse (selectedArea, 1.5f);
    }

    if (selectionPulse > 0.01f)
    {
        const float pulseExpansion = 32.0f + (1.0f - selectionPulse) * 48.0f;
        const auto pulseArea = dotArea.expanded (pulseExpansion);

        g.setColour (juce::Colours::white.withAlpha (0.36f * selectionPulse));
        g.fillEllipse (pulseArea);
    }

    g.setColour (padColour.withAlpha (0.92f));
    g.fillEllipse (dotArea);

    g.setColour (selectionPulse > 0.01f ? juce::Colours::white
                                         : padColour.brighter (0.35f));
    g.drawEllipse (dotArea, selectionPulse > 0.01f ? 2.2f : 1.5f);

    g.setColour (juce::Colours::white.withAlpha (0.72f));
    g.setFont (juce::FontOptions (10.0f));
    g.drawFittedText (owner.padLabels[(size_t) index],
                      juce::Rectangle<int> ((int) centre.x + 13,
                                            (int) centre.y - 8,
                                            48,
                                            16),
                      juce::Justification::centredLeft,
                      1);
}

bool PadPoint::hitTest (int x, int y)
{
    const auto centre = getLocalBounds().toFloat().getCentre();
    const auto p = juce::Point<float> ((float) x, (float) y);

    if (rotaryMode)
        return p.getDistanceFrom (centre) <= rotarySize * 0.5f;

    return p.getDistanceFrom (centre) <= pointDotSize * 1.5f;
}

void PadPoint::mouseDown (const juce::MouseEvent& e)
{
    owner.selectPadFromPoint (index, ! rotaryMode);

    if (auto* parent = getParentComponent())
        dragStartY = e.getEventRelativeTo (parent).position.y;

    dragStartValue = rotaryValue;
}

void PadPoint::mouseDrag (const juce::MouseEvent& e)
{
    if (auto* parent = getParentComponent())
    {
        const auto parentPos = e.getEventRelativeTo (parent).position;

        if (rotaryMode)
        {
            const float delta = dragStartY - parentPos.y;
            owner.setPointRotaryValue (index, dragStartValue + delta * 0.006f);
        }
        else
        {
            owner.movePadPoint (index, parentPos);
        }
    }
}

void PadPoint::setRotaryMode (bool shouldShowRotary)
{
    rotaryMode = shouldShowRotary;
    repaint();
}

void PadPoint::setRotaryValue (float newValue)
{
    rotaryValue = juce::jlimit (0.0f, 1.0f, newValue);
    repaint();
}

void PadPoint::setSelectionPulse (float newPulse)
{
    selectionPulse = juce::jlimit (0.0f, 1.0f, newPulse);
    repaint();
}

void PadPoint::setSelectedHighlight (bool shouldBeSelected)
{
    selectedHighlight = shouldBeSelected;
    repaint();
}

void PadPoint::setDevianceValue (float newValue)
{
    devianceValue = juce::jlimit (0.0f, 1.0f, newValue);
    repaint();
}

void PadPoint::setInstabilityValue (float newValue)
{
    instabilityValue = juce::jlimit (0.0f, 1.0f, newValue);
    repaint();
}

void PadPoint::setVolumeValue (float newValue)
{
    volumeValue = juce::jlimit (0.0f, 1.0f, newValue);
    repaint();
}

void PadPoint::setMotionPhase (float newPhase)
{
    motionPhase = newPhase;
    repaint();
}

float PadPoint::getRotaryValue() const
{
    return rotaryValue;
}

//==============================================================================
MainComponent::MainComponent()
{
    guideImage = juce::ImageFileFormat::loadFrom (findGuideImageFile());

    for (int i = 0; i < 8; ++i)
    {
        xyPoints[(size_t) i] = std::make_unique<PadPoint> (*this, i);
        addAndMakeVisible (*xyPoints[(size_t) i]);
        xyPoints[(size_t) i]->setVisible (false);
    }

    for (auto& valuesForTopButton : pointRotaryValues)
        valuesForTopButton.fill (0.35f);

    pointRotaryValues[1].fill (0.5f);
    pointRotaryValues[4].fill (0.0f);
    pointRotaryValues[6].fill (0.5f);
    pointRotaryValues[5].fill (0.0f);
    pointRotaryValues[7].fill (1.0f);

    for (int i = 0; i < 8; ++i)
        padLabels[(size_t) i] = "pad " + juce::String (i + 1);

    addAndMakeVisible (padRenameEditor);
    padRenameEditor.setVisible (false);
    padRenameEditor.setSelectAllWhenFocused (true);
    padRenameEditor.setJustification (juce::Justification::centredLeft);
    padRenameEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha (0.82f));
    padRenameEditor.setColour (juce::TextEditor::textColourId, juce::Colours::white);
    padRenameEditor.setColour (juce::TextEditor::outlineColourId, juce::Colours::white.withAlpha (0.25f));
    padRenameEditor.onReturnKey = [this] { finishPadRename (true); };
    padRenameEditor.onEscapeKey = [this] { finishPadRename (false); };
    padRenameEditor.onFocusLost = [this] { finishPadRename (true); };

    setSize (632, 944);
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (18, 18, 20));

    if (guideImage.isValid())
    {
        g.drawImage (guideImage,
                     getLocalBounds().toFloat(),
                     juce::RectanglePlacement::stretchToFit);
    }

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (juce::FontOptions (13.0f));

    for (int i = 0; i < 8; ++i)
    {
        drawEmptyButton (g,
                         topButtons[(size_t) i],
                         selectedTopButton == i,
                         hoveredTopButton == i,
                         getTopButtonColour (i));

        if (i > 0)
        {
            const auto valueArea = topButtons[(size_t) i].reduced (8.0f, 8.0f);
            const bool bipolar = i == 1 || i == 6;
            const float laneWidth = valueArea.getWidth() / 8.0f;
            const float barWidth = 3.0f;

            if (bipolar)
            {
                g.setColour (juce::Colours::white.withAlpha (0.18f));
                g.drawLine (valueArea.getX(),
                            valueArea.getCentreY(),
                            valueArea.getRight(),
                            valueArea.getCentreY(),
                            1.0f);
            }

            for (int pad = 0; pad < 8; ++pad)
            {
                const float value = pointRotaryValues[(size_t) i][(size_t) pad];
                const float x = valueArea.getX() + laneWidth * (float) pad + laneWidth * 0.5f - barWidth * 0.5f;

                g.setColour (getPadColour (pad).withAlpha (0.90f));

                if (bipolar)
                {
                    const float signedValue = (value - 0.5f) * 2.0f;
                    const float centreY = valueArea.getCentreY();
                    const float h = std::abs (signedValue) * valueArea.getHeight() * 0.5f;

                    if (signedValue >= 0.0f)
                        g.fillRect (juce::Rectangle<float> (x, centreY - h, barWidth, h));
                    else
                        g.fillRect (juce::Rectangle<float> (x, centreY, barWidth, h));
                }
                else
                {
                    const float h = value * valueArea.getHeight();
                    g.fillRect (juce::Rectangle<float> (x,
                                                        valueArea.getBottom() - h,
                                                        barWidth,
                                                        h));
                }
            }
        }
    }

    if (sampleLoaded && ! waveformPeaks.empty())
    {
        const auto waveArea = samplePlayer;
        const auto centreY = waveArea.getCentreY();
        const auto count = (int) waveformPeaks.size();

        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawLine (waveArea.getX(), centreY, waveArea.getRight(), centreY, 1.0f);

        g.setColour (juce::Colours::white.withAlpha (0.25f));

        for (int i = 0; i < count; ++i)
        {
            const float x = juce::jmap ((float) i,
                                        0.0f,
                                        (float) juce::jmax (1, count - 1),
                                        waveArea.getX(),
                                        waveArea.getRight());

            const float h = juce::jmax (1.0f, waveformPeaks[(size_t) i] * waveArea.getHeight() * 0.5f);
            g.drawLine (x, centreY - h, x, centreY + h, 2.0f);
        }

        const std::array<float, 8> onsetPositions {
            0.07f, 0.18f, 0.31f, 0.44f, 0.56f, 0.69f, 0.82f, 0.94f
        };

        {
            const float leftEdge = waveArea.getX() + waveArea.getWidth() * onsetPositions[(size_t) selectedOnsetIndex];

            const float rightEdge = selectedOnsetIndex == 7
                ? waveArea.getRight()
                : waveArea.getX() + waveArea.getWidth() * onsetPositions[(size_t) selectedOnsetIndex + 1];

            g.setColour (juce::Colours::white.withAlpha (0.33f));
            g.fillRect (juce::Rectangle<float> (leftEdge,
                                                waveArea.getY(),
                                                rightEdge - leftEdge,
                                                waveArea.getHeight()));
        }

        for (int i = 0; i < 8; ++i)
        {
            const float x = waveArea.getX() + waveArea.getWidth() * onsetPositions[(size_t) i];

            g.setColour (juce::Colours::white.withAlpha (0.42f));
            g.drawLine (x,
                        waveArea.getY() + 4.0f,
                        x,
                        waveArea.getBottom() - 4.0f,
                        2.0f);
        }

    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.52f));
        g.setFont (juce::FontOptions (15.0f));
        g.drawFittedText ("drop a sample here",
                          samplePlayer.toNearestInt(),
                          juce::Justification::centred,
                          1);
    }

    for (int i = 0; i < 2; ++i)
    {
        drawEmptyButton (g,
                         sampleButtons[(size_t) i],
                         false,
                         hoveredSampleButton == i,
                         juce::Colours::white);
    }

    for (int i = 0; i < 8; ++i)
    {
        const auto padLabelArea = juce::Rectangle<float> (48.0f, 16.0f)
                                      .withPosition (padButtons[(size_t) i].getX(),
                                                     padButtons[(size_t) i].getY() - 16.0f);

        g.setColour (juce::Colours::white.withAlpha (0.72f));
        g.setFont (juce::FontOptions (10.0f));
        g.drawFittedText (padLabels[(size_t) i],
                          padLabelArea.toNearestInt(),
                          juce::Justification::centredLeft,
                          1);

        if (hoveredPadRenameButton == i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.fillRect (padRenameButtons[(size_t) i]);
        }

        const auto padAccentArea = padButtons[(size_t) i].reduced (4.0f);

        if (selectedPadButton == i)
        {
            g.setColour (getPadColour (i));
            g.fillRect (padAccentArea.reduced (10.0f));
        }

        g.setColour (getPadColour (i).withAlpha (0.95f));
        g.drawRect (padAccentArea.toNearestInt(), 4);

        drawEmptyButton (g,
                         padButtons[(size_t) i],
                         false,
                         hoveredPadButton == i,
                         getPadColour (i));

        if (padSelectionPulse[(size_t) i] > 0.01f)
        {
            const auto pulse = padSelectionPulse[(size_t) i];

            g.setColour (juce::Colours::white.withAlpha (0.36f * pulse));
            g.fillRect (padButtons[(size_t) i].reduced (1.0f));
        }
    }

    for (int pad = 0; pad < 8; ++pad)
    {
        const int lockIndex = pad * 2;
        const int dragIndex = lockIndex + 1;

        const auto lockButton = padUtilityButtons[(size_t) lockIndex];
        const auto dragButton = padUtilityButtons[(size_t) dragIndex];

        if (hoveredPadUtilityButton == lockIndex || padLocks[(size_t) pad])
        {
            g.setColour (juce::Colours::white.withAlpha (padLocks[(size_t) pad] ? 0.22f : 0.08f));
            g.fillRect (lockButton);

        }

        if (hoveredPadUtilityButton == dragIndex)
        {
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.fillRect (dragButton);
        }
    }
}

void MainComponent::paintOverChildren (juce::Graphics& g)
{
    drawPointConnections (g);

    if (draggingPadExport)
    {
        const auto dragImage = juce::Rectangle<float> (32.0f, 32.0f)
                                   .withCentre (exportDragPosition);

        g.setColour (juce::Colours::white.withAlpha (0.88f));
        g.fillRect (dragImage);
    }
}

void MainComponent::resized()
{
    updateLayout();
    layoutPadPoints();
}

void MainComponent::mouseDown (const juce::MouseEvent& e)
{
    const auto point = e.position;

    if (const auto button = findClickedButton (topButtons, point); button >= 0)
    {
        selectedTopButton = button;
        updatePointModes();
        repaint();
        return;
    }

    if (const auto button = findClickedButton (sampleButtons, point); button >= 0)
    {
        changeSelectedOnset (button == 0 ? -1 : 1);
        return;
    }

    if (const auto button = findClickedButton (padRenameButtons, point); button >= 0)
    {
        startPadRename (button);
        return;
    }

    if (const auto button = findClickedButton (padButtons, point); button >= 0)
    {
        selectedPadButton = button;
        triggerSelectionPulse (button);
        updatePointModes();
        repaint();
        return;
    }

    if (const auto button = findClickedButton (padUtilityButtons, point); button >= 0)
    {
        pressedPadUtilityButton = button;

        if ((button % 2) == 0)
        {
            const int padIndex = button / 2;
            padLocks[(size_t) padIndex] = ! padLocks[(size_t) padIndex];
            repaint();
            return;
        }

        exportDragPosition = point;
        repaint();
        return;
    }
}

void MainComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (pressedPadUtilityButton >= 0 && (pressedPadUtilityButton % 2) == 1)
    {
        draggingPadExport = true;
        exportDragPosition = e.position;
        repaint();
    }
}

void MainComponent::mouseUp (const juce::MouseEvent&)
{
    if (pressedPadUtilityButton != -1 || draggingPadExport)
    {
        pressedPadUtilityButton = -1;
        draggingPadExport = false;
        repaint();
    }
}

void MainComponent::mouseMove (const juce::MouseEvent& e)
{
    const auto point = e.position;

    const auto newHoveredTopButton = findClickedButton (topButtons, point);
    const auto newHoveredPadButton = findClickedButton (padButtons, point);
    const auto newHoveredSampleButton = findClickedButton (sampleButtons, point);
    const auto newHoveredPadUtilityButton = findClickedButton (padUtilityButtons, point);
    const auto newHoveredPadRenameButton = findClickedButton (padRenameButtons, point);

    if (hoveredTopButton != newHoveredTopButton
        || hoveredPadButton != newHoveredPadButton
        || hoveredSampleButton != newHoveredSampleButton
        || hoveredPadUtilityButton != newHoveredPadUtilityButton
        || hoveredPadRenameButton != newHoveredPadRenameButton)
    {
        hoveredTopButton = newHoveredTopButton;
        hoveredPadButton = newHoveredPadButton;
        hoveredSampleButton = newHoveredSampleButton;
        hoveredPadUtilityButton = newHoveredPadUtilityButton;
        hoveredPadRenameButton = newHoveredPadRenameButton;
        repaint();
    }
}

void MainComponent::mouseExit (const juce::MouseEvent&)
{
    if (hoveredTopButton != -1 || hoveredPadButton != -1 || hoveredSampleButton != -1 || hoveredPadUtilityButton != -1 || hoveredPadRenameButton != -1)
    {
        hoveredTopButton = -1;
        hoveredPadButton = -1;
        hoveredSampleButton = -1;
        hoveredPadUtilityButton = -1;
        hoveredPadRenameButton = -1;
        repaint();
    }
}

bool MainComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& path : files)
        if (juce::File (path).hasFileExtension ("wav"))
            return true;

    return false;
}

void MainComponent::filesDropped (const juce::StringArray& files, int x, int y)
{
    if (! samplePlayer.contains (juce::Point<float> ((float) x, (float) y)))
        return;

    if (files.isEmpty())
        return;

    if (loadWaveformFromWavFile (juce::File (files[0])))
    {
        activateXYPad();
        repaint();
    }
}

void MainComponent::changeSelectedOnset (int direction)
{
    selectedOnsetIndex = (selectedOnsetIndex + direction + 8) % 8;

    if (xyPadActivated)
    {
        const auto centre = xyPad.getCentre();

        for (int i = 0; i < 8; ++i)
            pointPositions[(size_t) i] = centre;

        xyAnimationProgress = 0.0f;
        layoutPadPoints();

        if (! isTimerRunning())
            startTimerHz (60);
    }

    repaint();
}

void MainComponent::timerCallback()
{
    if (xyAnimationProgress < 1.0f)
    {
        xyAnimationProgress = juce::jmin (1.0f, xyAnimationProgress + 0.055f);

        const float eased = 1.0f - std::pow (1.0f - xyAnimationProgress, 3.0f);
        const auto centre = xyPad.getCentre();

        for (int i = 0; i < 8; ++i)
            pointPositions[(size_t) i] = centre + (pointTargets[(size_t) i] - centre) * eased;

        layoutPadPoints();
    }

    devianceMotionPhase += 0.045f;

    for (int i = 0; i < 8; ++i)
    {
        padSelectionPulse[(size_t) i] = juce::jmax (0.0f, padSelectionPulse[(size_t) i] - 1.0f / 16.0f);
        pointSelectionPulse[(size_t) i] = juce::jmax (0.0f, pointSelectionPulse[(size_t) i] - 1.0f / 16.0f);

        xyPoints[(size_t) i]->setMotionPhase (devianceMotionPhase);
        xyPoints[(size_t) i]->setSelectionPulse (pointSelectionPulse[(size_t) i]);
    }

    repaint();

    if (xyAnimationProgress >= 1.0f && ! hasActiveDevianceMotion() && ! hasActiveSelectionPulse())
        stopTimer();
}

bool MainComponent::hasActiveDevianceMotion() const
{
    for (int i = 0; i < 8; ++i)
        if (pointRotaryValues[4][(size_t) i] > 0.01f)
            return true;

    return false;
}

bool MainComponent::hasActiveSelectionPulse() const
{
    for (int i = 0; i < 8; ++i)
        if (padSelectionPulse[(size_t) i] > 0.01f || pointSelectionPulse[(size_t) i] > 0.01f)
            return true;

    return false;
}

bool MainComponent::loadWaveformFromWavFile (const juce::File& file)
{
    waveformPeaks.clear();
    sampleLoaded = false;

    if (! file.hasFileExtension ("wav"))
        return false;

    juce::MemoryBlock bytes;

    if (! file.loadFileAsData (bytes))
        return false;

    const auto* data = static_cast<const char*> (bytes.getData());
    const auto size = bytes.getSize();

    if (size < 44 || ! hasTag (data, size, 0, "RIFF") || ! hasTag (data, size, 8, "WAVE"))
        return false;

    uint16_t audioFormat = 0;
    uint16_t channels = 0;
    uint16_t bitsPerSample = 0;
    uint16_t blockAlign = 0;

    size_t audioDataStart = 0;
    size_t audioDataSize = 0;

    size_t offset = 12;

    while (offset + 8 <= size)
    {
        const auto chunkSize = (size_t) readU32LE (data, offset + 4);
        const auto chunkData = offset + 8;

        if (chunkData + chunkSize > size)
            break;

        if (hasTag (data, size, offset, "fmt ") && chunkSize >= 16)
        {
            audioFormat = readU16LE (data, chunkData + 0);
            channels = readU16LE (data, chunkData + 2);
            blockAlign = readU16LE (data, chunkData + 12);
            bitsPerSample = readU16LE (data, chunkData + 14);
        }
        else if (hasTag (data, size, offset, "data"))
        {
            audioDataStart = chunkData;
            audioDataSize = chunkSize;
        }

        offset = chunkData + chunkSize + (chunkSize & 1u);
    }

    if (channels == 0 || blockAlign == 0 || audioDataStart == 0 || audioDataSize == 0)
        return false;

    if (! ((audioFormat == 1 && (bitsPerSample == 16 || bitsPerSample == 24 || bitsPerSample == 32))
        || (audioFormat == 3 && bitsPerSample == 32)))
        return false;

    const auto frameCount = audioDataSize / blockAlign;

    if (frameCount == 0)
        return false;

    constexpr int peakCount = 256;
    waveformPeaks.assign ((size_t) peakCount, 0.0f);

    const auto bytesPerSample = bitsPerSample / 8;
    const auto* sampleData = data + audioDataStart;

    for (int peakIndex = 0; peakIndex < peakCount; ++peakIndex)
    {
        const auto startFrame = (size_t) ((uint64_t) peakIndex * frameCount / peakCount);
        const auto endFrame = (size_t) ((uint64_t) (peakIndex + 1) * frameCount / peakCount);

        float peak = 0.0f;

        for (size_t frame = startFrame; frame < endFrame; ++frame)
        {
            const auto* framePtr = sampleData + frame * blockAlign;

            for (uint16_t channel = 0; channel < channels; ++channel)
            {
                const auto* samplePtr = framePtr + channel * bytesPerSample;
                peak = juce::jmax (peak, std::abs (readSampleValue (samplePtr, audioFormat, bitsPerSample)));
            }
        }

        waveformPeaks[(size_t) peakIndex] = juce::jlimit (0.0f, 1.0f, peak);
    }

    float maxPeak = 0.0f;

    for (auto peak : waveformPeaks)
        maxPeak = juce::jmax (maxPeak, peak);

    if (maxPeak > 0.001f)
        for (auto& peak : waveformPeaks)
            peak = juce::jlimit (0.0f, 1.0f, peak / maxPeak);

    sampleLoaded = true;
    return true;
}

void MainComponent::updateLayout()
{
    // Layout matched to the 2x guide coordinates provided by Nicolas.

    xyPad = { 33.0f, 152.0f, 568.0f, 568.0f };

    topButtons[0] = { 32.0f, 736.0f, 32.0f, 32.0f };

    const float topY = 736.0f;
    const float topParamX = 105.0f;
    const float topParamWidth = 64.0f;
    const float topParamHeight = 32.0f;
    const float topParamGap = 8.0f;

    float x = topParamX;

    for (int i = 1; i < 8; ++i)
    {
        topButtons[(size_t) i] = { x, topY, topParamWidth, topParamHeight };
        x += topParamWidth + topParamGap;
    }

    samplePlayer = { 32.0f, 72.0f, 528.0f, 64.0f };

    sampleButtons[0] = { 568.0f, 72.0f, 32.0f, 32.0f };
    sampleButtons[1] = { 568.0f, 104.0f, 32.0f, 32.0f };

    const float padY = 816.0f;
    const float padButtonSize = 64.0f;
    const float padButtonGap = 8.0f;

    x = 32.0f;

    for (int i = 0; i < 8; ++i)
    {
        padButtons[(size_t) i] = { x, padY, padButtonSize, padButtonSize };
        padRenameButtons[(size_t) i] = { x + 48.0f, padY - 16.0f, 16.0f, 16.0f };

        padUtilityButtons[(size_t) (i * 2)] = { x, padY + padButtonSize, 32.0f, 32.0f };
        padUtilityButtons[(size_t) (i * 2 + 1)] = { x + 32.0f, padY + padButtonSize, 32.0f, 32.0f };

        x += padButtonSize + padButtonGap;
    }
}

void MainComponent::activateXYPad()
{
    xyPadActivated = true;
    selectedTopButton = 0;
    selectedOnsetIndex = 3;
    xyAnimationProgress = 0.0f;

    const auto centre = xyPad.getCentre();
    const float maxRadius = xyPad.getWidth() * 0.5f - pointDotSize * 0.5f;

    for (int i = 0; i < 8; ++i)
    {
        const float angle = juce::MathConstants<float>::twoPi * ((float) i / 8.0f)
                          - juce::MathConstants<float>::halfPi;

        pointPositions[(size_t) i] = centre;
        pointTargets[(size_t) i] = { centre.x + std::cos (angle) * maxRadius,
                                     centre.y + std::sin (angle) * maxRadius };

        xyPoints[(size_t) i]->setVisible (true);
    }

    updatePointModes();
    layoutPadPoints();
    startTimerHz (60);
    repaint();
}

void MainComponent::updatePointModes()
{
    const bool rotaryMode = xyPadActivated && selectedTopButton != 0;

    for (int i = 0; i < 8; ++i)
    {
        xyPoints[(size_t) i]->setRotaryMode (rotaryMode);
        xyPoints[(size_t) i]->setRotaryValue (pointRotaryValues[(size_t) selectedTopButton][(size_t) i]);
        xyPoints[(size_t) i]->setDevianceValue (pointRotaryValues[4][(size_t) i]);
        xyPoints[(size_t) i]->setInstabilityValue (pointRotaryValues[5][(size_t) i]);
        xyPoints[(size_t) i]->setVolumeValue (pointRotaryValues[7][(size_t) i]);
        xyPoints[(size_t) i]->setSelectionPulse (pointSelectionPulse[(size_t) i]);
        xyPoints[(size_t) i]->setSelectedHighlight (selectedPadButton == i);
    }
}

void MainComponent::layoutPadPoints()
{
    for (int i = 0; i < 8; ++i)
    {
        if (xyPoints[(size_t) i] == nullptr)
            continue;

        xyPoints[(size_t) i]->setBounds (juce::Rectangle<float> (pointComponentSize, pointComponentSize)
                                             .withCentre (pointPositions[(size_t) i])
                                             .toNearestInt());
    }
}

void MainComponent::drawPointConnections (juce::Graphics& g)
{
    if (! xyPadActivated)
        return;

    g.setColour (juce::Colours::white.withAlpha (0.075f));

    for (int i = 0; i < 8; ++i)
    {
        int closest = -1;
        float closestDistance = std::numeric_limits<float>::max();

        for (int j = 0; j < 8; ++j)
        {
            if (i == j)
                continue;

            const auto distance = pointPositions[(size_t) i].getDistanceFrom (pointPositions[(size_t) j]);

            if (distance < closestDistance)
            {
                closestDistance = distance;
                closest = j;
            }
        }

        if (closest > i)
        {
            g.drawLine (pointPositions[(size_t) i].x,
                        pointPositions[(size_t) i].y,
                        pointPositions[(size_t) closest].x,
                        pointPositions[(size_t) closest].y,
                        1.0f);
        }
    }
}

void MainComponent::movePadPoint (int index, juce::Point<float> parentPosition)
{
    if (! xyPadActivated || selectedTopButton != 0)
        return;

    auto safeArea = xyPad.reduced (pointDotSize * 0.5f);

    pointPositions[(size_t) index] = {
        juce::jlimit (safeArea.getX(), safeArea.getRight(), parentPosition.x),
        juce::jlimit (safeArea.getY(), safeArea.getBottom(), parentPosition.y)
    };

    pointTargets[(size_t) index] = pointPositions[(size_t) index];

    layoutPadPoints();
    repaint (xyPad.toNearestInt());
}

void MainComponent::setPointRotaryValue (int index, float value)
{
    if (! xyPadActivated || selectedTopButton == 0)
        return;

    pointRotaryValues[(size_t) selectedTopButton][(size_t) index] = juce::jlimit (0.0f, 1.0f, value);
    xyPoints[(size_t) index]->setRotaryValue (pointRotaryValues[(size_t) selectedTopButton][(size_t) index]);

    if (selectedTopButton == 4)
        xyPoints[(size_t) index]->setDevianceValue (pointRotaryValues[4][(size_t) index]);

    if (selectedTopButton == 5)
        xyPoints[(size_t) index]->setInstabilityValue (pointRotaryValues[5][(size_t) index]);

    if (selectedTopButton == 7)
        xyPoints[(size_t) index]->setVolumeValue (pointRotaryValues[7][(size_t) index]);

    if (selectedTopButton > 0)
        repaint (topButtons[(size_t) selectedTopButton].toNearestInt());

    if ((selectedTopButton == 4 || selectedTopButton == 5) && hasActiveDevianceMotion() && ! isTimerRunning())
        startTimerHz (60);
}

void MainComponent::selectPadFromPoint (int index, bool shouldTriggerPulse)
{
    selectedPadButton = index;

    if (shouldTriggerPulse)
        triggerSelectionPulse (index);

    updatePointModes();
    repaint();
}

void MainComponent::startPadRename (int index)
{
    selectedPadButton = index;
    triggerSelectionPulse (index);
    updatePointModes();

    renamingPad = index;

    const auto labelArea = juce::Rectangle<float> (48.0f, 16.0f)
                               .withPosition (padButtons[(size_t) index].getX(),
                                              padButtons[(size_t) index].getY() - 16.0f);

    padRenameEditor.setBounds (labelArea.toNearestInt());
    padRenameEditor.setText (padLabels[(size_t) index], false);
    padRenameEditor.setVisible (true);
    padRenameEditor.toFront (true);
    padRenameEditor.grabKeyboardFocus();
    padRenameEditor.selectAll();
}

void MainComponent::finishPadRename (bool shouldCommit)
{
    if (renamingPad < 0)
        return;

    if (shouldCommit)
    {
        auto text = padRenameEditor.getText().trim();

        if (text.isNotEmpty())
            padLabels[(size_t) renamingPad] = text;
    }

    renamingPad = -1;
    padRenameEditor.setVisible (false);
    repaint();
}

void MainComponent::triggerSelectionPulse (int index)
{
    padSelectionPulse[(size_t) index] = 1.0f;
    pointSelectionPulse[(size_t) index] = 1.0f;

    xyPoints[(size_t) index]->setSelectionPulse (1.0f);

    if (! isTimerRunning())
        startTimerHz (60);
}

void MainComponent::drawEmptyButton (juce::Graphics& g,
                                     juce::Rectangle<float> bounds,
                                     bool selected,
                                     bool hovered,
                                     juce::Colour selectedColour)
{
    if (hovered)
    {
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillRect (bounds);
    }

    if (selected)
    {
        if (selectedColour == getTopButtonColour (0))
        {
            g.setColour (juce::Colours::white.withAlpha (0.16f));
            g.fillRect (bounds);
        }
        else
        {
            g.setColour (selectedColour);
            g.drawRect (bounds.reduced (2.0f).toNearestInt(), 2);
        }
    }

}

void MainComponent::drawMultiDirectionCross (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const auto c = bounds.getCentre();
    const float arm = 13.0f;

    g.setColour (juce::Colours::white.withAlpha (0.70f));

    g.drawLine (c.x - arm, c.y, c.x + arm, c.y, 1.4f);
    g.drawLine (c.x, c.y - arm, c.x, c.y + arm, 1.4f);

    g.drawLine (c.x - arm * 0.65f, c.y - arm * 0.65f,
                c.x + arm * 0.65f, c.y + arm * 0.65f, 1.1f);

    g.drawLine (c.x + arm * 0.65f, c.y - arm * 0.65f,
                c.x - arm * 0.65f, c.y + arm * 0.65f, 1.1f);

    g.fillEllipse (c.x - 2.0f, c.y - 2.0f, 4.0f, 4.0f);
}

int MainComponent::findClickedButton (const std::array<juce::Rectangle<float>, 8>& buttons,
                                      juce::Point<float> point) const
{
    for (int i = 0; i < 8; ++i)
        if (buttons[(size_t) i].contains (point))
            return i;

    return -1;
}

int MainComponent::findClickedButton (const std::array<juce::Rectangle<float>, 2>& buttons,
                                      juce::Point<float> point) const
{
    for (int i = 0; i < 2; ++i)
        if (buttons[(size_t) i].contains (point))
            return i;

    return -1;
}

int MainComponent::findClickedButton (const std::array<juce::Rectangle<float>, 16>& buttons,
                                      juce::Point<float> point) const
{
    for (int i = 0; i < 16; ++i)
        if (buttons[(size_t) i].contains (point))
            return i;

    return -1;
}
