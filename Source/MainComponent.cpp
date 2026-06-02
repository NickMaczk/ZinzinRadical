#include "MainComponent.h"

namespace
{
    constexpr float pointComponentSize = 120.0f;
    constexpr float pointDotSize = 18.0f;
    constexpr float rotarySize = 48.0f;

    juce::Colour getTopButtonColour (int index)
    {
        switch (index)
        {
            case 1:  return juce::Colour::fromRGB (255, 43, 214);  // pitch / hot pink
            case 2:  return juce::Colour::fromRGB (255, 91, 31);   // length / sanguine orange
            case 3:  return juce::Colour::fromRGB (0, 229, 255);   // super / electric cyan
            case 4:  return juce::Colour::fromRGB (178, 255, 36);  // instability / acid lime
            case 5:  return juce::Colour::fromRGB (155, 92, 255);  // deviance / violet
            case 6:  return juce::Colour::fromRGB (56, 130, 255);  // pan / electric blue
            case 7:  return juce::Colour::fromRGB (255, 218, 46);  // vol / hot yellow
            default: return juce::Colour::fromRGB (235, 58, 58);
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

    const float dotMotionRadius = juce::jmap (devianceValue, 0.0f, 1.0f, 0.0f, 28.0f);

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

    const float visualDotSize = juce::jmap (volumeValue, 0.0f, 1.0f, 8.0f, pointDotSize);

    const auto devianceBaseArea = juce::Rectangle<float> (pointDotSize, pointDotSize)
                                      .withCentre (centre);

    if (devianceValue > 0.01f)
    {
        const float haloExpand = juce::jmap (devianceValue, 0.0f, 1.0f, 12.0f, 42.0f);
        const auto haloArea = devianceBaseArea.expanded (haloExpand);

        g.setColour (getTopButtonColour (5).withAlpha (0.055f));
        g.fillEllipse (haloArea);

        g.setColour (getTopButtonColour (5).withAlpha (0.22f));
        g.drawEllipse (haloArea, 1.0f);
    }

    const auto ghostDotArea = juce::Rectangle<float> (visualDotSize, visualDotSize)
                                  .withCentre (centre + dotOffset);

    const auto dotArea = juce::Rectangle<float> (visualDotSize, visualDotSize)
                             .withCentre (centre);

    if (devianceValue > 0.01f)
    {
        g.setColour (getTopButtonColour (5).withAlpha (0.24f));
        g.fillEllipse (ghostDotArea);

        g.setColour (getTopButtonColour (5).withAlpha (0.50f));
        g.drawEllipse (ghostDotArea, 1.2f);
    }

    if (selectionPulse > 0.01f)
    {
        const float pulseExpansion = 8.0f + (1.0f - selectionPulse) * 12.0f;
        const auto pulseArea = dotArea.expanded (pulseExpansion);

        g.setColour (juce::Colour::fromRGB (45, 215, 105).withAlpha (0.18f * selectionPulse));
        g.fillEllipse (pulseArea);

        g.setColour (juce::Colour::fromRGB (45, 215, 105).withAlpha (0.65f * selectionPulse));
        g.drawEllipse (pulseArea, 2.0f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.88f));
    g.fillEllipse (dotArea);

    g.setColour (selectionPulse > 0.01f ? juce::Colour::fromRGB (45, 215, 105)
                                         : juce::Colours::white.withAlpha (0.38f));
    g.drawEllipse (dotArea, selectionPulse > 0.01f ? 2.2f : 1.5f);

    g.setColour (juce::Colours::white.withAlpha (0.72f));
    g.setFont (juce::FontOptions (11.0f));
    g.drawFittedText (juce::String (index + 1),
                      juce::Rectangle<int> ((int) centre.x + 13,
                                            (int) centre.y - 8,
                                            14,
                                            16),
                      juce::Justification::centred,
                      1);
}

bool PadPoint::hitTest (int x, int y)
{
    const auto centre = getLocalBounds().toFloat().getCentre();
    const auto p = juce::Point<float> ((float) x, (float) y);

    if (rotaryMode)
        return p.getDistanceFrom (centre) <= rotarySize * 0.5f;

    return p.getDistanceFrom (centre) <= pointDotSize * 0.5f;
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
    for (int i = 0; i < 8; ++i)
    {
        xyPoints[(size_t) i] = std::make_unique<PadPoint> (*this, i);
        addAndMakeVisible (*xyPoints[(size_t) i]);
        xyPoints[(size_t) i]->setVisible (false);
    }

    for (auto& valuesForTopButton : pointRotaryValues)
        valuesForTopButton.fill (0.35f);

    pointRotaryValues[4].fill (0.0f);
    pointRotaryValues[5].fill (0.0f);
    pointRotaryValues[7].fill (1.0f);

    setSize (640, 740);
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (18, 18, 20));

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (juce::FontOptions (13.0f));

    const std::array<juce::String, 8> topButtonLabels {
        "",
        "pitch",
        "length",
        "super",
        "instability",
        "deviance",
        "pan",
        "vol"
    };

    for (int i = 1; i < 8; ++i)
    {
        auto labelArea = topButtons[(size_t) i].toNearestInt()
                                             .translated (0, -22)
                                             .withHeight (18);

        g.setColour (getTopButtonColour (i).withAlpha (0.88f));
        g.drawFittedText (topButtonLabels[(size_t) i],
                          labelArea,
                          juce::Justification::centred,
                          1);
    }

    for (int i = 0; i < 8; ++i)
    {
        drawEmptyButton (g,
                         topButtons[(size_t) i],
                         selectedTopButton == i,
                         {},
                         getTopButtonColour (i));

        if (i == 0)
            drawMultiDirectionCross (g, topButtons[0]);
    }

    g.setColour (juce::Colour::fromRGB (31, 31, 35));
    g.fillRoundedRectangle (xyPad, 10.0f);

    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawRoundedRectangle (xyPad, 10.0f, 1.0f);

    if (! xyPadActivated)
    {
        g.setColour (juce::Colours::white.withAlpha (0.48f));
        g.setFont (juce::FontOptions (15.0f));
        g.drawFittedText ("drop a sample here",
                          xyPad.toNearestInt(),
                          juce::Justification::centred,
                          1);
    }

    auto padsLabel = padButtons[0].getUnion (padButtons[7])
                                  .toNearestInt()
                                  .translated (0, -24)
                                  .withHeight (18);

    g.setColour (juce::Colours::white.withAlpha (0.58f));
    g.setFont (juce::FontOptions (13.0f));
    g.drawFittedText ("pads", padsLabel, juce::Justification::centredLeft, 1);

    for (int i = 0; i < 8; ++i)
    {
        drawEmptyButton (g,
                         padButtons[(size_t) i],
                         false,
                         juce::String (i + 1));

        if (padSelectionPulse[(size_t) i] > 0.01f)
        {
            const auto pulse = padSelectionPulse[(size_t) i];
            const auto pulseArea = padButtons[(size_t) i].reduced (1.0f);

            g.setColour (juce::Colour::fromRGB (45, 215, 105).withAlpha (0.16f * pulse));
            g.fillRoundedRectangle (pulseArea, 6.0f);

            g.setColour (juce::Colour::fromRGB (45, 215, 105).withAlpha (0.65f * pulse));
            g.drawRoundedRectangle (pulseArea, 6.0f, 1.6f);
        }
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

    if (const auto button = findClickedButton (padButtons, point); button >= 0)
    {
        selectedPadButton = button;
        triggerSelectionPulse (button);
        updatePointModes();
        repaint();
        return;
    }
}

bool MainComponent::isInterestedInFileDrag (const juce::StringArray&)
{
    return true;
}

void MainComponent::filesDropped (const juce::StringArray&, int x, int y)
{
    if (xyPad.contains (juce::Point<float> ((float) x, (float) y)))
        activateXYPad();
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
        if (pointRotaryValues[5][(size_t) i] > 0.01f)
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

void MainComponent::updateLayout()
{
    const float padSize = 512.0f;
    const float gap = 8.0f;

    const float topButtonHeight = 48.0f;
    const float firstTopButtonWidth = topButtonHeight;
    const float otherTopButtonWidth = (padSize - firstTopButtonWidth - gap * 7.0f) / 7.0f;

    const float padButtonHeight = 52.0f;
    const float padButtonWidth = (padSize - gap * 7.0f) / 8.0f;

    const float startX = (float) getWidth() * 0.5f - padSize * 0.5f;
    float y = 54.0f;

    float x = startX;

    topButtons[0] = { x, y, firstTopButtonWidth, topButtonHeight };
    x += firstTopButtonWidth + gap;

    for (int i = 1; i < 8; ++i)
    {
        topButtons[(size_t) i] = { x, y, otherTopButtonWidth, topButtonHeight };
        x += otherTopButtonWidth + gap;
    }

    y += topButtonHeight + 20.0f;

    xyPad = { startX, y, padSize, padSize };

    y += padSize + 46.0f;
    x = startX;

    for (int i = 0; i < 8; ++i)
    {
        padButtons[(size_t) i] = { x, y, padButtonWidth, padButtonHeight };
        x += padButtonWidth + gap;
    }
}

void MainComponent::activateXYPad()
{
    xyPadActivated = true;
    selectedTopButton = 0;
    xyAnimationProgress = 0.0f;

    const auto centre = xyPad.getCentre();
    const float maxRadius = xyPad.getWidth() * 0.5f - 42.0f;

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
        xyPoints[(size_t) i]->setDevianceValue (pointRotaryValues[5][(size_t) i]);
        xyPoints[(size_t) i]->setInstabilityValue (pointRotaryValues[4][(size_t) i]);
        xyPoints[(size_t) i]->setVolumeValue (pointRotaryValues[7][(size_t) i]);
        xyPoints[(size_t) i]->setSelectionPulse (pointSelectionPulse[(size_t) i]);
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

void MainComponent::movePadPoint (int index, juce::Point<float> parentPosition)
{
    if (! xyPadActivated || selectedTopButton != 0)
        return;

    auto safeArea = xyPad.reduced (pointComponentSize * 0.5f);

    pointPositions[(size_t) index] = {
        juce::jlimit (safeArea.getX(), safeArea.getRight(), parentPosition.x),
        juce::jlimit (safeArea.getY(), safeArea.getBottom(), parentPosition.y)
    };

    layoutPadPoints();
}

void MainComponent::setPointRotaryValue (int index, float value)
{
    if (! xyPadActivated || selectedTopButton == 0)
        return;

    pointRotaryValues[(size_t) selectedTopButton][(size_t) index] = juce::jlimit (0.0f, 1.0f, value);
    xyPoints[(size_t) index]->setRotaryValue (pointRotaryValues[(size_t) selectedTopButton][(size_t) index]);

    if (selectedTopButton == 5)
        xyPoints[(size_t) index]->setDevianceValue (pointRotaryValues[5][(size_t) index]);

    if (selectedTopButton == 4)
        xyPoints[(size_t) index]->setInstabilityValue (pointRotaryValues[4][(size_t) index]);

    if (selectedTopButton == 7)
        xyPoints[(size_t) index]->setVolumeValue (pointRotaryValues[7][(size_t) index]);

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
                                     const juce::String& textInside,
                                     juce::Colour selectedColour)
{
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (juce::Colours::white.withAlpha (0.20f));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    if (textInside.isNotEmpty())
    {
        g.setColour (juce::Colours::white.withAlpha (0.72f));
        g.setFont (juce::FontOptions (17.0f));
        g.drawFittedText (textInside,
                          bounds.toNearestInt(),
                          juce::Justification::centred,
                          1);
    }

    if (selected)
    {
        const auto y = bounds.getBottom() - 4.0f;

        g.setColour (selectedColour);
        g.drawLine (bounds.getX() + 8.0f,
                    y,
                    bounds.getRight() - 8.0f,
                    y,
                    2.0f);
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
