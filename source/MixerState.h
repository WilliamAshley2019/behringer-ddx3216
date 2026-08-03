#pragma once
#include <array>
#include <JuceHeader.h>
#include "DDX3216Protocol.h"

/*
    Single source of truth for everything the GUI displays. Both the GUI (on
    user interaction) and the MIDI receive callback (on incoming SysEx/CC)
    write into this; both read from it to stay in sync. Keep this dumb (no
    JUCE Component dependencies) so it's easy to unit-test in isolation later.
*/
struct MixerState
{
    static constexpr int kChannelCount = DDX3216::Module::kChannelCount;

    struct Channel
    {
        double volumeDb = -80.0;
        double pan      = 0.0;   // -30..+30
        bool   mute     = false;
    };

    std::array<Channel, kChannelCount> channels;

    double masterVolumeDb = -80.0;
    double masterBalance  = 0.0;

    // Listener interface so the GUI can repaint only what changed, instead of
    // polling the whole state every frame.
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void channelChanged (int channelIndex) {}
        virtual void masterChanged() {}
    };

    void addListener (Listener* l)    { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

    void setChannelVolumeDb (int ch, double db)
    {
        if (! isPositiveAndBelow (ch, kChannelCount)) return;
        channels[(size_t) ch].volumeDb = db;
        listeners.call ([ch] (Listener& l) { l.channelChanged (ch); });
    }

    void setChannelPan (int ch, double pan)
    {
        if (! isPositiveAndBelow (ch, kChannelCount)) return;
        channels[(size_t) ch].pan = pan;
        listeners.call ([ch] (Listener& l) { l.channelChanged (ch); });
    }

    void setChannelMute (int ch, bool mute)
    {
        if (! isPositiveAndBelow (ch, kChannelCount)) return;
        channels[(size_t) ch].mute = mute;
        listeners.call ([ch] (Listener& l) { l.channelChanged (ch); });
    }

    void setMasterVolumeDb (double db) { masterVolumeDb = db; listeners.call ([] (Listener& l) { l.masterChanged(); }); }
    void setMasterBalance (double b)   { masterBalance  = b;  listeners.call ([] (Listener& l) { l.masterChanged(); }); }

private:
    static bool isPositiveAndBelow (int v, int upper) { return v >= 0 && v < upper; }
    juce::ListenerList<Listener> listeners;
};
