# name=FLStudio_DDX3216
# url=
# supportedDevices=FLStudio_DDX3216
# version 2026.1

"""
device_DDX3216.py

FL Studio MIDI Controller script for the Behringer DDX3216.

INSTALL: copy this file and ddx3216_protocol.py into
    Documents\\Image-Line\\FL Studio\\Settings\\Hardware\\DDX3216\\
then select "DDX3216" as a MIDI controller in FL Studio's MIDI settings,
with input/output set to your DDX3216 MIDI interface.

STATUS / KNOWN LIMITATIONS (read this before assuming something's broken):

  * Physical fader/pan -> FL Studio mixer track: WORKING, via MIDI CC
    (confirmed against real hardware traffic during development).
  * FL Studio mixer track -> physical fader/pan motor movement: NOT YET
    CONFIRMED. The outgoing messages this script sends use the same
    protocol as the standalone JUCE controller app, which as of this
    writing has an open, unresolved question about whether outgoing
    writes actually drive the desk's motors (see that project's
    docs/ROADMAP.md). If faders don't move when you touch them in FL
    Studio, that's very likely the same underlying issue, not a bug
    specific to this script.
  * Mute is wired via CC only; SysEx mute is available in
    ddx3216_protocol.py but not sent by this script yet.
  * Channel mapping is a straight 1:1 -- DDX3216 channel N <-> FL Studio
    mixer track N (1-indexed in the UI, 0-indexed internally). Change
    CHANNEL_OFFSET below if you want it to start elsewhere.
  * The dB<->FL-volume conversion below is a first-pass approximation, not
    calibrated against FL's actual internal volume curve. Expect fader
    positions to be in the right ballpark but not pixel/dB-perfect; tune
    VOLUME_MIN_DB/VOLUME_MAX_DB or the conversion function by ear/eye if
    that matters to you.
"""

import midi
import device
import mixer
import transport
import ui

import ddx3216_protocol as proto

CHANNEL_OFFSET = 0       # DDX3216 channel 1 (index 0) -> FL mixer track 0 + this
NUM_CHANNELS = 32
DEVICE_MIDI_CHANNEL = 2  # matches this project's sniffed hardware traffic;
                          # set to None for omni if your unit responds to that instead

# FL Studio mixer volume is roughly 0.0-1.0 internally (not a direct dB
# scale). This is a simple linear approximation over the DDX3216's -80..+12dB
# fader range -- see the calibration note above.
VOLUME_MIN_DB = -80.0
VOLUME_MAX_DB = 12.0


def db_to_fl_volume(db):
    span = VOLUME_MAX_DB - VOLUME_MIN_DB
    return max(0.0, min(1.0, (db - VOLUME_MIN_DB) / span))


def fl_volume_to_db(vol):
    span = VOLUME_MAX_DB - VOLUME_MIN_DB
    return VOLUME_MIN_DB + max(0.0, min(1.0, vol)) * span


def fl_track_for_channel(channel_index):
    return channel_index + CHANNEL_OFFSET


def channel_for_fl_track(track_index):
    ch = track_index - CHANNEL_OFFSET
    if 0 <= ch < NUM_CHANNELS:
        return ch
    return None


# ---------------------------------------------------------------------------
# FL Studio script lifecycle
# ---------------------------------------------------------------------------

def OnInit():
    print("DDX3216 control surface script loaded")
    # Track which mixer changes originated from the hardware itself, so we
    # don't immediately echo them back out and fight the motorized faders
    # (a feedback loop between "hardware moved" -> "update FL" -> "FL changed,
    # tell hardware" -> ...). Simple one-shot suppression per channel.
    global _suppress_echo
    _suppress_echo = set()


def OnDeInit():
    print("DDX3216 control surface script unloaded")


def OnMidiMsg(event):
    if event.sysex:
        _handle_incoming_sysex(event)
        return

    if event.midiId == midi.MIDI_CONTROLCHANGE:
        _handle_incoming_cc(event)


def OnDirtyMixerTrack(index):
    """Called by FL Studio when a mixer track's volume/pan/mute changes for
    ANY reason (mouse, automation, another controller, or our own incoming
    CC handling below). We use this to push the new value back out to the
    hardware -- this is the half of the round-trip that's unconfirmed on
    real hardware; see the module docstring."""
    channel = channel_for_fl_track(index)
    if channel is None:
        return

    if channel in _suppress_echo:
        _suppress_echo.discard(channel)
        return

    _send_volume_to_hardware(channel, mixer.getTrackVolume(index))
    _send_pan_to_hardware(channel, mixer.getTrackPan(index))


# ---------------------------------------------------------------------------
# Incoming (hardware -> FL Studio)
# ---------------------------------------------------------------------------

def _handle_incoming_cc(event):
    cc = event.data1
    value7bit = event.data2

    if proto.is_fader_cc(cc):
        channel = proto.cc_to_channel_index(cc)
        track = fl_track_for_channel(channel)
        if track < mixer.trackCount():
            fl_vol = value7bit / 127.0  # simple linear 0-127 -> 0.0-1.0
            _suppress_echo.add(channel)
            mixer.setTrackVolume(track, fl_vol)
        event.handled = True

    elif proto.is_pan_cc(cc):
        channel = proto.cc_to_channel_index(cc)
        track = fl_track_for_channel(channel)
        if track < mixer.trackCount():
            fl_pan = (value7bit / 127.0) * 2.0 - 1.0  # 0-127 -> -1.0..+1.0
            _suppress_echo.add(channel)
            mixer.setTrackPan(track, fl_pan)
        event.handled = True

    elif cc == proto.MUTE_ON_CC:
        channel = value7bit
        track = fl_track_for_channel(channel)
        if track < mixer.trackCount():
            mixer.muteTrack(track)  # NOTE: this TOGGLES in most FL API
            # versions -- if your build's muteTrack doesn't take an explicit
            # on/off, you may need mixer.setTrackMuted(track, True) instead;
            # check against your installed FL Studio's API docs.
        event.handled = True

    elif cc == proto.MUTE_OFF_CC:
        # see note above re: mute API shape
        event.handled = True


def _handle_incoming_sysex(event):
    changes = proto.parse_param_change_sysex(list(event.sysex))
    for module, param, raw in changes:
        if not (proto.MODULE_CHANNEL_BASE <= module < proto.MODULE_CHANNEL_BASE + NUM_CHANNELS):
            continue  # bus/aux/FX modules -- addresses still unconfirmed, ignore for now

        channel = module - proto.MODULE_CHANNEL_BASE
        track = fl_track_for_channel(channel)
        if track >= mixer.trackCount():
            continue

        if param == proto.PARAM_VOLUME:
            db = proto.volume_raw_to_db(raw)
            _suppress_echo.add(channel)
            mixer.setTrackVolume(track, db_to_fl_volume(db))
        elif param == proto.PARAM_PAN:
            pos = proto.pan_raw_to_position(raw)  # -30..+30
            _suppress_echo.add(channel)
            mixer.setTrackPan(track, pos / 30.0)  # -> -1.0..+1.0

    event.handled = True


# ---------------------------------------------------------------------------
# Outgoing (FL Studio -> hardware)
# ---------------------------------------------------------------------------

def _send_volume_to_hardware(channel, fl_volume):
    db = fl_volume_to_db(fl_volume)
    raw = proto.volume_db_to_raw(db)
    device_byte = proto.device_byte_for_channel(DEVICE_MIDI_CHANNEL)
    msg = proto.build_param_change_sysex(
        proto.MODULE_CHANNEL_BASE + channel, proto.PARAM_VOLUME, raw, device_byte
    )
    device.midiOutSysex(bytes(msg))


def _send_pan_to_hardware(channel, fl_pan):
    position = fl_pan * 30.0  # -1.0..+1.0 -> -30..+30
    raw = proto.pan_position_to_raw(position)
    device_byte = proto.device_byte_for_channel(DEVICE_MIDI_CHANNEL)
    msg = proto.build_param_change_sysex(
        proto.MODULE_CHANNEL_BASE + channel, proto.PARAM_PAN, raw, device_byte
    )
    device.midiOutSysex(bytes(msg))
