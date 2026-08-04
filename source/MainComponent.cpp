#include "MainComponent.h"

static constexpr int kStripWidth = 90;

MainComponent::MainComponent()
{
    addAndMakeVisible (inputLabel);
    addAndMakeVisible (outputLabel);
    addAndMakeVisible (inputSelector);
    addAndMakeVisible (outputSelector);
    addAndMakeVisible (midiChannelLabel);
    addAndMakeVisible (midiChannelSelector);
    addAndMakeVisible (serialLabel);
    addAndMakeVisible (serialPortSelector);
    addAndMakeVisible (serialConnectButton);

    inputSelector.onChange = [this]
    {
        auto name = inputSelector.getText();
        if (io.openInput (name))
            logLine ("Opened MIDI input: " + name);
        else
            logLine ("Failed to open MIDI input: " + name);
    };

    outputSelector.onChange = [this]
    {
        auto name = outputSelector.getText();
        if (io.openOutput (name))
            logLine ("Opened MIDI output: " + name);
        else
            logLine ("Failed to open MIDI output: " + name);
    };

    // "Omni" first (matches the default in aldipower/bitwig-ddx3216-controller),
    // then explicit channels 1-16 in case your desk needs one (see conversation
    // re: sniffed "ic" byte == channel 2 on this user's unit).
    midiChannelSelector.addItem ("Omni", 1);
    for (int ch = 1; ch <= 16; ++ch)
        midiChannelSelector.addItem ("Ch " + juce::String (ch), ch + 1);
    midiChannelSelector.setSelectedId (1, juce::dontSendNotification);
    midiChannelSelector.onChange = [this]
    {
        auto id = midiChannelSelector.getSelectedId();
        if (id <= 1)
        {
            io.setOmni();
            logLine ("MIDI device byte: omni");
        }
        else
        {
            io.setDeviceMidiChannel (id - 1);
            logLine ("MIDI device byte: channel " + juce::String (id - 1));
        }
    };

    serialConnectButton.onClick = [this]
    {
        if (serial.isOpen())
        {
            serial.close();
            serialConnectButton.setButtonText ("Connect");
            return;
        }

        auto portName = serialPortSelector.getText().upToFirstOccurrenceOf (" ", false, false);
        if (portName.isNotEmpty() && serial.open (portName))
            serialConnectButton.setButtonText ("Disconnect");
    };

    for (int i = 0; i < MixerState::kChannelCount; ++i)
    {
        auto* strip = new ChannelStripComponent (i, state, io);
        channelStrips.add (strip);
        channelsHolder.addAndMakeVisible (strip);
    }
    channelsHolder.setSize (kStripWidth * MixerState::kChannelCount, 400);
    channelsViewport.setViewedComponent (&channelsHolder, false);
    addAndMakeVisible (channelsViewport);

    logBox.setMultiLine (true);
    logBox.setReadOnly (true);
    logBox.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
    addAndMakeVisible (logBox);

    io.addRawMessageListener (this);
    serial.addRawMessageListener (this);
    refreshPortLists();
    startTimer (2000); // re-scan for newly connected MIDI/serial devices every 2s

    setSize (1000, 650);
}

MainComponent::~MainComponent()
{
    io.removeRawMessageListener (this);
    serial.removeRawMessageListener (this);
    stopTimer();
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (8);

    auto topRow = area.removeFromTop (28);
    inputLabel.setBounds (topRow.removeFromLeft (55));
    inputSelector.setBounds (topRow.removeFromLeft (180));
    topRow.removeFromLeft (10);
    outputLabel.setBounds (topRow.removeFromLeft (65));
    outputSelector.setBounds (topRow.removeFromLeft (180));
    topRow.removeFromLeft (10);
    midiChannelLabel.setBounds (topRow.removeFromLeft (30));
    midiChannelSelector.setBounds (topRow.removeFromLeft (90));

    auto serialRow = area.removeFromTop (28);
    serialLabel.setBounds (serialRow.removeFromLeft (55));
    serialPortSelector.setBounds (serialRow.removeFromLeft (150));
    serialRow.removeFromLeft (10);
    serialConnectButton.setBounds (serialRow.removeFromLeft (100));

    area.removeFromTop (8);

    auto logArea = area.removeFromBottom (160);
    logBox.setBounds (logArea);

    area.removeFromBottom (8);
    channelsViewport.setBounds (area);

    int y = 0;
    for (int i = 0; i < channelStrips.size(); ++i)
        channelStrips[i]->setBounds (i * kStripWidth, y, kStripWidth, channelsHolder.getHeight());
}

void MainComponent::refreshPortLists()
{
    auto ins = io.getAvailableInputNames();
    auto outs = io.getAvailableOutputNames();

    if (inputSelector.getNumItems() != ins.size())
    {
        inputSelector.clear (juce::dontSendNotification);
        inputSelector.addItemList (ins, 1);
    }
    if (outputSelector.getNumItems() != outs.size())
    {
        outputSelector.clear (juce::dontSendNotification);
        outputSelector.addItemList (outs, 1);
    }

    if (! serial.isOpen())
    {
        auto ports = SerialPortManager::getAvailablePorts();
        if (serialPortSelector.getNumItems() != ports.size())
        {
            serialPortSelector.clear (juce::dontSendNotification);
            serialPortSelector.addItemList (ports, 1);
        }
    }
}

void MainComponent::timerCallback()
{
    refreshPortLists();
}

void MainComponent::midiMessageLogged (const juce::MidiMessage& message, bool outgoing)
{
    // Called from the MIDI thread for incoming messages -- hop to the message
    // thread before touching the GUI.
    juce::MessageManager::callAsync ([this, message, outgoing]
    {
        auto prefix = outgoing ? "MIDI OUT  " : "MIDI IN   ";
        logLine (prefix + message.getDescription());
    });
}

void MainComponent::serialMessageLogged (const juce::MidiMessage& message, bool outgoing)
{
    // Called from SerialPortManager's background read thread -- same
    // thread-hop rule applies.
    juce::MessageManager::callAsync ([this, message, outgoing]
    {
        auto prefix = outgoing ? "RS232 OUT " : "RS232 IN  ";
        logLine (prefix + message.getDescription());
    });
}

void MainComponent::serialStatusChanged (const juce::String& status)
{
    juce::MessageManager::callAsync ([this, status] { logLine ("[serial] " + status); });
}

void MainComponent::logLine (const juce::String& line)
{
    logBox.moveCaretToEnd();
    logBox.insertTextAtCaret (line + "\n");
}
