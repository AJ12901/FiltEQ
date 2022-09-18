/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
  CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
        
    }
};

//==============================================================================
/**
*/
class FiltEQAudioProcessorEditor  : public juce::AudioProcessorEditor,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
public:
    FiltEQAudioProcessorEditor (FiltEQAudioProcessor&);
    ~FiltEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {};
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FiltEQAudioProcessor& audioProcessor;
    
    juce::Atomic<bool> parametersChanged {false};
    
    CustomRotarySlider peakFreqSlider, peakGainSlider, peakQualitySlider, lowCutFreqSlider, highCutFreqSlider, lowCutSlopeSlider, highCutSlopeSlider;
    
    using APVTS = juce::AudioProcessorValueTreeState; // typename aliases since these names are quite long
    using Attachment = APVTS::SliderAttachment;
    Attachment peakFreqSliderAttachment, peakGainSliderAttachment, peakQualitySliderAttachment, lowCutFreqSliderAttachment, highCutFreqSliderAttachment, lowCutSlopeSliderAttachment, highCutSlopeSliderAttachment;
    
    MonoChain monoChain; // adding a dedicated monochain for the editor
    
    
    
    
    std::vector<juce::Component*> getComps(); // vector that's goint to allow iterating over all our sliders

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FiltEQAudioProcessorEditor)
};
