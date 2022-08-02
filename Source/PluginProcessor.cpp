#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FiltEQAudioProcessor::FiltEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

FiltEQAudioProcessor::~FiltEQAudioProcessor()
{
}

//==============================================================================
const juce::String FiltEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FiltEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FiltEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FiltEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FiltEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FiltEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FiltEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FiltEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FiltEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void FiltEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FiltEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void FiltEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FiltEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FiltEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool FiltEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FiltEQAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void FiltEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void FiltEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FiltEQAudioProcessor();
}

// AudioParameterFloat represents parameters with a continuous knob/slider


// Low Cut Parameters
juce::AudioProcessorValueTreeState::ParameterLayout FiltEQAudioProcessor::parameterLayoutCreation()
{
    juce::AudioProcessorValueTreeState::ParameterLayout pluginLayout; // Overall plugin Layout
    
    juce::StringArray filterCutoffChoices; // String Array Comprising of choices of how steep the filter cutoff is
    for (int i=0; i<4; i++)
    {
        juce::String value;
        value << (12 + i*12);
        value << " db/Oct";
        filterCutoffChoices.add(value);
    }
    
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("Low Cut Freq", "Low Cut Freq", juce::NormalisableRange<float>(20.f, 20000.f, 0.1f, 0.5f), 20.f)); // Low Cut Freq
    pluginLayout.add(std::make_unique<juce::AudioParameterChoice>("Low Cut Slope", "Low Cut Slope", filterCutoffChoices, 0)); // Low Cut Slope
    
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("High Cut Freq", "High Cut Freq", juce::NormalisableRange<float>(20.f, 20000.f, 0.1f, 0.5f), 20000.f)); // High Cut Freq
    pluginLayout.add(std::make_unique<juce::AudioParameterChoice>("High Cut Slope", "High Cut Slope", filterCutoffChoices, 0)); // High Cut Slope
    
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("Peak Frequency", "Peak Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 0.1f, 0.5f), 1000.f)); // Peak Freq
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain", "Peak Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.f)); // Peak Gain
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality", "Peak Quality", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f)); // Peak Quality
    
    
    
    return pluginLayout;
}