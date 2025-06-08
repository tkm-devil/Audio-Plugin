/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.
  setSize(400, 300);

  addAndMakeVisible(dspOrderLabel);
  dspOrderLabel.setText("DSP Chain: ", juce::dontSendNotification);
  dspOrderLabel.setJustificationType(juce::Justification::centredTop);

  addAndMakeVisible(refreshOrderButton);
  refreshOrderButton.onClick = [this]()
  {
    juce::String chain;
    for (auto option : audioProcessor.getDSPOrder())
    {
      chain += juce::String(static_cast<int>(option)) + " ";
    }
    dspOrderLabel.setText("DSP Chain: " + chain.trim(), juce::dontSendNotification);
  };
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g)
{
  // (Our component is opaque, so we must completely fill the background with a solid colour)
  g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  g.setColour(juce::Colours::white);
  g.setFont(juce::FontOptions(15.0f));
  g.drawFittedText("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void AudioPluginAudioProcessorEditor::resized()
{
  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..
  auto bounds = getLocalBounds().reduced(20);
  dspOrderLabel.setBounds(bounds.removeFromTop(30));
  refreshOrderButton.setBounds(bounds.removeFromTop(30));
}
