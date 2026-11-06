#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VintageUI.h"

// Look im Stil eines 19-Zoll-Studiogeräts: blaugrauer Hammerschlaglack, weißer Siebdruck, schwarze Zeigerknöpfe
class RackLookAndFeel : public VintageLookAndFeel
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
};

class TrioAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TrioAudioProcessorEditor (TrioAudioProcessor&);
    ~TrioAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob { juce::Slider slider; juce::String name; std::unique_ptr<SliderAttachment> attachment; };

    juce::Slider& addKnob (const juce::String& paramID, const juce::String& name, const juce::String& steps);
    juce::ToggleButton& addToggle (const juce::String& paramID, const juce::String& onText, const juce::String& offText, juce::Colour lamp);
    void timerCallback() override;
    void renderPanel();

    TrioAudioProcessor& proc;
    RackLookAndFeel lnf;

    std::vector<std::unique_ptr<Knob>> knobs;                 // je Band 5, dann Trennung 2, Mix, Ausgang
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles; // je Band Solo, Bypass, dann Ein/Aus
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    juce::ComboBox presetBox;
    std::array<std::unique_ptr<VuMeter>, 3> vus;

    static constexpr int rackW = 1280, rackH = 700;
    juce::Rectangle<int> nameplate, tagArea;
    std::array<juce::Rectangle<int>, 5> sections;
    std::array<juce::Rectangle<int>, 3> vuBezels;
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> toggleLabels;
    juce::Image panelImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrioAudioProcessorEditor)
};
