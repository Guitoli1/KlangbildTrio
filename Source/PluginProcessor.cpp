#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
using APF = juce::AudioParameterFloat;
using Attr = juce::AudioParameterFloatAttributes;

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return { lo, hi,
             [] (float a, float b, float n) { return a * std::pow (b / a, n); },
             [] (float a, float b, float v) { return std::log (v / a) / std::log (b / a); },
             [] (float a, float b, float v) { return juce::jlimit (a, b, v); } };
}

Attr unit (const char* u, int decimals, bool sign = false)
{
    return Attr().withStringFromValueFunction ([=] (float v, int)
                 {
                     auto s = juce::String (std::abs (v) < 0.05f && decimals > 0 ? 0.0f : v, decimals).replaceCharacter ('.', ',');
                     if (sign && v > 0.049f) s = "+" + s;
                     return (s + " " + u).trim();
                 })
                 .withValueFromStringFunction ([] (const juce::String& s) { return s.replaceCharacter (',', '.').getFloatValue(); });
}

const char* bandNames[] = { "Tiefen", "Mitten", "H\xc3\xb6hen" };
}

juce::AudioProcessorValueTreeState::ParameterLayout TrioAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<APF> (juce::ParameterID { "x1", 1 }, "Trennung Tiefen/Mitten", logRange (40.0f, 1000.0f), 180.0f, unit ("Hz", 0)));
    l.add (std::make_unique<APF> (juce::ParameterID { "x2", 1 }, juce::String (juce::CharPointer_UTF8 ("Trennung Mitten/H\xc3\xb6hen")), logRange (800.0f, 12800.0f), 3200.0f, unit ("Hz", 0)));
    for (int b = 0; b < numBands; ++b)
    {
        const auto n = juce::String (juce::CharPointer_UTF8 (bandNames[b])) + " ";
        l.add (std::make_unique<APF> (juce::ParameterID { bid ("thr", b), 1 }, n + "Schwelle", juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -18.0f, unit ("dB", 1)));
        l.add (std::make_unique<APF> (juce::ParameterID { bid ("ratio", b), 1 }, n + "Ratio", logRange (1.0f, 16.0f), 2.0f, unit (": 1", 1)));
        l.add (std::make_unique<APF> (juce::ParameterID { bid ("att", b), 1 }, n + "Attack", logRange (0.1f, 100.0f), b == 0 ? 20.0f : (b == 1 ? 10.0f : 5.0f), unit ("ms", 1)));
        l.add (std::make_unique<APF> (juce::ParameterID { bid ("rel", b), 1 }, n + "Release", logRange (20.0f, 2000.0f), b == 0 ? 250.0f : (b == 1 ? 150.0f : 100.0f), unit ("ms", 0)));
        l.add (std::make_unique<APF> (juce::ParameterID { bid ("gain", b), 1 }, n + "Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, unit ("dB", 1, true)));
        l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { bid ("solo", b), 1 }, n + "Solo", false));
        l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { bid ("byp", b), 1 }, n + "Bypass", false));
    }
    l.add (std::make_unique<APF> (juce::ParameterID { "mix", 1 }, "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, unit ("%", 0)));
    l.add (std::make_unique<APF> (juce::ParameterID { "output", 1 }, "Ausgang", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, unit ("dB", 1, true)));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "on", 1 }, "Kompressor an", true));
    return l;
}

const std::vector<TrioAudioProcessor::Preset>& TrioAudioProcessor::presets()
{
    // Werte je Band: Schwelle, Ratio, Attack, Release, Gain
    auto bands = [] (std::array<std::array<float, 5>, 3> v, float x1, float x2)
    {
        std::vector<std::pair<juce::String, float>> out { { "x1", x1 }, { "x2", x2 } };
        const char* ids[] = { "thr", "ratio", "att", "rel", "gain" };
        for (int b = 0; b < 3; ++b)
            for (int k = 0; k < 5; ++k)
                out.push_back ({ bid (ids[k], b), v[(size_t) b][(size_t) k] });
        return out;
    };
    static const std::vector<Preset> p = {
        { "Standard",              bands ({{ { -18, 2, 20, 250, 0 }, { -18, 2, 10, 150, 0 }, { -18, 2, 5, 100, 0 } }}, 180, 3200) },
        { "Mastering sanft",       bands ({{ { -20, 1.6f, 30, 300, 0.5f }, { -18, 1.4f, 20, 200, 0.5f }, { -20, 1.6f, 10, 150, 0.5f } }}, 150, 4000) },
        { "Bass bändigen",         bands ({{ { -24, 4, 15, 200, 1 }, { 0, 1, 10, 150, 0 }, { 0, 1, 5, 100, 0 } }}, 120, 3200) },
        { "Zischlaute zähmen",     bands ({{ { 0, 1, 20, 250, 0 }, { 0, 1, 10, 150, 0 }, { -30, 6, 0.5f, 60, 0 } }}, 180, 5600) },
        { "Gitarre glätten",       bands ({{ { -24, 2, 20, 200, 0 }, { -26, 3, 8, 120, 1 }, { -28, 2.5f, 5, 100, 0 } }}, 250, 2500) },
        { "Drums mit Punch",       bands ({{ { -22, 3, 30, 150, 2 }, { -20, 2.5f, 25, 120, 1 }, { -22, 2, 15, 80, 1 } }}, 120, 5000) },
        { "Gesang dicht",          bands ({{ { -26, 2, 15, 200, 0 }, { -24, 3, 5, 150, 2 }, { -26, 3, 2, 80, 0 } }}, 200, 4500) },
    };
    return p;
}

void TrioAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size()))
        return;
    currentProgram = index;
    for (auto& [id, value] : presets()[(size_t) index].values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    for (int b = 0; b < numBands; ++b)
        for (const char* id : { "solo", "byp" })
            if (auto* param = apvts.getParameter (bid (id, b)))
                param->setValueNotifyingHost (0.0f);
}

const juce::String TrioAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) presets().size())
               ? juce::String (juce::CharPointer_UTF8 (presets()[(size_t) index].name)) : juce::String();
}

TrioAudioProcessor::TrioAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KlangbildTrio", createLayout())
{
    for (auto& g : grDb) g.store (0.0f);
}

bool TrioAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out;
}

void TrioAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    lastF1 = lastF2 = -1;
    for (auto& x : xover) x.reset();
    for (auto& c : comp) c.prepare (sr);
    auto v = [this] (const juce::String& id) { return apvts.getRawParameterValue (id)->load(); };
    for (int b = 0; b < numBands; ++b)
    {
        smGain[(size_t) b].reset (sr, 0.03); smGain[(size_t) b].setCurrentAndTargetValue (juce::Decibels::decibelsToGain (v (bid ("gain", b))));
        smOn[(size_t) b].reset (sr, 0.01);   smOn[(size_t) b].setCurrentAndTargetValue (1.0f);
        grHold[(size_t) b] = 0;
    }
    smMix.reset (sr, 0.03);    smMix.setCurrentAndTargetValue (v ("mix") * 0.01f);
    smOut.reset (sr, 0.03);    smOut.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (v ("output")));
    smActive.reset (sr, 0.02); smActive.setCurrentAndTargetValue (v ("on") > 0.5f ? 1.0f : 0.0f);
    vuCoef = (float) (1.0 - std::exp (-1.0 / (0.01 * sr)));
    vuState = 0;
}

void TrioAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nch = juce::jmin (2, buffer.getNumChannels());
    if (nch == 0)
        return;

    auto v = [this] (const juce::String& id) { return apvts.getRawParameterValue (id)->load(); };
    const double f1 = v ("x1"), f2 = v ("x2");
    if (! juce::exactlyEqual (f1, lastF1) || ! juce::exactlyEqual (f2, lastF2))
    {
        lastF1 = f1; lastF2 = f2;
        for (auto& x : xover) x.setFrequencies (sr, f1, f2);
    }

    bool anySolo = false;
    std::array<bool, numBands> byp {};
    for (int b = 0; b < numBands; ++b)
    {
        anySolo = anySolo || v (bid ("solo", b)) > 0.5f;
        byp[(size_t) b] = v (bid ("byp", b)) > 0.5f;
        comp[(size_t) b].setParams (v (bid ("thr", b)), v (bid ("ratio", b)), v (bid ("att", b)), v (bid ("rel", b)));
        smGain[(size_t) b].setTargetValue (byp[(size_t) b] ? 1.0f : juce::Decibels::decibelsToGain (v (bid ("gain", b))));
    }
    for (int b = 0; b < numBands; ++b)
        smOn[(size_t) b].setTargetValue (! anySolo || v (bid ("solo", b)) > 0.5f ? 1.0f : 0.0f);
    smMix.setTargetValue (v ("mix") * 0.01f);
    smOut.setTargetValue (juce::Decibels::decibelsToGain (v ("output")));
    smActive.setTargetValue (v ("on") > 0.5f ? 1.0f : 0.0f);

    float* L = buffer.getWritePointer (0);
    float* R = nch > 1 ? buffer.getWritePointer (1) : nullptr;
    std::array<float, numBands> grMax { 0, 0, 0 };
    float pk = 0;

    for (int i = 0; i < n; ++i)
    {
        const float dl = L[i], dr = R != nullptr ? R[i] : dl;
        std::array<float, 3> bl {}, br {};
        xover[0].process (dl, bl[0], bl[1], bl[2]);
        if (R != nullptr) xover[1].process (dr, br[0], br[1], br[2]); else br = bl;

        const float m = smMix.getNextValue();
        float yl = 0, yr = 0;
        for (int b = 0; b < numBands; ++b)
        {
            const size_t k = (size_t) b;
            const float g = byp[k] ? 1.0f : comp[k].process (std::max (std::abs (bl[k]), std::abs (br[k])));
            if (! byp[k]) grMax[k] = std::max (grMax[k], comp[k].reductionDb());
            const float f = smOn[k].getNextValue() * ((1.0f - m) + m * g * smGain[k].getNextValue());
            yl += bl[k] * f; yr += br[k] * f;
        }
        const float go = smOut.getNextValue(), act = smActive.getNextValue();
        yl = dl + (yl * go - dl) * act;
        yr = dr + (yr * go - dr) * act;
        L[i] = yl;
        if (R != nullptr) R[i] = yr;
        pk = juce::jmax (pk, std::abs (yl), std::abs (yr));
        vuState += (0.5f * (std::abs (yl) + std::abs (yr)) - vuState) * vuCoef;
    }

    // Anzeige: schnell rein, langsam raus
    const bool active = v ("on") > 0.5f;
    for (int b = 0; b < numBands; ++b)
    {
        auto& h = grHold[(size_t) b];
        h = grMax[(size_t) b] > h ? grMax[(size_t) b] : h + (grMax[(size_t) b] - h) * 0.2f;
        grDb[(size_t) b].store (active ? h : 0.0f);
    }
    vuOut.store (vuState);
    peakOut.store (juce::jmax (pk, peakOut.load()));
}

void TrioAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void TrioAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;
    auto tree = juce::ValueTree::fromXml (*xml);
    currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
    apvts.replaceState (tree);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->convertTo0to1 (rp->convertFrom0to1 (rp->getValue())));
}

juce::AudioProcessorEditor* TrioAudioProcessor::createEditor() { return new TrioAudioProcessorEditor (*this); }

#ifndef KB_EMBEDDED
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TrioAudioProcessor(); }
#endif
