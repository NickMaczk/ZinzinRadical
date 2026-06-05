#include "MainComponent.h"

MainComponent::MainComponent()
{
    addAndMakeVisible (zinzinPage);
    addAndMakeVisible (pluginUxPage);

    addAndMakeVisible (zinzinTab);
    addAndMakeVisible (pluginUxTab);

    zinzinTab.onClick = [this] { showPage (0); };
    pluginUxTab.onClick = [this] { showPage (1); };

    setSize (632, 840);
    showPage (0);

    juce::MessageManager::callAsync ([safeThis = juce::Component::SafePointer<MainComponent> (this)]
    {
        if (safeThis != nullptr)
            safeThis->showPage (safeThis->selectedPage);
    });
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (18, 18, 20));
}

void MainComponent::resized()
{
    const int baseW = 632;
    const int baseH = selectedPage == 0 ? 840 : 808;

    const float scaleX = (float) getWidth() / (float) baseW;
    const float scaleY = (float) getHeight() / (float) baseH;
    const float scale = juce::jmin (scaleX, scaleY);

    zinzinPage.setTransform ({});
    pluginUxPage.setTransform ({});

    zinzinPage.setBounds (0, 0, 632, 840);
    pluginUxPage.setBounds (0, 0, 632, 808);

    const float offsetX = ((float) getWidth() - (float) baseW * scale) * 0.5f;
    const float offsetY = ((float) getHeight() - (float) baseH * scale) * 0.5f;

    auto transform = juce::AffineTransform::scale (scale)
                        .translated (offsetX, offsetY);

    zinzinPage.setTransform (transform);
    pluginUxPage.setTransform (transform);

    zinzinTab.setBounds (8, 8, 84, 22);
    pluginUxTab.setBounds (96, 8, 104, 22);

    zinzinTab.toFront (false);
    pluginUxTab.toFront (false);
}

void MainComponent::showPage (int pageIndex)
{
    selectedPage = pageIndex;

    const int targetW = 632;
    const int targetH = selectedPage == 0 ? 840 : 808;

    if (auto* window = dynamic_cast<juce::ResizableWindow*> (getTopLevelComponent()))
    {
        if (auto* constrainer = window->getConstrainer())
        {
            constrainer->setFixedAspectRatio ((double) targetW / (double) targetH);
            constrainer->setSizeLimits (targetW / 2, targetH / 2, targetW * 2, targetH * 2);
        }

        window->setContentComponentSize (targetW, targetH);
    }
    else
    {
        setSize (targetW, targetH);
    }

    zinzinPage.setVisible (selectedPage == 0);
    pluginUxPage.setVisible (selectedPage == 1);

    updateTabStyle();

    zinzinTab.toFront (false);
    pluginUxTab.toFront (false);
}

void MainComponent::updateTabStyle()
{
    auto styleButton = [] (juce::TextButton& button, bool selected)
    {
        button.setColour (juce::TextButton::buttonColourId,
                          selected ? juce::Colours::white.withAlpha (0.22f)
                                   : juce::Colours::black.withAlpha (0.36f));

        button.setColour (juce::TextButton::textColourOffId,
                          selected ? juce::Colours::white
                                   : juce::Colours::white.withAlpha (0.58f));

        button.setColour (juce::TextButton::buttonOnColourId,
                          juce::Colours::white.withAlpha (0.22f));
    };

    styleButton (zinzinTab, selectedPage == 0);
    styleButton (pluginUxTab, selectedPage == 1);

    repaint();
}
