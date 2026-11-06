#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "TrioDsp.h"

class TrioAudioProcessor : public juce::AudioProcessor
{
public:
    TrioAudioProcessor();
    ~TrioAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    struct Preset { const char* name; std::vector<std::pair<juce::String, float>> values; };
    static const std::vector<Preset>& presets();
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static constexpr int numBands = 3;
    static juce::String bid (const char* base, int band) { return juce::String (base) + juce::String (band + 1); }

    juce::AudioProcessorValueTreeState apvts;
    std::array<std::atomic<float>, numBands> grDb;
    std::atomic<float> vuOut { 0 }, peakOut { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::array<trio::Crossover, 2> xover;
    std::array<trio::Compressor, numBands> comp;
    double lastF1 = -1, lastF2 = -1;
    std::array<juce::SmoothedValue<float>, numBands> smGain, smOn;
    juce::SmoothedValue<float> smMix, smOut, smActive;
    std::array<float, numBands> grHold { 0, 0, 0 };
    float vuState = 0, vuCoef = 0.001f;
    int currentProgram = 0;
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrioAudioProcessor)
};
