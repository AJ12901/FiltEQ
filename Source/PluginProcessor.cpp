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
    // Here we prepare our filters before using them and to do so, we need to pass in a process spec object to the chain which then passes it to each link in the chain

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock; // we pass in these 3 values to our spec object
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    
    leftChannel.prepare(spec);
    rightChannel.prepare(spec);
    
    updateFilters();
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
    
    updateFilters();
        
    // Here we take our audio buffer, split it into channels, wrap it in an ProcessingContext which we can then ask our chains to process
    juce::dsp::AudioBlock<float> block(buffer); // Create Audio Block from buffer
    
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    leftChannel.process(leftContext);
    rightChannel.process(rightContext);
}

//==============================================================================
bool FiltEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FiltEQAudioProcessor::createEditor()
{
    return new FiltEQAudioProcessorEditor (*this);
//    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void FiltEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream state(destData, true);
    apvts.state.writeToStream(state);
}

void FiltEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
        updateFilters();
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FiltEQAudioProcessor();
}

// AudioParameterFloat represents parameters with a continuous knob/slider

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState &apvts) // loads the raw values of our parameters into ChainSettings which is then called later in the processBlock and prepareToPlay functions
{
    ChainSettings settings;
    
    settings.lowCutFreq = apvts.getRawParameterValue("Low Cut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("High Cut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Frequency")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("Low Cut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("High Cut Slope")->load());
    settings.midFreq = apvts.getRawParameterValue("Mid Frequency")->load();
    settings.midGainInDecibels = apvts.getRawParameterValue("Mid Gain")->load();
    settings.midQuality = apvts.getRawParameterValue("Mid Quality")->load();
    
    return settings;
}

Coefficients makePeakFilter(const ChainSettings &chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

Coefficients makeMidFilter(const ChainSettings &chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.midFreq, chainSettings.midQuality, juce::Decibels::decibelsToGain(chainSettings.midGainInDecibels));
}

void FiltEQAudioProcessor::updatePeakFilter (const ChainSettings& chainSettings)
{
//    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
  
    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());
    
    updateCoefficients(leftChannel.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChannel.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

void FiltEQAudioProcessor::updateMidFilter (const ChainSettings& chainSettings)
{
    auto midCoefficients = makeMidFilter(chainSettings, getSampleRate());
    updateCoefficients(leftChannel.get<ChainPositions::Mid>().coefficients, midCoefficients);
    updateCoefficients(rightChannel.get<ChainPositions::Mid>().coefficients, midCoefficients);
}

template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType &chain, const CoefficientType &coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}

template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType &chain, const CoefficientType &coefficients, const Slope &slope)
{
    chain.template setBypassed<0>(true);
    chain.template setBypassed<1>(true);
    chain.template setBypassed<2>(true);
    chain.template setBypassed<3>(true);
    
    switch (slope) {
        case Slope_48:
            update<3>(chain, coefficients);
        case Slope_36:
            update<2>(chain, coefficients);
        case Slope_24:
            update<1>(chain, coefficients);
        case Slope_12:
            update<0>(chain, coefficients);
    }
}

void /*FiltEQAudioProcessor::*/updateCoefficients(Coefficients &old, const Coefficients &replacements)
{
    *old = *replacements;
}

void FiltEQAudioProcessor::updateLowCutFilters(const ChainSettings &chainSettings)
{
    auto cutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
    auto &leftLowCut = leftChannel.get<ChainPositions::LowCut>();
    auto &rightLowCut = rightChannel.get<ChainPositions::LowCut>();
    updateCutFilter(leftLowCut, cutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, cutCoefficients, chainSettings.lowCutSlope);
}

void FiltEQAudioProcessor::updateHighCutFilters(const ChainSettings &chainSettings)
{
    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());
    auto &leftHighCut = leftChannel.get<ChainPositions::HighCut>();
    auto &rightHighCut = rightChannel.get<ChainPositions::HighCut>();
    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void FiltEQAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);
    
    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateMidFilter(chainSettings);
    updateHighCutFilters(chainSettings);
}

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
    
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("Low Cut Freq", "Low Cut Freq", juce::NormalisableRange<float>(20.f, 20000.f, 0.1f, 0.3f), 20.f)); // Low Cut Freq
    pluginLayout.add(std::make_unique<juce::AudioParameterChoice>("Low Cut Slope", "Low Cut Slope", filterCutoffChoices, 0)); // Low Cut Slope
    
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("High Cut Freq", "High Cut Freq", juce::NormalisableRange<float>(20.f, 20000.f, 0.1f, 0.5f), 20000.f)); // High Cut Freq
    pluginLayout.add(std::make_unique<juce::AudioParameterChoice>("High Cut Slope", "High Cut Slope", filterCutoffChoices, 0)); // High Cut Slope
    
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("Peak Frequency", "Peak Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 0.1f, 0.3f), 2000.f)); // Peak Freq
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain", "Peak Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.f)); // Peak Gain
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality", "Peak Quality", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f)); // Peak Quality
    
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("Mid Frequency", "Mid Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 0.1f, 0.3f), 1000.f)); // Mid Freq
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("Mid Gain", "Mid Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.f)); // Mid Gain
    pluginLayout.add(std::make_unique<juce::AudioParameterFloat>("Mid Quality", "Mid Quality", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f)); // Mid Quality
    
    return pluginLayout;
}
