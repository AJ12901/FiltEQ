/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle, float rotaryEndAngle, juce::Slider &slider)
{
    using namespace juce;
    
    auto bounds = Rectangle<float>(x, y, width, height);
    
    g.setColour(Colour (0xff020d12));
//    g.setColour(Colour (0xff03141e));
    g.fillEllipse(bounds);

    
    if (auto *rsw1 = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;
        
        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rsw1->getTextHeight()*1.5);
        
        jassert(rotaryStartAngle < rotaryEndAngle);
        
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
        g.fillPath(p);
        
        float w = width*0.5;
        float h = height*0.5;
        
        Path curve;
        curve.addCentredArc(center.getX(), center.getY(), w, h, rotaryStartAngle-juce::MathConstants<float>::pi, juce::MathConstants<float>::pi, sliderAngRad-(juce::MathConstants<float>::pi/4), true);
        g.setColour(Colours::teal);
        g.strokePath (curve, PathStrokeType(3.0));
        
        g.setFont(rsw1->getTextHeight());
        auto text = rsw1->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        
        r.setSize(strWidth + 4, rsw1->getTextHeight()+2);
        r.setCentre(bounds.getCentre());
        
        g.setColour(Colours::white);
        
//       float y = mx + c;
        
        g.setFont(11.25);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    juce::String str = juce::String(getValue());
    
    if (auto *choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();
    
    if (suffix.isNotEmpty())
    {
        str << " ";
        str << suffix;
    }
    return str;
}

void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;
    
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    
    auto range = getRange();
    auto sliderBounds = getSliderBounds();
    
//    g.setColour(Colours::red);
//    g.drawRect(getLocalBounds());
//    g.setColour(Colours::yellow);
//    g.drawRect(sliderBounds);
    
    getLookAndFeel().drawRotarySlider(g, sliderBounds.getX(), sliderBounds.getY(), sliderBounds.getWidth(), sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng, endAng, *this);
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    
    size = size - getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);
    
    return r;
}


ResponseCurveComponent::ResponseCurveComponent(FiltEQAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }
    updateChain();
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
    
}

void ResponseCurveComponent::parameterValueChanged (int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        updateChain();
        repaint();
    }
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    
    auto midCoefficients = makeMidFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Mid>().coefficients, midCoefficients);
    
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    
    g.fillAll (juce::Colour (0xff041e29));
    
    auto responseArea = getLocalBounds();
    auto w = responseArea.getWidth();
    
    auto &lowcut = monoChain.get<ChainPositions::LowCut>();
    auto &peak = monoChain.get<ChainPositions::Peak>();
    auto &highcut = monoChain.get<ChainPositions::HighCut>();
    auto &mid = monoChain.get<ChainPositions::Mid>();
    
    auto sampleRate = audioProcessor.getSampleRate();
    std::vector<double> mags;
    
    mags.resize(w);
    
    for (int i=0; i<w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double (i) / double(w), 20.0, 20000.0);
        
        if (!monoChain.isBypassed<ChainPositions::Peak>())
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if (!monoChain.isBypassed<ChainPositions::Mid>())
            mag *= mid.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if (!lowcut.isBypassed<0>())
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<1>())
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<2>())
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<3>())
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if (!highcut.isBypassed<0>())
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<1>())
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<2>())
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<3>())
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        mags[i] = Decibels::gainToDecibels(mag);
        
    }
    
    Path responseCurve;
    
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap (input, -24.0, 24.0, outputMin, outputMax);
    };
    
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    
    for (size_t i = 1; i<mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX()+i, map(mags[i]));
    }
    
    g.setColour (juce::Colour (0xff0b5574));
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
    g.setColour(Colours::cyan);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}


//==============================================================================
FiltEQAudioProcessorEditor::FiltEQAudioProcessorEditor (FiltEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),

peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Frequency"), "Hz"),
peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
lowCutFreqSlider(*audioProcessor.apvts.getParameter("Low Cut Freq"), "Hz"),
highCutFreqSlider(*audioProcessor.apvts.getParameter("High Cut Freq"), "Hz"),
lowCutSlopeSlider(*audioProcessor.apvts.getParameter("Low Cut Slope"), "dB/Oct"),
highCutSlopeSlider(*audioProcessor.apvts.getParameter("High Cut Slope"), "dB/Oct"),
midFreqSlider(*audioProcessor.apvts.getParameter("Mid Frequency"), "Hz"),
midGainSlider(*audioProcessor.apvts.getParameter("Mid Gain"), "dB"),
midQualitySlider(*audioProcessor.apvts.getParameter("Mid Quality"), ""),

responseCurveComponent(audioProcessor),
peakFreqSliderAttachment(audioProcessor.apvts, "Peak Frequency", peakFreqSlider),
peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
lowCutFreqSliderAttachment(audioProcessor.apvts, "Low Cut Freq", lowCutFreqSlider),
highCutFreqSliderAttachment(audioProcessor.apvts, "High Cut Freq", highCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.apvts, "Low Cut Slope", lowCutSlopeSlider),
highCutSlopeSliderAttachment(audioProcessor.apvts, "High Cut Slope", highCutSlopeSlider),
midFreqSliderAttachment(audioProcessor.apvts, "Mid Frequency", midFreqSlider),
midGainSliderAttachment(audioProcessor.apvts, "Mid Gain", midGainSlider),
midQualitySliderAttachment(audioProcessor.apvts, "Mid Quality", midQualitySlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for (auto* comp :getComps())
    {
        addAndMakeVisible(comp);
    }

    
    setSize (600, 400);
}

FiltEQAudioProcessorEditor::~FiltEQAudioProcessorEditor()
{

}

//==============================================================================
void FiltEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    g.fillAll (juce::Colour (0xff041e29));
}

void FiltEQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    float hRatio = 0.33; // JUCE_LIVE_CONSTANT(33)/100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);
    
    responseCurveComponent.setBounds(responseArea);
    
    bounds.removeFromTop(8);
    
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.3);
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5) );
    lowCutSlopeSlider.setBounds(lowCutArea);
    
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.4286);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5) );
    highCutSlopeSlider.setBounds(highCutArea);
    
    auto midCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    midFreqSlider.setBounds(midCutArea.removeFromTop(midCutArea.getHeight() * 0.33));
    midGainSlider.setBounds(midCutArea.removeFromTop(midCutArea.getHeight() * 0.5));
    midQualitySlider.setBounds(midCutArea);
    
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
    
} 

std::vector<juce::Component*> FiltEQAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider, &peakGainSlider, &peakQualitySlider, &lowCutFreqSlider, &highCutFreqSlider, &lowCutSlopeSlider, &highCutSlopeSlider, &responseCurveComponent, &midFreqSlider, &midGainSlider, &midQualitySlider
    };
}
