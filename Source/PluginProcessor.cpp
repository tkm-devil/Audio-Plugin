/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JucePluginDefines.h>

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      )
#endif
{
    // Initialize DSP order and map instances
    dspOrder = {DSP_OPTION::Phase, DSP_OPTION::Chorus, DSP_OPTION::Overdrive, DSP_OPTION::LadderFilter, DSP_OPTION::Delay};
    dspInstances = {&phaser, &chorus, &overdrive, &ladderFilter, &delay};
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Set up process spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumInputChannels());

    // Prepare all DSP modules
    for (auto *dsp : dspInstances)
    {
        if (dsp != nullptr)
            dsp->prepare(spec);
    }

    // Configure individual DSP modules with default parameters
    configureDSPModules();
}

void AudioPluginAudioProcessor::releaseResources()
{
    // Reset all DSP modules
    for (auto *dsp : dspInstances)
    {
        if (dsp != nullptr)
            dsp->reset();
    }
}

void AudioPluginAudioProcessor::configureDSPModules()
{
    // Configure Phaser
    phaser.dsp.setRate(1.0f);
    phaser.dsp.setDepth(1.0f);
    phaser.dsp.setCentreFrequency(1300.0f);
    phaser.dsp.setFeedback(0.7f);
    phaser.dsp.setMix(0.5f);

    // Configure Chorus
    chorus.dsp.setRate(1.5f);
    chorus.dsp.setDepth(0.25f);
    chorus.dsp.setCentreDelay(7.0f);
    chorus.dsp.setFeedback(0.0f);
    chorus.dsp.setMix(0.5f);

    // Configure Overdrive (using LadderFilter as distortion)
    overdrive.dsp.setMode(juce::dsp::LadderFilterMode::LPF24);
    overdrive.dsp.setCutoffFrequencyHz(2000.0f);
    overdrive.dsp.setResonance(0.7f);
    overdrive.dsp.setDrive(5.0f);

    // Configure LadderFilter
    ladderFilter.dsp.setMode(juce::dsp::LadderFilterMode::LPF24);
    ladderFilter.dsp.setCutoffFrequencyHz(1000.0f);
    ladderFilter.dsp.setResonance(0.5f);
    ladderFilter.dsp.setDrive(1.0f);

    // Configure Delay
    delay.dsp.setMaximumDelayInSamples(static_cast<int>(spec.sampleRate * 2.0)); // 2 seconds max
    delay.dsp.setDelay(static_cast<float>(spec.sampleRate * 0.25f)); // 250ms delay
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that didn't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Create audio block and processing context (create them locally)
    auto audioBlock = juce::dsp::AudioBlock<float>(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(audioBlock);

    // Process through DSP chain in specified order
    for (size_t i = 0; i < dspOrder.size(); ++i)
    {
        auto effectType = dspOrder[i];
        auto effectIndex = static_cast<size_t>(effectType);

        if (effectIndex < dspInstances.size() && dspInstances[effectIndex] != nullptr)
        {
            dspInstances[effectIndex]->process(context);
        }
    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused(destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused(data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}