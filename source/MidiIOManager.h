#pragma once
#include <optional>
#include <JuceHeader.h>
#include "MixerState.h"

/*
    Owns the actual MIDI ports. Everything the app sends to the desk, and
    everything it receives, flows through here -- the GUI never touches
    juce::MidiInput/Output directly. This is also the natural place to hang a
    raw-message log callback for the monitor pane, and later, an RS232 backend
    behind the same interface.
*/
class MidiIOManager : private juce::MidiInputCallback
{
public:
    struct RawMessageListener
    {
        virtual ~RawMessageListener() = default;
        virtual void midiMessageLogged (const juce::MidiMessage& message, bool outgoing) {}
    };

    MidiIOManager (MixerState& stateToUpdate);
    ~MidiIOManager() override;

    juce::StringArray getAvailableInputNames() const;
    juce::StringArray getAvailableOutputNames() const;

    bool openInput (const juce::String& deviceName);
    bool openOutput (const juce::String& deviceName);
    void closeInput();
    void closeOutput();
    bool isInputOpen() const  { return midiInput  != nullptr; }
    bool isOutputOpen() const { return midiOutput != nullptr; }

    // ---- Outgoing commands (surface-level control) ----
    void sendChannelVolume (int channelIndex, double db);
    void sendChannelPan (int channelIndex, double pan);
    void sendChannelMute (int channelIndex, bool mute);

    // Device/channel targeting. Default is omni (matches the independently-
    // built aldipower/bitwig-ddx3216-controller script's default). Call
    // setDeviceMidiChannel(n) if you've confirmed via sniffing that your
    // desk needs an explicit channel instead.
    void setOmni() { deviceMidiChannel.reset(); }
    void setDeviceMidiChannel (int channel1to16) { deviceMidiChannel = channel1to16; }
    bool isOmni() const { return ! deviceMidiChannel.has_value(); }
    int getDeviceMidiChannel() const { return deviceMidiChannel.value_or (0); }

    void addRawMessageListener (RawMessageListener* l)    { rawListeners.add (l); }
    void removeRawMessageListener (RawMessageListener* l) { rawListeners.remove (l); }

private:
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;
    void sendFrame (const juce::MidiMessage& frame);

    MixerState& state;
    std::unique_ptr<juce::MidiInput> midiInput;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    juce::ListenerList<RawMessageListener> rawListeners;
    std::optional<int> deviceMidiChannel; // nullopt == omni
};
