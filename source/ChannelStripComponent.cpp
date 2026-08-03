#include "ChannelStripComponent.h"

ChannelStripComponent::ChannelStripComponent (int channelIndexToShow, MixerState& stateToWatch, MidiIOManager& ioToUse)
    : channelIndex (channelIndexToShow), state (stateToWatch), io (ioToUse)
{
    channelLabel.setText (juce::String (channelIndex + 1), juce::dontSendNotification);
    channelLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (channelLabel);

    fader.setRange (-80.0, 12.0, 0.1);
    fader.setTextValueSuffix (" dB");
    fader.onValueChange = [this]
    {
        if (updatingFromState) return;
        io.sendChannelVolume (channelIndex, fader.getValue());
    };
    addAndMakeVisible (fader);

    panKnob.setRange (-30.0, 30.0, 1.0);
    panKnob.onValueChange = [this]
    {
        if (updatingFromState) return;
        io.sendChannelPan (channelIndex, panKnob.getValue());
    };
    addAndMakeVisible (panKnob);

    muteButton.setClickingTogglesState (true);
    muteButton.onClick = [this]
    {
        if (updatingFromState) return;
        io.sendChannelMute (channelIndex, muteButton.getToggleState());
    };
    addAndMakeVisible (muteButton);

    state.addListener (this);
    refreshFromState();
}

ChannelStripComponent::~ChannelStripComponent()
{
    state.removeListener (this);
}

void ChannelStripComponent::resized()
{
    auto area = getLocalBounds().reduced (2);
    channelLabel.setBounds (area.removeFromTop (20));
    muteButton.setBounds (area.removeFromBottom (24).reduced (8, 0));
    panKnob.setBounds (area.removeFromBottom (70));
    fader.setBounds (area);
}

void ChannelStripComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::darkgrey.darker (0.4f));
    g.fillRect (getLocalBounds());
}

void ChannelStripComponent::channelChanged (int changedIndex)
{
    if (changedIndex == channelIndex)
        refreshFromState();
}

void ChannelStripComponent::refreshFromState()
{
    updatingFromState = true;
    auto& ch = state.channels[(size_t) channelIndex];
    fader.setValue (ch.volumeDb, juce::dontSendNotification);
    panKnob.setValue (ch.pan, juce::dontSendNotification);
    muteButton.setToggleState (ch.mute, juce::dontSendNotification);
    updatingFromState = false;
}
