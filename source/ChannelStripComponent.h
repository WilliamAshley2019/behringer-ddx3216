#pragma once
#include <JuceHeader.h>
#include "MixerState.h"
#include "MidiIOManager.h"

class ChannelStripComponent : public juce::Component,
                               private MixerState::Listener
{
public:
    ChannelStripComponent (int channelIndexToShow, MixerState& stateToWatch, MidiIOManager& ioToUse);
    ~ChannelStripComponent() override;

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    void channelChanged (int channelIndex) override;
    void refreshFromState();

    int channelIndex;
    MixerState& state;
    MidiIOManager& io;

    juce::Slider fader   { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
    juce::Slider panKnob { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::TextButton muteButton { "M" };
    juce::Label channelLabel;

    bool updatingFromState = false; // guards against feedback loops (state->GUI->send->state...)
};
