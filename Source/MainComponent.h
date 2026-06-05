#pragma once

#include <JuceHeader.h>
#include "ZinzinPage.h"
#include "PluginUxPage.h"

class MainComponent  : public juce::Component
{
public:
    MainComponent();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void showPage (int pageIndex);
    void updateTabStyle();

    ZinzinPage zinzinPage;
    PluginUxPage pluginUxPage;

    juce::TextButton zinzinTab { "ZINZIN" };
    juce::TextButton pluginUxTab { "PLUGIN UX" };

    int selectedPage = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
