/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <Fifo.h>

//==============================================================================
/**
*/
class AudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

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

    // Enumeration for DSP options
    enum class DSP_OPTION
    {
        Phase,
        Chorus,
        Overdrive,
        LadderFilter,
        GeneralFilter,
        END_OF_LIST
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    using DSP_ORDER = std::array<DSP_OPTION, static_cast<size_t>(DSP_OPTION::END_OF_LIST)>;
    using DSP_POINTERS = std::array<juce::dsp::ProcessorBase*, static_cast<size_t>(DSP_OPTION::END_OF_LIST)>;

    SimpleMBComp::Fifo<DSP_ORDER> dspOrderFifo;

    // Parameters for Phaser
    struct PhaserParams {
        std::atomic<float>* rateHz = nullptr;
        std::atomic<float>* depthPercent = nullptr;
        std::atomic<float>* centerFreqHz = nullptr;
        std::atomic<float>* feedbackPercent = nullptr;
        std::atomic<float>* mixPercent = nullptr;
    };

    PhaserParams phaserParams;

    // Parmeters for Chorus
    struct ChorusParams {
        std::atomic<float>* rateHz = nullptr;
        std::atomic<float>* depthPercent = nullptr;
        std::atomic<float>* centerDelayMs = nullptr;
        std::atomic<float>* feedbackPercent = nullptr;
        std::atomic<float>* mixPercent = nullptr;
    };
    ChorusParams chorusParams;

    // Parameters for Overdrive
    struct OverdriveParams{
        std::atomic<float>* overdriveSaturation = nullptr;
    };
    OverdriveParams overdriveParams;

    // Parameters for Ladder Filter
    struct LadderFilterParams {
        std::atomic<float>* cutoffHz = nullptr;
        std::atomic<float>* resonance = nullptr;
        std::atomic<float>* drive = nullptr;
        std::atomic<int>* mode = nullptr; // Mode is enum-backed, so int works
    };

    LadderFilterParams ladderFilterParams;

    // Parameters for General Filter
    struct GeneralFilterParams {
        std::atomic<int>* mode = nullptr; // Mode is enum-backed, so int works
        std::atomic<float>* freqHz = nullptr;
        std::atomic<float>* quality = nullptr;
        std::atomic<float>* gainDb = nullptr;
    };
    GeneralFilterParams generalFilterParams;

private:

    // DSP chain configuration
    DSP_ORDER dspOrder;
    DSP_POINTERS dspInstances;

    // Template wrapper for DSP modules
    template <typename T>
    struct DSP_CHOICE : juce::dsp::ProcessorBase
    {
        void prepare(const juce::dsp::ProcessSpec& spec) override 
        { 
            dsp.prepare(spec); 
        }
        
        void process(const juce::dsp::ProcessContextReplacing<float>& context) override 
        { 
            dsp.process(context); 
        }
        
        void reset() override 
        { 
            dsp.reset(); 
        }

        T dsp;
    };

    // DSP module instances
    DSP_CHOICE<juce::dsp::Phaser<float>> phaser;
    DSP_CHOICE<juce::dsp::Chorus<float>> chorus;
    DSP_CHOICE<juce::dsp::LadderFilter<float>> overdrive;
    DSP_CHOICE<juce::dsp::LadderFilter<float>> ladderFilter;
    DSP_CHOICE<juce::dsp::IIR::Filter<float>> generalFilter;

    // Processing utilities
    juce::dsp::ProcessSpec spec;
    
    // Configuration method
    void configureDSPModules();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};