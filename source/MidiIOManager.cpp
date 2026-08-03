#include "MidiIOManager.h"

MidiIOManager::MidiIOManager (MixerState& stateToUpdate) : state (stateToUpdate) {}

MidiIOManager::~MidiIOManager()
{
    closeInput();
    closeOutput();
}

juce::StringArray MidiIOManager::getAvailableInputNames() const
{
    juce::StringArray names;
    for (auto& d : juce::MidiInput::getAvailableDevices())
        names.add (d.name);
    return names;
}

juce::StringArray MidiIOManager::getAvailableOutputNames() const
{
    juce::StringArray names;
    for (auto& d : juce::MidiOutput::getAvailableDevices())
        names.add (d.name);
    return names;
}

bool MidiIOManager::openInput (const juce::String& deviceName)
{
    closeInput();
    for (auto& d : juce::MidiInput::getAvailableDevices())
    {
        if (d.name == deviceName)
        {
            midiInput = juce::MidiInput::openDevice (d.identifier, this);
            if (midiInput != nullptr)
            {
                midiInput->start();
                return true;
            }
        }
    }
    return false;
}

bool MidiIOManager::openOutput (const juce::String& deviceName)
{
    closeOutput();
    for (auto& d : juce::MidiOutput::getAvailableDevices())
    {
        if (d.name == deviceName)
        {
            midiOutput = juce::MidiOutput::openDevice (d.identifier);
            return midiOutput != nullptr;
        }
    }
    return false;
}

void MidiIOManager::closeInput()
{
    if (midiInput != nullptr)
    {
        midiInput->stop();
        midiInput.reset();
    }
}

void MidiIOManager::closeOutput()
{
    midiOutput.reset();
}

void MidiIOManager::sendChannelVolume (int channelIndex, double db)
{
    auto raw = DDX3216::volumeDbToRaw (db);
    auto frame = DDX3216::buildSingleParamChange ((uint8_t) (DDX3216::Module::kChannelBase + channelIndex),
                                                   DDX3216::Param::kVolume, raw);
    sendFrame (frame);
    state.setChannelVolumeDb (channelIndex, db);
}

void MidiIOManager::sendChannelPan (int channelIndex, double pan)
{
    auto raw = DDX3216::panPositionToRaw (pan);
    auto frame = DDX3216::buildSingleParamChange ((uint8_t) (DDX3216::Module::kChannelBase + channelIndex),
                                                   DDX3216::Param::kPan, raw);
    sendFrame (frame);
    state.setChannelPan (channelIndex, pan);
}

void MidiIOManager::sendChannelMute (int channelIndex, bool mute)
{
    auto frame = DDX3216::buildSingleParamChange ((uint8_t) (DDX3216::Module::kChannelBase + channelIndex),
                                                   DDX3216::Param::kMute, mute ? 1 : 0);
    sendFrame (frame);
    state.setChannelMute (channelIndex, mute);
}

void MidiIOManager::sendFrame (const juce::MidiMessage& frame)
{
    if (midiOutput != nullptr)
        midiOutput->sendMessageNow (frame);

    rawListeners.call ([&frame] (RawMessageListener& l) { l.midiMessageLogged (frame, true); });
}

void MidiIOManager::handleIncomingMidiMessage (juce::MidiInput* /*source*/, const juce::MidiMessage& message)
{
    rawListeners.call ([&message] (RawMessageListener& l) { l.midiMessageLogged (message, false); });

    if (! message.isSysEx())
        return; // CC handling can be added here once the CC map is verified against real hardware

    auto changes = DDX3216::parseIncomingSysEx (message);
    for (auto& c : changes)
    {
        auto moduleAsInt = (int) c.module;

        if (moduleAsInt >= DDX3216::Module::kChannelBase &&
            moduleAsInt <  DDX3216::Module::kChannelBase + DDX3216::Module::kChannelCount)
        {
            auto channelIndex = moduleAsInt - DDX3216::Module::kChannelBase;

            if (c.param == DDX3216::Param::kVolume)
                state.setChannelVolumeDb (channelIndex, DDX3216::volumeRawToDb (c.rawValue));
            else if (c.param == DDX3216::Param::kPan)
                state.setChannelPan (channelIndex, DDX3216::panRawToPosition (c.rawValue));
            else if (c.param == DDX3216::Param::kMute)
                state.setChannelMute (channelIndex, c.rawValue != 0);
        }
        // Bus/aux/FX-master/FX-return module ranges are INFERRED (see
        // DDX3216Protocol.h) -- deliberately not wired into state updates yet
        // until confirmed against real hardware traffic.
    }
}
