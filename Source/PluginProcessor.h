#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class FiltEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    FiltEQAudioProcessor();
    ~FiltEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout parameterLayoutCreation(); // Function that Creates ALL the parameters in the plugin (Its static since it doesnt use any member variables)
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", parameterLayoutCreation()} ; // Object that coordinates syncing of parameters between gui knobs and dsp variables

private:
    using Filter = juce::dsp::IIR::Filter<float>; // type namespace to avoid always having to write out nested namespaces
    
    // The dsp namespace in JUCE works by defining a chain and passing a processing context which will run through each element of the chain automatically
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>; // Chain has 4 filters since the default one is 12db/oct and we need it to go up to 48db/oct
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>; // Represents the layout of our EQ where we have a cut on either end and a parametric filter in the middle
    MonoChain leftChannel, rightChannel;
    
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FiltEQAudioProcessor)
};
