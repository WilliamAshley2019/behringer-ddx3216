#pragma once
#include <JuceHeader.h>
#include "MixerState.h"
#include "MidiIOManager.h"
#include "SerialPortManager.h"
#include "ChannelStripComponent.h"

class MainComponent : public juce::Component,
                       private MidiIOManager::RawMessageListener,
                       private SerialPortManager::RawMessageListener,
                       private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;

private:
    void midiMessageLogged (const juce::MidiMessage& message, bool outgoing) override;
    void serialMessageLogged (const juce::MidiMessage& message, bool outgoing) override;
    void serialStatusChanged (const juce::String& status) override;
    void timerCallback() override; // periodically refreshes port lists
    void refreshPortLists();
    void logLine (const juce::String& line);

    MixerState state;
    MidiIOManager io { state };
    SerialPortManager serial;

    juce::ComboBox inputSelector, outputSelector, midiChannelSelector, serialPortSelector;
    juce::Label inputLabel { {}, "MIDI In:" }, outputLabel { {}, "MIDI Out:" },
                midiChannelLabel { {}, "Ch:" }, serialLabel { {}, "RS232:" };
    juce::TextButton serialConnectButton { "Connect" };

    juce::Viewport channelsViewport;
    juce::Component channelsHolder;
    juce::OwnedArray<ChannelStripComponent> channelStrips;

    juce::TextEditor logBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
