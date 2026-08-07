"""
ddx3216_protocol.py

Protocol helpers for the Behringer DDX3216, shared by the FL Studio device
script. This is a Python port of the relevant parts of DDX3216Protocol.h
from the standalone JUCE controller project -- keep the two in sync if the
protocol understanding changes (e.g. once the bus/aux/FX module addresses
move from INFERRED to CONFIRMED).

Two control paths are implemented:

  * MIDI CC (PRIMARY) -- confirmed bidirectionally active on real hardware
    during testing: moving a physical fader emits a standard Control Change
    message (e.g. CC2 = channel 2's fader), matching this mapping. This is
    the simpler, better-tested path.

  * SysEx direct-parameter-change (SECONDARY, more complete) -- matches the
    official documentation and the JUCE app, gives access to pan/mute (which
    aren't in the CC map) and the wider parameter set (EQ, dynamics, etc.),
    but outgoing SysEx writes actually moving the motorized faders is NOT
    YET CONFIRMED working on real hardware as of this script's writing --
    see the project's docs/ROADMAP.md for the live status of that question.
"""

# ---------------------------------------------------------------------------
# MIDI CC map (from the original project's midi_definitions.py, confirmed by
# observed traffic: a channel-2 fader move emitted CC2)
# ---------------------------------------------------------------------------
FADER_CC_START = 1     # channel 1 fader = CC1, channel 2 = CC2, ... channel 32 = CC32
FADER_CC_END = 32
PAN_CC_START = 64       # channel 1 pan = CC64, ... channel 32 = CC95
PAN_CC_END = 95
MUTE_ON_CC = 104         # value = channel number (0-based)
MUTE_OFF_CC = 105        # value = channel number (0-based)
MAIN_FADER_CC = 61


def cc_to_channel_index(cc_number):
    """Returns 0-based channel index for a fader/pan CC, or None if cc_number
    isn't a channel fader/pan CC."""
    if FADER_CC_START <= cc_number <= FADER_CC_END:
        return cc_number - FADER_CC_START
    if PAN_CC_START <= cc_number <= PAN_CC_END:
        return cc_number - PAN_CC_START
    return None


def is_fader_cc(cc_number):
    return FADER_CC_START <= cc_number <= FADER_CC_END


def is_pan_cc(cc_number):
    return PAN_CC_START <= cc_number <= PAN_CC_END


def fader_cc_for_channel(channel_index):
    """0-based channel index -> CC number for that channel's fader."""
    return FADER_CC_START + channel_index


def pan_cc_for_channel(channel_index):
    return PAN_CC_START + channel_index


# ---------------------------------------------------------------------------
# SysEx direct-parameter-change (function 0x20), matching DDX3216Protocol.h
# ---------------------------------------------------------------------------
MANUFACTURER_ID = (0x00, 0x20, 0x32)
APPARATUS_ID = 0x0B
FUNC_PARAM_CHANGE = 0x20
DEVICE_BYTE_OMNI = 0x60

PARAM_VOLUME = 1   # 0-1472, dB = -80 + value/16
PARAM_MUTE = 2     # 0/1
PARAM_PAN = 3      # 0-60, position = value - 30

MODULE_CHANNEL_BASE = 0   # channels 1-32 -> module 0-31
MODULE_MASTER_LEFT = 64   # CONFIRMED, see DDX3216Protocol.h
MODULE_MASTER_RIGHT = 65  # CONFIRMED


def device_byte_for_channel(midi_channel_1to16=None):
    """None -> omni (0x60). Otherwise 1-16 -> 0x40 | (channel-1)."""
    if midi_channel_1to16 is None:
        return DEVICE_BYTE_OMNI
    return 0x40 | ((midi_channel_1to16 - 1) & 0x0F)


def volume_db_to_raw(db):
    raw = round((db + 80.0) * 16.0)
    return max(0, min(1472, raw))


def volume_raw_to_db(raw):
    return -80.0 + raw / 16.0


def pan_position_to_raw(position):
    raw = round(position + 30.0)
    return max(0, min(60, raw))


def pan_raw_to_position(raw):
    return raw - 30.0


def build_param_change_sysex(module, param, raw_value, device_byte=DEVICE_BYTE_OMNI):
    """Returns a list of ints (F0..F7 inclusive) for a single-parameter
    direct-parameter-change frame. Pass to device.midiOutSysex(bytes(...))."""
    hi = (raw_value >> 7) & 0x7F
    lo = raw_value & 0x7F
    return [
        0xF0,
        MANUFACTURER_ID[0], MANUFACTURER_ID[1], MANUFACTURER_ID[2],
        device_byte,
        APPARATUS_ID,
        FUNC_PARAM_CHANGE,
        1,          # nn = 1 parameter in this frame
        module & 0x7F,
        param & 0x7F,
        hi, lo,
        0xF7,
    ]


def parse_param_change_sysex(data):
    """data: list/bytes of the full F0..F7 message. Returns a list of
    (module, param, raw_value) tuples, or [] if not a recognised frame."""
    if len(data) < 13 or data[0] != 0xF0 or data[-1] != 0xF7:
        return []
    if tuple(data[1:4]) != MANUFACTURER_ID:
        return []
    if data[5] != APPARATUS_ID or data[6] != FUNC_PARAM_CHANGE:
        return []

    nn = data[7]
    results = []
    for i in range(nn):
        base = 8 + 4 * i
        if base + 4 > len(data) - 1:  # -1 to leave room for trailing F7
            break
        module, param, hi, lo = data[base:base + 4]
        raw_value = (hi << 7) | lo
        results.append((module, param, raw_value))
    return results
