#include "MainComponent.h"

static constexpr int kStripWidth = 90;

MainComponent::MainComponent()
{
    addAndMakeVisible (inputLabel);
    addAndMakeVisible (outputLabel);
    addAndMakeVisible (inputSelector);
    addAndMakeVisible (outputSelector);

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
    refreshPortLists();
    startTimer (2000); // re-scan for newly connected MIDI devices every 2s

    setSize (1000, 650);
}

MainComponent::~MainComponent()
{
    io.removeRawMessageListener (this);
    stopTimer();
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (8);

    auto topRow = area.removeFromTop (28);
    inputLabel.setBounds (topRow.removeFromLeft (60));
    inputSelector.setBounds (topRow.removeFromLeft (220));
    topRow.removeFromLeft (16);
    outputLabel.setBounds (topRow.removeFromLeft (70));
    outputSelector.setBounds (topRow.removeFromLeft (220));

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
        auto prefix = outgoing ? "OUT  " : "IN   ";
        logLine (prefix + message.getDescription());
    });
}

void MainComponent::logLine (const juce::String& line)
{
    logBox.moveCaretToEnd();
    logBox.insertTextAtCaret (line + "\n");
}
