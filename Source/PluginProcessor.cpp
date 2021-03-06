/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#define USE_UI true

//==============================================================================
WebUISynthAudioProcessor::WebUISynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
#endif
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                          ),
      parameterValueTree (*this, nullptr, "PARAMETERS", createParameterLayout()),
      audioEngine (parameterValueTree)
#endif
{
}

WebUISynthAudioProcessor::~WebUISynthAudioProcessor()
{
}

//==============================================================================
const juce::String WebUISynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WebUISynthAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool WebUISynthAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool WebUISynthAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double WebUISynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int WebUISynthAudioProcessor::getNumPrograms()
{
    return 1;// NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
}

int WebUISynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WebUISynthAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String WebUISynthAudioProcessor::getProgramName (int index)
{
    return {};
}

void WebUISynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void WebUISynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    audioEngine.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 2 });

    juce::Timer::callAfterDelay (1000, [this]() {
        DBG (parameterValueTree.state.toXmlString());
    });
}

void WebUISynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool WebUISynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

juce::AudioProcessorValueTreeState::ParameterLayout WebUISynthAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        ParameterIds::osc1Type,
        "Osc 1 Type",
        juce::StringArray { OscTypes::saw, OscTypes::sin },
        0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIds::filterCutoff,
        "Filter Cutoff",
        juce::NormalisableRange<float> (20, 10000),
        1000,
        "Filter Cutoff",
        juce::AudioProcessorParameter::genericParameter,
        [] (float value, int) {
            return juce::String (value) + "hz";
        }));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIds::filterResonance,
        "Filter Resonance",
        0,
        0.9,
        0.3));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIds::ampEnvAttack,
        "Amp Env Attack",
        0.001,
        2,
        0.001));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIds::ampEnvDecay,
        "Amp Env Decay",
        0,
        2,
        0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIds::ampEnvSustain,
        "Amp Env Sustain",
        0,
        1,
        1));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIds::ampEnvRelease,
        "Amp Env Release",
        0.001,
        2,
        0.5));

    //    layout.add (std::make_unique<juce::AudioParameterInt> ("testInt", "Int", 0, 10, 5));
    //    layout.add (std::make_unique<juce::AudioParameterBool> ("testBool", "Bool", false));
    //    juce::StringArray choices = { "choice a", "choice b", "choice c" };
    //    layout.add (std::make_unique<juce::AudioParameterChoice> ("testChoice", "Choice", choices, 0));

    return layout;
}

void WebUISynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
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
    audioEngine.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    scopeDataCollector.process (buffer.getReadPointer (0), (size_t) buffer.getNumSamples());
}

//==============================================================================
bool WebUISynthAudioProcessor::hasEditor() const
{
#if USE_UI
    return true;
#else
    return false;
#endif
}

juce::AudioProcessorEditor* WebUISynthAudioProcessor::createEditor()
{
#if USE_UI
    return new WebUISynthAudioProcessorEditor (*this, parameterValueTree);
#else
    return nullptr;
#endif
}

//==============================================================================
void WebUISynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void WebUISynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

AudioBufferQueue<float>& WebUISynthAudioProcessor::getAudioBufferQueue() noexcept
{
    return audioBufferQueue;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WebUISynthAudioProcessor();
}
