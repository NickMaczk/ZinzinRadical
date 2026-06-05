#include "PluginUxPage.h"

#include <cmath>
#include <cstring>
#include <cstdint>

namespace
{

    juce::File findGuideImageFile()
    {
        auto dir = juce::File::getCurrentWorkingDirectory();

        for (int i = 0; i < 8; ++i)
        {
            auto assets = dir.getChildFile ("Assets");

            if (assets.isDirectory())
            {
                for (auto name : { "guide.png", "guide.jpg", "guide.jpeg", "guide.webp" })
                {
                    auto candidate = assets.getChildFile (name);

                    if (candidate.existsAsFile())
                        return candidate;
                }

                juce::Array<juce::File> files;
                assets.findChildFiles (files, juce::File::findFiles, false, "*.png;*.jpg;*.jpeg;*.webp");

                if (! files.isEmpty())
                    return files.getFirst();
            }

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

    juce::Colour getPadColour (int index, float alpha = 0.95f)
    {
        return juce::Colour::fromHSV ((float) index / 8.0f, 0.75f, 0.95f, alpha);
    }

    void drawFrame (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour accent)
    {
        g.setColour (juce::Colours::white.withAlpha (0.055f));
        g.fillRect (bounds);

        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawRect (bounds.toNearestInt(), 1);

        g.setColour (accent.withAlpha (0.35f));
        g.drawRect (bounds.reduced (1.0f).toNearestInt(), 1);
    }
}

WavePreviewComponent::WavePreviewComponent()
{
    setWantsKeyboardFocus (true);
}

void WavePreviewComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    waveArea = { 0.0f, 0.0f, bounds.getWidth() - 40.0f, bounds.getHeight() };
    navButtons[0] = { bounds.getWidth() - 32.0f, 0.0f, 32.0f, 32.0f };
    navButtons[1] = { bounds.getWidth() - 32.0f, 32.0f, 32.0f, 32.0f };

    if (sampleLoaded && ! waveformPeaks.empty())
    {
        const auto centreY = waveArea.getCentreY();
        const auto count = (int) waveformPeaks.size();

        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.drawLine (waveArea.getX(), centreY, waveArea.getRight(), centreY, 1.0f);

        g.setColour (juce::Colours::white.withAlpha (0.28f));

        for (int i = 0; i < count; ++i)
        {
            const float x = juce::jmap ((float) i, 0.0f, (float) juce::jmax (1, count - 1), waveArea.getX() + 8.0f, waveArea.getRight() - 8.0f);
            const float h = juce::jmax (1.0f, waveformPeaks[(size_t) i] * waveArea.getHeight() * 0.42f);
            g.drawLine (x, centreY - h, x, centreY + h, 2.0f);
        }

        const std::array<float, 8> onsets { 0.07f, 0.18f, 0.31f, 0.44f, 0.56f, 0.69f, 0.82f, 0.94f };

        const float left = waveArea.getX() + waveArea.getWidth() * onsets[(size_t) selectedOnsetIndex];
        const float right = selectedOnsetIndex == 7 ? waveArea.getRight()
                                                    : waveArea.getX() + waveArea.getWidth() * onsets[(size_t) selectedOnsetIndex + 1];

        g.setColour (juce::Colour::fromRGB (0, 229, 255).withAlpha (0.20f));
        g.fillRect (juce::Rectangle<float> (left, waveArea.getY() + 4.0f, right - left, waveArea.getHeight() - 8.0f));

        g.setColour (juce::Colours::white.withAlpha (0.42f));

        for (auto onset : onsets)
        {
            const float x = waveArea.getX() + waveArea.getWidth() * onset;
            g.drawLine (x, waveArea.getY() + 6.0f, x, waveArea.getBottom() - 6.0f, 1.5f);
        }

        g.setColour (juce::Colours::white.withAlpha (0.64f));
        g.setFont (juce::FontOptions (11.0f));
        g.drawFittedText (fileName, waveArea.reduced (10.0f, 4.0f).toNearestInt(), juce::Justification::topLeft, 1);
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.52f));
        g.setFont (juce::FontOptions (15.0f));
        g.drawFittedText ("drop a sample here", waveArea.toNearestInt(), juce::Justification::centred, 1);
    }

}

void WavePreviewComponent::mouseDown (const juce::MouseEvent& e)
{
    if (navButtons[0].contains (e.position))
        moveSelectedOnset (-1);

    if (navButtons[1].contains (e.position))
        moveSelectedOnset (1);
}

bool WavePreviewComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& path : files)
        if (juce::File (path).hasFileExtension ("wav"))
            return true;

    return false;
}

void WavePreviewComponent::filesDropped (const juce::StringArray& files, int, int)
{
    if (files.isEmpty())
        return;

    loadWaveformFromWavFile (juce::File (files[0]));
    repaint();
}

void WavePreviewComponent::moveSelectedOnset (int delta)
{
    selectedOnsetIndex = (selectedOnsetIndex + delta + 8) % 8;
    repaint();
}

bool WavePreviewComponent::loadWaveformFromWavFile (const juce::File& file)
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
                peak = juce::jmax (peak, std::abs (readSampleValue (framePtr + channel * bytesPerSample, audioFormat, bitsPerSample)));
        }

        waveformPeaks[(size_t) peakIndex] = juce::jlimit (0.0f, 1.0f, peak);
    }

    float maxPeak = 0.0f;

    for (auto peak : waveformPeaks)
        maxPeak = juce::jmax (maxPeak, peak);

    if (maxPeak > 0.001f)
        for (auto& peak : waveformPeaks)
            peak = juce::jlimit (0.0f, 1.0f, peak / maxPeak);

    fileName = file.getFileName();
    sampleLoaded = true;
    return true;
}

XYPadComponent::XYPadComponent()
{
}
void XYPadComponent::setPointColour (juce::Colour newColour)
{
    pointColour = newColour;
    repaint();
}

void XYPadComponent::paint (juce::Graphics& g)
{
    const auto p = toPixelPosition (point);
    const auto dot = juce::Rectangle<float> (24.0f, 24.0f).withCentre (p);

    if (pointHovered)
    {
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillRect (dot.expanded (10.0f));
    }

    g.setColour (pointColour);
    g.fillRect (dot);

    g.setColour (juce::Colours::white.withAlpha (0.95f));
    g.drawRect (dot.toNearestInt(), 2);
}

void XYPadComponent::mouseDown (const juce::MouseEvent& e)
{
    movePointTo (e.position);
}

void XYPadComponent::mouseDrag (const juce::MouseEvent& e)
{
    movePointTo (e.position);
}

void XYPadComponent::mouseMove (const juce::MouseEvent& e)
{
    const bool hovered = isPointAt (e.position);

    if (pointHovered != hovered)
    {
        pointHovered = hovered;
        repaint();
    }
}

void XYPadComponent::mouseExit (const juce::MouseEvent&)
{
    pointHovered = false;
    repaint();
}

juce::Point<float> XYPadComponent::toPixelPosition (juce::Point<float> normalised) const
{
    const auto bounds = getLocalBounds().toFloat().reduced (16.0f);
    return {
        bounds.getX() + normalised.x * bounds.getWidth(),
        bounds.getY() + normalised.y * bounds.getHeight()
    };
}

juce::Point<float> XYPadComponent::toNormalisedPosition (juce::Point<float> pixel) const
{
    const auto bounds = getLocalBounds().toFloat().reduced (16.0f);

    return {
        juce::jlimit (0.0f, 1.0f, (pixel.x - bounds.getX()) / bounds.getWidth()),
        juce::jlimit (0.0f, 1.0f, (pixel.y - bounds.getY()) / bounds.getHeight())
    };
}

bool XYPadComponent::isPointAt (juce::Point<float> pixel) const
{
    return pixel.getDistanceFrom (toPixelPosition (point)) <= 16.0f;
}

void XYPadComponent::movePointTo (juce::Point<float> pixel)
{
    point = toNormalisedPosition (pixel);
    repaint();
}

PadGridComponent::PadGridComponent()
{
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
}

void PadGridComponent::paint (juce::Graphics& g)
{
    updatePadBounds();

    for (int i = 0; i < 8; ++i)
    {
        const auto pad = padBounds[(size_t) i];
        auto labelArea = juce::Rectangle<float> (pad.getWidth(), 16.0f)
                            .withPosition (pad.getX(), pad.getY() - 18.0f);

        padRenameButtons[(size_t) i] = labelArea.removeFromRight (16.0f);

        if (renamingPad != i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.72f));
            g.setFont (juce::FontOptions (10.0f));
            g.drawFittedText (padLabels[(size_t) i],
                              labelArea.toNearestInt(),
                              juce::Justification::centredLeft,
                              1);

            if (hoveredPadRenameButton == i)
            {
                g.setColour (juce::Colours::white.withAlpha (0.12f));
                g.fillRect (padRenameButtons[(size_t) i]);
            }

        }

        const auto padAccentArea = pad.reduced (4.0f);

        if (selectedPad == i)
        {
            g.setColour (getPadColour (i));
            g.fillRect (padAccentArea.reduced (10.0f));
        }

        g.setColour (getPadColour (i).withAlpha (0.95f));
        g.drawRect (padAccentArea.toNearestInt(), 4);

        if (hoveredPad == i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.fillRect (pad);
        }

    }
}

void PadGridComponent::mouseDown (const juce::MouseEvent& e)
{
    updatePadBounds();

    for (int i = 0; i < 8; ++i)
    {
        if (padRenameButtons[(size_t) i].contains (e.position))
        {
            startPadRename (i);
            return;
        }
    }

    const int pad = findPadAt (e.position);

    if (pad >= 0)
    {
        selectedPad = pad;

        if (onSelectedPadChanged != nullptr)
            onSelectedPadChanged (selectedPad);

        repaint();
    }
}

void PadGridComponent::mouseMove (const juce::MouseEvent& e)
{
    updatePadBounds();

    const int pad = findPadAt (e.position);
    int renameButton = -1;

    for (int i = 0; i < 8; ++i)
        if (padRenameButtons[(size_t) i].contains (e.position))
            renameButton = i;

    if (hoveredPad != pad || hoveredPadRenameButton != renameButton)
    {
        hoveredPad = pad;
        hoveredPadRenameButton = renameButton;
        repaint();
    }
}

void PadGridComponent::mouseExit (const juce::MouseEvent&)
{
    hoveredPad = -1;
    hoveredPadRenameButton = -1;
    repaint();
}

void PadGridComponent::startPadRename (int index)
{
    selectedPad = index;

    if (onSelectedPadChanged != nullptr)
        onSelectedPadChanged (selectedPad);

    updatePadBounds();

    auto labelArea = juce::Rectangle<float> (padBounds[(size_t) index].getWidth(), 16.0f)
                        .withPosition (padBounds[(size_t) index].getX(), padBounds[(size_t) index].getY() - 18.0f)
                        .withTrimmedRight (16.0f);

    renamingPad = index;

    padRenameEditor.setBounds (labelArea.toNearestInt());
    padRenameEditor.setText (padLabels[(size_t) index], false);
    padRenameEditor.setVisible (true);
    padRenameEditor.toFront (true);
    padRenameEditor.grabKeyboardFocus();
    padRenameEditor.selectAll();

    repaint();
}

void PadGridComponent::finishPadRename (bool shouldCommit)
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

void PadGridComponent::updatePadBounds()
{
    const auto bounds = getLocalBounds().toFloat();
    const float gap = 8.0f;
    const float labelHeight = 18.0f;
    const float size = juce::jmin ((bounds.getWidth() - gap * 7.0f) / 8.0f,
                                   bounds.getHeight() - labelHeight);

    const float totalW = size * 8.0f + gap * 7.0f;
    const float startX = bounds.getCentreX() - totalW * 0.5f;
    const float startY = bounds.getY() + labelHeight + (bounds.getHeight() - labelHeight - size) * 0.5f;

    for (int i = 0; i < 8; ++i)
    {
        padBounds[(size_t) i] = {
            startX + (float) i * (size + gap),
            startY,
            size,
            size
        };
    }
}

int PadGridComponent::findPadAt (juce::Point<float> p) const
{
    for (int i = 0; i < 8; ++i)
        if (padBounds[(size_t) i].contains (p))
            return i;

    return -1;
}

SimpleRotaryComponent::SimpleRotaryComponent()
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void SimpleRotaryComponent::setValue (float newValue)
{
    value = juce::jlimit (0.0f, 1.0f, newValue);
    repaint();
}

void SimpleRotaryComponent::setBipolar (bool shouldBeBipolar)
{
    bipolar = shouldBeBipolar;
    repaint();
}

void SimpleRotaryComponent::setAccentColour (juce::Colour newColour)
{
    accentColour = newColour;
    repaint();
}

void SimpleRotaryComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (8.0f);
    const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto circle = juce::Rectangle<float> (size, size).withCentre (bounds.getCentre()).reduced (9.0f);
    const auto centre = circle.getCentre();

    g.setColour (juce::Colour::fromRGB (62, 62, 66));
    g.fillEllipse (circle);

    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawEllipse (circle, 1.0f);

    const float outerRadius = circle.getWidth() * 0.5f + 6.0f;

    g.setColour (juce::Colours::white.withAlpha (0.13f));
    g.drawEllipse (juce::Rectangle<float> (outerRadius * 2.0f, outerRadius * 2.0f).withCentre (centre), 2.0f);

    juce::Path arc;
    const float start = -juce::MathConstants<float>::halfPi;

    if (bipolar)
    {
        const float signedValue = (value - 0.5f) * 2.0f;

        if (std::abs (signedValue) > 0.01f)
        {
            const float end = start + signedValue * juce::MathConstants<float>::pi;
            arc.addCentredArc (centre.x, centre.y, outerRadius, outerRadius, 0.0f,
                               juce::jmin (start, end),
                               juce::jmax (start, end),
                               true);
        }
    }
    else
    {
        const float end = start + value * juce::MathConstants<float>::twoPi;
        arc.addCentredArc (centre.x, centre.y, outerRadius, outerRadius, 0.0f, start, end, true);
    }

    g.setColour (accentColour);
    g.strokePath (arc, juce::PathStrokeType (3.0f));

    const float angle = bipolar
        ? start + (value - 0.5f) * juce::MathConstants<float>::pi
        : start + value * juce::MathConstants<float>::twoPi;

    const auto pointer = juce::Point<float> {
        centre.x + std::cos (angle) * circle.getWidth() * 0.28f,
        centre.y + std::sin (angle) * circle.getWidth() * 0.28f
    };

    g.setColour (accentColour.withAlpha (0.92f));
    g.drawLine (centre.x, centre.y, pointer.x, pointer.y, 2.0f);
}

void SimpleRotaryComponent::mouseDown (const juce::MouseEvent& e)
{
    dragStartY = e.position.y;
    dragStartValue = value;
}

void SimpleRotaryComponent::mouseDrag (const juce::MouseEvent& e)
{
    setValue (dragStartValue + (dragStartY - e.position.y) * 0.006f);
}

LowerSectionComponent::LowerSectionComponent()
{
    for (int i = 0; i < 10; ++i)
    {
        auto& rotary = rotaries[(size_t) i];

        addAndMakeVisible (rotary);
        rotary.setBipolar (i >= 5);
        rotary.setValue (i >= 5 ? 0.5f : 0.35f + (float) i * 0.06f);
    }
}

void LowerSectionComponent::setRotaryColour (juce::Colour newColour)
{
    for (auto& rotary : rotaries)
        rotary.setAccentColour (newColour);
}

void LowerSectionComponent::resized()
{
    const auto bounds = getLocalBounds().toFloat();
    const float gap = 16.0f;
    const float size = juce::jmin ((bounds.getWidth() - gap * 5.0f) / 6.0f,
                                   (bounds.getHeight() - gap) / 2.0f);

    const float totalW = size * 6.0f + gap * 5.0f;
    const float totalH = size * 2.0f + gap;
    const float startX = bounds.getCentreX() - totalW * 0.5f;
    const float startY = bounds.getCentreY() - totalH * 0.5f;

    for (int i = 0; i < 10; ++i)
    {
        const int col = i % 5;
        const int row = i / 5;

        rotaries[(size_t) i].setBounds (juce::Rectangle<float> (
            startX + (float) col * (size + gap),
            startY + (float) row * (size + gap),
            size,
            size).toNearestInt());
    }

    for (int row = 0; row < 2; ++row)
    {
        steppers[(size_t) row] = juce::Rectangle<float> (
            startX + 5.0f * (size + gap),
            startY + (float) row * (size + gap),
            size,
            size);
    }
}

void LowerSectionComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    drawFrame (g, bounds, juce::Colour::fromRGB (178, 255, 36));

    for (int i = 0; i < 2; ++i)
        drawStepper (g, steppers[(size_t) i], hoveredStepper == i);
}

void LowerSectionComponent::mouseMove (const juce::MouseEvent& e)
{
    int newHover = -1;

    for (int i = 0; i < 2; ++i)
        if (steppers[(size_t) i].contains (e.position))
            newHover = i;

    if (hoveredStepper != newHover)
    {
        hoveredStepper = newHover;
        repaint();
    }
}

void LowerSectionComponent::mouseExit (const juce::MouseEvent&)
{
    hoveredStepper = -1;
    repaint();
}

void LowerSectionComponent::drawStepper (juce::Graphics& g, juce::Rectangle<float> bounds, bool hovered)
{
    g.setColour (juce::Colours::white.withAlpha (hovered ? 0.10f : 0.055f));
    g.fillRect (bounds);

    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawRect (bounds.toNearestInt(), 1);

    const auto top = bounds.withTrimmedBottom (bounds.getHeight() * 0.5f);
    const auto bottom = bounds.withTrimmedTop (bounds.getHeight() * 0.5f);

    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.drawLine (bounds.getX(), bounds.getCentreY(), bounds.getRight(), bounds.getCentreY(), 1.0f);

    auto drawArrow = [&] (juce::Rectangle<float> area, bool up)
    {
        const auto c = area.getCentre();

        juce::Path arrow;

        if (up)
        {
            arrow.startNewSubPath (c.x - 6.0f, c.y + 4.0f);
            arrow.lineTo (c.x, c.y - 5.0f);
            arrow.lineTo (c.x + 6.0f, c.y + 4.0f);
        }
        else
        {
            arrow.startNewSubPath (c.x - 6.0f, c.y - 4.0f);
            arrow.lineTo (c.x, c.y + 5.0f);
            arrow.lineTo (c.x + 6.0f, c.y - 4.0f);
        }

        g.setColour (juce::Colours::white.withAlpha (0.72f));
        g.strokePath (arrow, juce::PathStrokeType (1.7f));
    };

    drawArrow (top, true);
    drawArrow (bottom, false);
}

PluginUxPage::PluginUxPage()
{
    guideImage = juce::ImageFileFormat::loadFrom (findGuideImageFile());

    addAndMakeVisible (wavePreview);
    addAndMakeVisible (xyPad);
    addAndMakeVisible (padGrid);
    addAndMakeVisible (lowerSection);

    padGrid.onSelectedPadChanged = [this] (int pad)
    {
        const auto colour = getPadColour (pad);
        xyPad.setPointColour (colour);
        lowerSection.setRotaryColour (colour);
    };

    lowerSection.setRotaryColour (getPadColour (0));

    if (guideImage.isValid())
        setSize (guideImage.getWidth() / 2, guideImage.getHeight() / 2);
    else
        setSize (632, 736);
}

void PluginUxPage::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (18, 18, 20));

    auto guide2 = juce::ImageCache::getFromMemory (BinaryData::guide2_png,
                                                   BinaryData::guide2_pngSize);

    if (guide2.isValid())
    {
        g.drawImage (guide2,
                     getLocalBounds().toFloat(),
                     juce::RectanglePlacement::stretchToFit);
    }
}

void PluginUxPage::resized()
{
    auto area = getLocalBounds().toFloat();

    const float contentX = 32.0f;
    const float contentW = 568.0f;
    const float gap = 16.0f;

    wavePreview.setBounds (juce::Rectangle<float> (32.0f, 72.0f, 568.0f, 64.0f).toNearestInt());

    const float xyY = 152.0f;
    const float xyH = 256.0f;

    xyPad.setBounds (juce::Rectangle<float> (contentX, xyY, contentW, xyH).toNearestInt());

    const float padY = 415.0f;
    const float padH = 96.0f;

    padGrid.setBounds (juce::Rectangle<float> (contentX, padY, contentW, padH).toNearestInt());

    const float lowerY = padY + padH + gap;
    lowerSection.setBounds (juce::Rectangle<float> (contentX, lowerY, contentW, area.getBottom() - lowerY - 32.0f).toNearestInt());
}
