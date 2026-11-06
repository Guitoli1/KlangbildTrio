#include "PluginEditor.h"

using namespace vintage;

namespace
{
juce::String u8 (const char* t) { return juce::String (juce::CharPointer_UTF8 (t)); }

const juce::Colour lacquer   { 0xff5f7487 };   // blaugrauer Hammerschlaglack
const juce::Colour silk      { 0xfff2eee4 };   // weißer Siebdruck
const juce::Colour silkDim   { 0xc8f2eee4 };
}

//==============================================================================
void RackLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& slider)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float knobR = juce::jmax (16.0f, R - 27.0f);
    const float angle = startAngle + pos * (endAngle - startAngle);

    // Skala im Siebdruck
    const auto steps = juce::StringArray::fromTokens (slider.getProperties()["stepLabels"].toString(), "|", "");
    g.setColour (silk);
    if (steps.size() > 1)
    {
        for (int i = 0; i < steps.size(); ++i)
        {
            const float a = startAngle + (float) i / (float) (steps.size() - 1) * (endAngle - startAngle);
            const auto p1 = c.getPointOnCircumference (knobR + 4.0f, a), p2 = c.getPointOnCircumference (knobR + 11.0f, a);
            g.drawLine ({ p1, p2 }, 1.6f);
            if (steps[i].isNotEmpty())
            {
                g.setFont (labelFont (steps[i].length() > 3 ? 10.5f : 12.0f, 0.03f));
                const auto tp = c.getPointOnCircumference (knobR + (steps[i].length() > 3 ? 22.0f : 19.0f), a);
                g.drawText (steps[i], juce::Rectangle<float> (56.0f, 16.0f).withCentre (tp), juce::Justification::centred);
            }
        }
    }
    else
    {
        for (int i = 0; i <= 20; ++i)
        {
            const float a = startAngle + (float) i / 20.0f * (endAngle - startAngle);
            const bool major = i % 2 == 0;
            g.drawLine ({ c.getPointOnCircumference (knobR + 4.0f, a), c.getPointOnCircumference (knobR + (major ? 11.0f : 7.0f), a) }, major ? 1.6f : 1.0f);
            if (major)
            {
                g.setFont (labelFont (12.0f, 0.03f));
                g.drawText (juce::String (i / 2), juce::Rectangle<float> (24.0f, 14.0f).withCentre (c.getPointOnCircumference (knobR + 19.0f, a)), juce::Justification::centred);
            }
        }
    }

    // Schatten
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (juce::Rectangle<float> (knobR * 2.0f, knobR * 2.0f).withCentre (c.translated (2.5f, 4.0f)));
    // Rock (geriffelt)
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3836), c.x, c.y - knobR, juce::Colour (0xff0c0b0a), c.x, c.y + knobR, false));
    g.fillEllipse (juce::Rectangle<float> (knobR * 2.0f, knobR * 2.0f).withCentre (c));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    for (int k = 0; k < 48; ++k)
    {
        const float a = (float) k / 48.0f * juce::MathConstants<float>::twoPi;
        g.drawLine ({ c.getPointOnCircumference (knobR * 0.86f, a), c.getPointOnCircumference (knobR - 0.5f, a) }, 1.2f);
    }
    // Kappe
    const float capR = knobR * 0.74f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4744), c.x - capR * 0.5f, c.y - capR, juce::Colour (0xff161514), c.x + capR * 0.4f, c.y + capR, false));
    g.fillEllipse (juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c));
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.fillEllipse (juce::Rectangle<float> (capR * 1.1f, capR * 0.55f).withCentre (c.translated (-capR * 0.2f, -capR * 0.5f)));
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c), 1.0f);
    // weiße Zeigerlinie über Kappe und Rock
    g.setColour (juce::Colour (0xfff5f1e6));
    g.drawLine ({ c.getPointOnCircumference (capR * 0.25f, angle), c.getPointOnCircumference (knobR - 1.5f, angle) }, 3.0f);
}

//==============================================================================
TrioAudioProcessorEditor::TrioAudioProcessorEditor (TrioAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    const auto minus = u8 ("\xe2\x88\x92");

    for (int b = 0; b < TrioAudioProcessor::numBands; ++b)
    {
        addKnob (TrioAudioProcessor::bid ("thr", b),   "SCHWELLE (DB)", minus + "60|" + minus + "45|" + minus + "30|" + minus + "15|0");
        addKnob (TrioAudioProcessor::bid ("ratio", b), "RATIO", "1|2|4|8|16");
        addKnob (TrioAudioProcessor::bid ("att", b),   "ATTACK (MS)", "0,1|1|10|100");
        addKnob (TrioAudioProcessor::bid ("rel", b),   "RELEASE (MS)", "20|200|2000");
        addKnob (TrioAudioProcessor::bid ("gain", b),  "GAIN (DB)", minus + "12|" + minus + "6|0|+6|+12");
        addToggle (TrioAudioProcessor::bid ("solo", b), "SOLO", "AUS", lampAmber);
        addToggle (TrioAudioProcessor::bid ("byp", b),  "BYPASS", "AKTIV", lampAmber);
        vus[(size_t) b] = std::make_unique<VuMeter> (lnf, "GR");
        addAndMakeVisible (*vus[(size_t) b]);
    }
    addKnob ("x1", "TIEF / MITTE (HZ)", "40|200|1000");
    addKnob ("x2", u8 ("MITTE / H\xc3\x96HE (HZ)"), "800|3200|12800");
    addKnob ("mix", "MIX (%)", "0|25|50|75|100");
    addKnob ("output", "AUSGANG (DB)", minus + "12|" + minus + "6|0|+6|+12");
    addToggle ("on", "EIN", "AUS", lampRed);

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    setSize (rackW, rackH);
    startTimerHz (60);
}

TrioAudioProcessorEditor::~TrioAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

juce::Slider& TrioAudioProcessorEditor::addKnob (const juce::String& paramID, const juce::String& name, const juce::String& steps)
{
    auto k = std::make_unique<Knob>();
    k->name = name;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    k->slider.setMouseDragSensitivity (240);
    k->slider.getProperties().set ("stepLabels", steps);
    addAndMakeVisible (k->slider);
    k->attachment = std::make_unique<SliderAttachment> (proc.apvts, paramID, k->slider);
    if (auto* rp = proc.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
    knobs.push_back (std::move (k));
    return knobs.back()->slider;
}

juce::ToggleButton& TrioAudioProcessorEditor::addToggle (const juce::String& paramID, const juce::String& onText, const juce::String& offText, juce::Colour lamp)
{
    auto b = std::make_unique<juce::ToggleButton>();
    b->setButtonText ("x");
    b->getProperties().set ("onText", onText);
    b->getProperties().set ("offText", offText);
    b->getProperties().set ("lamp", (int) lamp.getARGB());
    b->setColour (juce::ToggleButton::textColourId, silk);
    addAndMakeVisible (*b);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, paramID, *b));
    toggles.push_back (std::move (b));
    return *toggles.back();
}

void TrioAudioProcessorEditor::timerCallback()
{
    constexpr float dt = 1.0f / 60.0f;
    // Pegelreduktion: Zeiger steht ohne Kompression auf 0 VU und wandert nach links
    for (int b = 0; b < 3; ++b)
        vus[(size_t) b]->update (juce::Decibels::decibelsToGain (-proc.grDb[(size_t) b].load() - 18.0f) / 1.1107f, 0.0f, dt);

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

//==============================================================================
void TrioAudioProcessorEditor::renderPanel()
{
    panelImage = juce::Image (juce::Image::ARGB, rackW, rackH, true);
    juce::Graphics g (panelImage);
    g.setGradientFill (juce::ColourGradient (lacquer.brighter (0.12f), 0, 0, lacquer.darker (0.18f), 0, (float) rackH, false));
    g.fillRect (0, 0, rackW, rackH);
    juce::Random rnd (13);
    for (int i = 0; i < 12000; ++i)
    {
        const float r = 1.5f + rnd.nextFloat() * 4.0f;
        const float x = rnd.nextFloat() * rackW, y = rnd.nextFloat() * rackH;
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.012f + rnd.nextFloat() * 0.02f));
        g.fillEllipse (x - r, y - r, 2 * r, 2 * r);
    }
    {
        juce::Image::BitmapData data (panelImage, juce::Image::BitmapData::readWrite);
        for (int y = 0; y < rackH; ++y)
            for (int x = 0; x < rackW; ++x)
            {
                auto c = data.getPixelColour (x, y);
                const int n = rnd.nextInt (9) - 4;
                data.setPixelColour (x, y, juce::Colour ((juce::uint8) juce::jlimit (0, 255, c.getRed() + n),
                                                         (juce::uint8) juce::jlimit (0, 255, c.getGreen() + n),
                                                         (juce::uint8) juce::jlimit (0, 255, c.getBlue() + n)));
            }
    }
    g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawHorizontalLine (0, 0.0f, (float) rackW);
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.drawHorizontalLine (rackH - 1, 0.0f, (float) rackW);

    // Rack-Ohren mit Langlöchern
    for (float ex : { 0.0f, (float) rackW - 56.0f })
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawVerticalLine ((int) (ex < 1.0f ? 56.0f : ex), 0.0f, (float) rackH);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawVerticalLine ((int) (ex < 1.0f ? 57.0f : ex + 1.0f), 0.0f, (float) rackH);
        for (float sy : { 44.0f, (float) rackH * 0.5f, (float) rackH - 44.0f })
        {
            const auto slot = juce::Rectangle<float> (ex + 14.0f, sy - 8.0f, 28.0f, 16.0f);
            g.setColour (juce::Colour (0xff141210));
            g.fillRoundedRectangle (slot, 8.0f);
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.drawRoundedRectangle (slot.translated (0.0f, 1.0f), 8.0f, 1.0f);
            VintageLookAndFeel::drawScrew (g, slot.getCentre().translated (ex < 1.0f ? 3.0f : -3.0f, 0.0f), 6.5f, sy * 0.01f + ex * 0.003f);
        }
    }

    const juce::String titles[] = { "TRENNUNG", "TIEFEN", "MITTEN", u8 ("H\xc3\x96HEN"), "SUMME" };
    for (size_t i = 0; i < sections.size(); ++i)
    {
        const auto r = sections[i].toFloat();
        g.setColour (silkDim);
        juce::Path box; box.addRoundedRectangle (r, 5.0f);
        g.strokePath (box, juce::PathStrokeType (1.2f));
        const auto f = lnf.labelFont (13.0f, 0.3f);
        const float tw = (float) juce::GlyphArrangement::getStringWidthInt (f, titles[i]) + 20.0f;
        const auto tr = juce::Rectangle<float> (tw, 16.0f).withCentre ({ r.getCentreX(), r.getY() });
        g.setColour (lacquer);
        g.fillRect (tr);
        g.setColour (silk);
        g.setFont (f);
        g.drawText (titles[i], tr, juce::Justification::centred);
    }

    // VU-Rahmen
    for (auto& vb : vuBezels)
    {
        const auto r = vb.toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (r.translated (2.0f, 3.0f), 10.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2a28), 0, r.getY(), juce::Colour (0xff121110), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 10.0f);
    }

    for (auto& k : knobs)
    {
        const auto sb = k->slider.getBounds().toFloat();
        g.setColour (silk);
        g.setFont (lnf.labelFont (12.0f, 0.16f));
        g.drawText (k->name, juce::Rectangle<float> (sb.getX() - 30.0f, sb.getBottom() - 8.0f, sb.getWidth() + 60.0f, 16.0f), juce::Justification::centred);
    }
    for (auto& [r, t] : toggleLabels)
    {
        g.setColour (silk);
        g.setFont (lnf.labelFont (11.0f, 0.22f));
        g.drawText (t, r.toFloat(), juce::Justification::centredLeft);
    }

    // Typenschild
    const auto np = nameplate.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (np.translated (1.5f, 2.5f), 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2522), 0, np.getY(), juce::Colour (0xff121010), 0, np.getBottom(), false));
    g.fillRoundedRectangle (np, 4.0f);
    g.setColour (gold.withAlpha (0.7f));
    g.drawRoundedRectangle (np.reduced (3.0f), 3.0f, 1.0f);
    g.setFont (lnf.scriptFont (34.0f));
    g.setColour (logo);
    g.drawText ("Klangbild", np.withTrimmedLeft (12.0f).withHeight (44.0f).translated (0.0f, 2.0f), juce::Justification::centredLeft);
    g.setColour (gold);
    g.setFont (lnf.labelFont (17.0f, 0.45f));
    g.drawText ("TRIO", np.withTrimmedLeft (16.0f).withTrimmedTop (44.0f).withHeight (20.0f), juce::Justification::centredLeft);
    g.setColour (logo.withAlpha (0.6f));
    g.setFont (lnf.labelFont (9.0f, 0.2f));
    g.drawText ("DREI-BAND-KOMPRESSOR", np.withTrimmedLeft (16.0f).withTrimmedTop (66.0f).withHeight (14.0f), juce::Justification::centredLeft);

    g.setColour (silk);
    g.setFont (lnf.labelFont (11.0f, 0.22f));
    g.drawText ("PROGRAMM", presetBox.getBounds().toFloat().withY ((float) presetBox.getY() - 16.0f).withHeight (14.0f), juce::Justification::centredLeft);
    g.setColour (silkDim);
    g.setFont (lnf.labelFont (10.0f, 0.3f));
    g.drawText (u8 ("TYP KB-3B  \xc2\xb7  NR. 0613"), tagArea.toFloat(), juce::Justification::centredRight);
}

void TrioAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (panelImage, 0, 0);
}

void TrioAudioProcessorEditor::resized()
{
    const int top = 30, bottom = rackH - 34;
    sections[0] = { 72, top, 150, bottom - top };
    for (int b = 0; b < 3; ++b)
        sections[(size_t) b + 1] = { 234 + b * 268, top, 258, bottom - top };
    const int colX = 1038, colW = rackW - 56 - 16 - colX;
    sections[4] = { colX, top + 176, colW, bottom - top - 176 };

    toggleLabels.clear();
    const int K = 124;   // Knopf-Fläche
    for (int b = 0; b < 3; ++b)
    {
        const auto s = sections[(size_t) b + 1];
        vuBezels[(size_t) b] = juce::Rectangle<int> (232, 146).withCentre ({ s.getCentreX(), s.getY() + 96 });
        vus[(size_t) b]->setBounds (vuBezels[(size_t) b].reduced (8));
        const int lx = s.getX() + 6, rx = s.getRight() - 6 - K;
        auto kn = [&] (int idx) -> juce::Slider& { return knobs[(size_t) (b * 5 + idx)]->slider; };
        kn (0).setBounds (lx, s.getY() + 182, K, K);   // Schwelle
        kn (1).setBounds (rx, s.getY() + 182, K, K);   // Ratio
        kn (2).setBounds (lx, s.getY() + 322, K, K);   // Attack
        kn (3).setBounds (rx, s.getY() + 322, K, K);   // Release
        kn (4).setBounds (lx, s.getY() + 462, K, K);   // Gain
        const int tx = s.getX() + 140;
        toggles[(size_t) (b * 2)]->setBounds (tx, s.getY() + 486, 120, 38);
        toggles[(size_t) (b * 2 + 1)]->setBounds (tx, s.getY() + 548, 120, 38);
        toggleLabels.push_back ({ { tx + 4, s.getY() + 470, 110, 14 }, "SOLO" });
        toggleLabels.push_back ({ { tx + 4, s.getY() + 532, 110, 14 }, "BAND" });
    }
    // Trennung
    knobs[15]->slider.setBounds (sections[0].getX() + 5, top + 120, 140, 140);
    knobs[16]->slider.setBounds (sections[0].getX() + 5, top + 330, 140, 140);

    // rechte Spalte
    nameplate = { colX, top - 8, colW, 88 };
    presetBox.setBounds (colX, top + 112, colW, 28);
    knobs[17]->slider.setBounds (colX + (colW - 140) / 2, top + 200, 140, 140);
    knobs[18]->slider.setBounds (colX + (colW - 140) / 2, top + 360, 140, 140);
    toggles[6]->setBounds (colX + 14, top + 548, 130, 40);
    toggleLabels.push_back ({ { colX + 18, top + 532, 120, 14 }, "KOMPRESSOR" });
    tagArea = { colX - 60, rackH - 28, colW + 60, 16 };

    renderPanel();
}
