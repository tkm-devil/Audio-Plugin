/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JucePluginDefines.h>
#include <juce_dsp/juce_dsp.h>

// getters for Phaser parameters
auto getPhaserRateName() { return juce::String("Phaser Rate Hz"); }
auto getPhaserDepthName() { return juce::String("Phaser Depth %"); }
auto getPhaserCentreFreqName() { return juce::String("Phaser CentreFreq Hz"); }
auto getPhaserFeedbackName() { return juce::String("Phaser Feedback %"); }
auto getPhaserMixName() { return juce::String("Phaser Mix %"); }
auto getPhaserBypassName() { return juce::String("Phaser Bypass"); }

// getters for Chorus parameters
auto getChorusRateName() { return juce::String("Chorus Rate Hz"); }
auto getChorusDepthName() { return juce::String("Chorus Depth %"); }
auto getChorusCentreDelayName() { return juce::String("Chorus CentreDelay Ms"); }
auto getChorusFeedbackName() { return juce::String("Chorus Feedback %"); }
auto getChorusMixName() { return juce::String("Chorus Mix %"); }
auto getChorusBypassName() { return juce::String("Chorus Bypass"); }

// getters for WaveShaper parameters
auto getWaveShaperSaturationName() { return juce::String("WaveShaper Saturation"); }
auto getWaveShaperBypassName() { return juce::String("WaveShaper Bypass"); }

// getters for Ladder Filter parameters
auto getLadderFilterCutoffName() { return juce::String("Ladder Filter Cutoff Hz"); }
auto getLadderFilterResonanceName() { return juce::String("Ladder Filter Resonance"); }
auto getLadderFilterDriveName() { return juce::String("Ladder Filter Drive"); }
auto getLadderFilterModeName() { return juce::String("Ladder Filter Mode"); }
auto getLadderFilterBypassName() { return juce::String("Ladder Filter Bypass"); }

// getters for General Filter parameters
auto getGeneralFilterChoices()
{
    return juce::StringArray{"Peak", "Low Pass", "High Pass", "Band Pass", "Notch", "All Pass"};
}
auto getGeneralFilterModeName() { return juce::String("General Filter Mode"); }
auto getGeneralFilterFreqName() { return juce::String("General Filter Frequency Hz"); }
auto getGeneralFilterQualityName() { return juce::String("General Filter Quality"); }
auto getGeneralFilterGainName() { return juce::String("General Filter Gain dB"); }
auto getGeneralFilterBypassName() { return juce::String("General Filter Bypass"); }

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::mono(), true)
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::mono(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         ),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
#else
    : apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // Initialize DSP order and map instances
    dspOrder = {DSP_OPTION::Phase, DSP_OPTION::Chorus, DSP_OPTION::WaveShaper, DSP_OPTION::LadderFilter,
                DSP_OPTION::GeneralFilter};
    dspInstances = {&phaser, &chorus, &waveShaper, &ladderFilter, &generalFilter};

    // Set up Phaser parameters
    phaserParams.rateHz = apvts.getRawParameterValue(getPhaserRateName());
    phaserParams.depthPercent = apvts.getRawParameterValue(getPhaserDepthName());
    phaserParams.centerFreqHz = apvts.getRawParameterValue(getPhaserCentreFreqName());
    phaserParams.feedbackPercent = apvts.getRawParameterValue(getPhaserFeedbackName());
    phaserParams.mixPercent = apvts.getRawParameterValue(getPhaserMixName());
    phaserParams.bypass = reinterpret_cast<std::atomic<bool> *>(apvts.getRawParameterValue(getPhaserBypassName()));
    jassert(phaserParams.rateHz && phaserParams.depthPercent && phaserParams.centerFreqHz &&
            phaserParams.feedbackPercent && phaserParams.mixPercent && phaserParams.bypass);

    // Set up Chorus parameters
    chorusParams.rateHz = apvts.getRawParameterValue(getChorusRateName());
    chorusParams.depthPercent = apvts.getRawParameterValue(getChorusDepthName());
    chorusParams.centerDelayMs = apvts.getRawParameterValue(getChorusCentreDelayName());
    chorusParams.feedbackPercent = apvts.getRawParameterValue(getChorusFeedbackName());
    chorusParams.mixPercent = apvts.getRawParameterValue(getChorusMixName());
    chorusParams.bypass = reinterpret_cast<std::atomic<bool> *>(apvts.getRawParameterValue(getChorusBypassName()));
    jassert(chorusParams.rateHz && chorusParams.depthPercent && chorusParams.centerDelayMs &&
            chorusParams.feedbackPercent && chorusParams.mixPercent && chorusParams.bypass);

    // Set up WaveShaper parameters
    waveShaperParams.saturation = apvts.getRawParameterValue(getWaveShaperSaturationName());
    waveShaperParams.bypass = reinterpret_cast<std::atomic<bool> *>(apvts.getRawParameterValue(getWaveShaperBypassName()));
    jassert(waveShaperParams.saturation && waveShaperParams.bypass);

    // Set up Ladder Filter parameters
    ladderFilterParams.cutoffHz = apvts.getRawParameterValue(getLadderFilterCutoffName());
    ladderFilterParams.resonance = apvts.getRawParameterValue(getLadderFilterResonanceName());
    ladderFilterParams.drive = apvts.getRawParameterValue(getLadderFilterDriveName());
    ladderFilterParams.mode = reinterpret_cast<std::atomic<int> *>(apvts.getRawParameterValue(getLadderFilterModeName()));
    ladderFilterParams.bypass = reinterpret_cast<std::atomic<bool> *>(apvts.getRawParameterValue(getLadderFilterBypassName()));
    jassert(ladderFilterParams.cutoffHz && ladderFilterParams.resonance &&
            ladderFilterParams.drive && ladderFilterParams.mode && ladderFilterParams.bypass);

    // Set up General Filter parameters
    generalFilterParams.mode = reinterpret_cast<std::atomic<int> *>(apvts.getRawParameterValue(getGeneralFilterModeName()));
    generalFilterParams.freqHz = apvts.getRawParameterValue(getGeneralFilterFreqName());
    generalFilterParams.quality = apvts.getRawParameterValue(getGeneralFilterQualityName());
    generalFilterParams.gainDb = apvts.getRawParameterValue(getGeneralFilterGainName());
    generalFilterParams.bypass = reinterpret_cast<std::atomic<bool> *>(apvts.getRawParameterValue(getGeneralFilterBypassName()));
    jassert(generalFilterParams.mode && generalFilterParams.freqHz &&
            generalFilterParams.quality && generalFilterParams.gainDb && generalFilterParams.bypass);
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

void AudioPluginAudioProcessor::configurePhaser()
{
    phaser.dsp.setRate(*phaserParams.rateHz);
    phaser.dsp.setDepth(*phaserParams.depthPercent);
    phaser.dsp.setCentreFrequency(*phaserParams.centerFreqHz);
    phaser.dsp.setFeedback(*phaserParams.feedbackPercent);
    phaser.dsp.setMix(*phaserParams.mixPercent);
}

void AudioPluginAudioProcessor::configureChorus()
{
    chorus.dsp.setRate(*chorusParams.rateHz);
    chorus.dsp.setDepth(*chorusParams.depthPercent);
    chorus.dsp.setCentreDelay(*chorusParams.centerDelayMs);
    chorus.dsp.setFeedback(*chorusParams.feedbackPercent);
    chorus.dsp.setMix(*chorusParams.mixPercent);
}

void AudioPluginAudioProcessor::configureWaveShaper()
{
    const float saturationValue = *waveShaperParams.saturation;
    // Scale/normalize the saturation value as needed
    const float drive = juce::jlimit(1.0f, 20.0f, saturationValue * 0.2f); // Adjust curve if needed

    // Set the shaping function based on drive
    waveShaper.dsp.functionToUse = [drive](float x)
    {
        return std::tanh(drive * x);
    };
}

void AudioPluginAudioProcessor::configureLadderFilter()
{
    ladderFilter.dsp.setCutoffFrequencyHz(*ladderFilterParams.cutoffHz);
    ladderFilter.dsp.setResonance(*ladderFilterParams.resonance);
    ladderFilter.dsp.setDrive(*ladderFilterParams.drive);
    ladderFilter.dsp.setMode(static_cast<juce::dsp::LadderFilter<float>::Mode>(ladderFilterParams.mode->load()));
}

void AudioPluginAudioProcessor::configureGeneralFilter()
{
    auto sampleRate = getSampleRate();
    int mode = generalFilterParams.mode->load();
    float freq = generalFilterParams.freqHz->load();
    float Q = generalFilterParams.quality->load();
    float gainDb = generalFilterParams.gainDb->load();
    float gainLinear = juce::Decibels::decibelsToGain(gainDb);

    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    switch (mode)
    {
    case 0: // Peak
        generalFilter.dsp.coefficients = Coefficients::makePeakFilter(sampleRate, freq, Q, gainLinear);
        break;
    case 1: // Low Pass
        generalFilter.dsp.coefficients = Coefficients::makeLowPass(sampleRate, freq, Q);
        break;
    case 2: // High Pass
        generalFilter.dsp.coefficients = Coefficients::makeHighPass(sampleRate, freq, Q);
        break;
    case 3: // Band Pass
        generalFilter.dsp.coefficients = Coefficients::makeBandPass(sampleRate, freq, Q);
        break;
    case 4: // Notch
        generalFilter.dsp.coefficients = Coefficients::makeNotch(sampleRate, freq, Q);
        break;
    case 5: // All Pass
        generalFilter.dsp.coefficients = Coefficients::makeAllPass(sampleRate, freq, Q);
        break;
    default: // fallback
        generalFilter.dsp.coefficients = Coefficients::makePeakFilter(sampleRate, freq, Q, gainLinear);
        break;
    }
}

void AudioPluginAudioProcessor::configureDSPModules()
{
    // Configure each DSP module with its parameters
    configurePhaser();
    configureChorus();
    configureWaveShaper();
    configureLadderFilter();
    configureGeneralFilter();

    // Notify that the DSP order has changed
    dspOrderFifo.push(dspOrder);
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

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const int versionHint = 1;
    // Add parameters for Phaser
    // Phaser Rate
    auto phaserRateName = getPhaserRateName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(phaserRateName, versionHint),
        phaserRateName,
        juce::NormalisableRange<float>(0.1f, 10.f, 0.01f, 1.0f),
        0.5f,
        "Hz"));
    // Phaser Depth
    auto phaserDepthName = getPhaserDepthName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(phaserDepthName, versionHint),
        phaserDepthName,
        juce::NormalisableRange<float>(0.f, 1.0f, 0.01f, 1.f),
        0.25f,
        "%"));
    // Phaser Centre Frequency
    auto phaserFreqName = getPhaserCentreFreqName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(phaserFreqName, versionHint),
        phaserFreqName,
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
        1000.f,
        "Hz"));
    // Phaser Feedback
    auto phaserFeedbackName = getPhaserFeedbackName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(phaserFeedbackName, versionHint),
        phaserFeedbackName,
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f, 1.f),
        0.0f,
        "%"));
    // Phaser Mix
    auto phaserMixName = getPhaserMixName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(phaserMixName, versionHint),
        phaserMixName,
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.f),
        0.3f,
        "%"));
    // Phaser Bypass
    auto phaserBypassName = getPhaserBypassName();
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(phaserBypassName, versionHint),
        phaserBypassName,
        false)); // Default to not bypassed

    // Add parameters for Chorus
    // Chorus Rate
    auto chorusRateName = getChorusRateName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(chorusRateName, versionHint),
        chorusRateName,
        juce::NormalisableRange<float>(0.1f, 5.f, 0.01f, 1.0f),
        0.8f,
        "Hz"));
    // Chorus Depth
    auto chorusDepthName = getChorusDepthName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(chorusDepthName, versionHint),
        chorusDepthName,
        juce::NormalisableRange<float>(0.f, 1.0f, 0.01f, 1.f),
        0.2f,
        "%"));
    // Chorus Centre Delay
    auto chorusCentreDelayName = getChorusCentreDelayName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(chorusCentreDelayName, versionHint),
        chorusCentreDelayName,
        juce::NormalisableRange<float>(0.1f, 100.f, 0.1f, 1.f),
        7.0f,
        "ms"));
    // Chorus Feedback
    auto chorusFeedbackName = getChorusFeedbackName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(chorusFeedbackName, versionHint),
        chorusFeedbackName,
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f, 1.f),
        0.0f,
        "%"));
    // Chorus Mix
    auto chorusMixName = getChorusMixName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(chorusMixName, versionHint),
        chorusMixName,
        juce::NormalisableRange<float>(0.f, 1.0f, 0.01f, 1.f),
        0.4f,
        "%"));
    // Chorus Bypass
    auto chorusBypassName = getChorusBypassName();
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(chorusBypassName, versionHint),
        chorusBypassName,
        false)); // Default to not bypassed

    // Add parameters for WaveShaper
    auto waveShaperSaturationName = getWaveShaperSaturationName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(waveShaperSaturationName, versionHint),
        waveShaperSaturationName,
        juce::NormalisableRange<float>(1.f, 100.0f, 0.1f, 1.f),
        1.f,
        ""));
    // WaveShaper Bypass
    auto waveShaperBypassName = getWaveShaperBypassName();
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(waveShaperBypassName, versionHint),
        waveShaperBypassName,
        false)); // Default to not bypassed

    // Add parameters for Ladder Filter
    // Ladder Filter Cutoff
    auto ladderFilterCutoffName = getLadderFilterCutoffName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ladderFilterCutoffName, versionHint),
        ladderFilterCutoffName,
        juce::NormalisableRange<float>(20.f, 20000.f, 0.1f, 1.f),
        8000.f,
        "Hz"));
    // Ladder Filter Resonance
    auto ladderFilterResonanceName = getLadderFilterResonanceName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ladderFilterResonanceName, versionHint),
        ladderFilterResonanceName,
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f, 1.f),
        0.1f,
        ""));
    // Ladder Filter Drive
    auto ladderFilterDriveName = getLadderFilterDriveName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ladderFilterDriveName, versionHint),
        ladderFilterDriveName,
        juce::NormalisableRange<float>(1.f, 100.f, 0.1f, 1.f),
        1.f,
        ""));
    // Ladder Filter Mode
    auto ladderFilterModeName = getLadderFilterModeName();
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ladderFilterModeName, versionHint),
        ladderFilterModeName,
        juce::StringArray{"LPF12", "HPF12", "BPF12", "LPF24", "HPF24", "BPF24"},
        0)); // Default to LPF12
    // Ladder Filter Bypass
    auto ladderFilterBypassName = getLadderFilterBypassName();
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ladderFilterBypassName, versionHint),
        ladderFilterBypassName,
        false)); // Default to not bypassed

    // General Filter: https://docs.juce.com/develop/structdsp_1_1IIR_1_1Coefficients.html
    // Add parameters for General Filter
    auto generalFilterModeName = getGeneralFilterModeName();
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(generalFilterModeName, versionHint),
        generalFilterModeName,
        getGeneralFilterChoices(),
        0)); // Default to Peak

    auto generalFilterFreqName = getGeneralFilterFreqName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(generalFilterFreqName, versionHint),
        generalFilterFreqName,
        juce::NormalisableRange<float>(20.f, 20000.f, 0.1f, 1.f),
        1000.f,
        "Hz"));

    auto generalFilterQualityName = getGeneralFilterQualityName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(generalFilterQualityName, versionHint),
        generalFilterQualityName,
        juce::NormalisableRange<float>(0.1f, 10.f, 0.01f, 1.f),
        1.f,
        ""));

    auto generalFilterGainName = getGeneralFilterGainName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(generalFilterGainName, versionHint),
        generalFilterGainName,
        juce::NormalisableRange<float>(-24.f, 24.f, 0.1f, 1.f),
        0.f,
        "dB"));
    // General Filter Bypass
    auto generalFilterBypassName = getGeneralFilterBypassName();
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(generalFilterBypassName, versionHint),
        generalFilterBypassName,
        false)); // Default to not bypassed

    return layout;
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that didn't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Check if a new DSP order is available
    DSP_ORDER newOrder;
    if (dspOrderFifo.pull(newOrder))
    {
        dspOrder = newOrder; // Replace the current DSP order
    }

    // Create audio block and processing context (create them locally)
    auto audioBlock = juce::dsp::AudioBlock<float>(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(audioBlock);

    configureDSPModules(); // Ensure DSP modules are configured with current parameters

    // Process through DSP chain in specified order
    for (size_t i = 0; i < dspOrder.size(); ++i)
    {
        auto effectType = dspOrder[i];
        auto effectIndex = static_cast<size_t>(effectType);

        if (effectIndex < dspInstances.size() && dspInstances[effectIndex] != nullptr)
        {
            // Check bypass flags before processing
            bool bypass = false;

            switch (effectType)
            {
            case DSP_OPTION::Phase:
                bypass = phaserParams.bypass && phaserParams.bypass->load();
                break;
            case DSP_OPTION::Chorus:
                bypass = chorusParams.bypass && chorusParams.bypass->load();
                break;
            case DSP_OPTION::WaveShaper:
                bypass = waveShaperParams.bypass && waveShaperParams.bypass->load();
                break;
            case DSP_OPTION::LadderFilter:
                bypass = ladderFilterParams.bypass && ladderFilterParams.bypass->load();
                break;
            case DSP_OPTION::GeneralFilter:
                bypass = generalFilterParams.bypass && generalFilterParams.bypass->load();
                break;
            default:
                break;
            }

            if (!bypass)
            {
                dspInstances[effectIndex]->process(context);
            }
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
    // return new AudioPluginAudioProcessorEditor(*this);
    return new juce::GenericAudioProcessorEditor(*this); // Use GenericAudioProcessorEditor for simplicity
}

template <>
struct juce::VariantConverter<AudioPluginAudioProcessor::DSP_ORDER>
{
    static AudioPluginAudioProcessor::DSP_ORDER fromVar(const juce::var &v)
    {
        AudioPluginAudioProcessor::DSP_ORDER order;
        juce::String orderString = v.toString();

        if (orderString.isNotEmpty())
        {
            juce::StringArray tokens = juce::StringArray::fromTokens(orderString, ",", "");

            for (size_t i = 0; i < order.size() && i < static_cast<size_t>(tokens.size()); ++i)
            {
                int enumValue = tokens[static_cast<int>(i)].getIntValue();

                if (enumValue >= 0 && enumValue < static_cast<int>(AudioPluginAudioProcessor::DSP_OPTION::END_OF_LIST))
                {
                    order[i] = static_cast<AudioPluginAudioProcessor::DSP_OPTION>(enumValue);
                }
                else
                {
                    order[i] = static_cast<AudioPluginAudioProcessor::DSP_OPTION>(i);
                }
            }
        }
        else
        {
            // Default order
            for (size_t i = 0; i < order.size(); ++i)
            {
                order[i] = static_cast<AudioPluginAudioProcessor::DSP_OPTION>(i);
            }
        }

        return order;
    }

    static juce::var toVar(const AudioPluginAudioProcessor::DSP_ORDER &order)
    {
        juce::String result;

        for (size_t i = 0; i < order.size(); ++i)
        {
            if (i > 0)
                result += ",";
            result += juce::String(static_cast<int>(order[i]));
        }

        return result;
    }
};

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // Copy the current state from the APVTS
    auto state = apvts.copyState();

    // Serialize DSP order using VariantConverter
    auto dspOrderVar = juce::VariantConverter<DSP_ORDER>::toVar(dspOrder);
    state.setProperty("dspOrder", dspOrderVar, nullptr);

    // Convert to XML and store it
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // Optionally write the XML to a file for debugging purposes
    // xml->writeToFile(juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
    //              .getChildFile("PluginStateDump.xml"), {});

    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // Load the XML from the binary block
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        // Convert XML back into a ValueTree
        juce::ValueTree tree = juce::ValueTree::fromXml(*xmlState);

        if (tree.isValid())
        {
            // Extract DSP order if present
            if (tree.hasProperty("dspOrder"))
            {
                DSP_ORDER restoredOrder = juce::VariantConverter<DSP_ORDER>::fromVar(tree.getProperty("dspOrder"));
                dspOrderFifo.push(restoredOrder); // Apply restored DSP order
            }

            // Apply the parameter state
            apvts.replaceState(tree);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}