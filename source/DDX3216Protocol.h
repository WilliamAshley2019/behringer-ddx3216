#pragma once
#include <JuceHeader.h>

/*
    DDX3216 direct-parameter-change SysEx layer.

    Frame format (from Behringer's official SysEx Documentation v1.0, function 0x20):

        F0 00 20 32 <ic> 0B 20 <nn> [ <module> <param> <hi7> <lo7> ] * nn  F7

        ic     - device/channel byte. 0x60 = ignore appID + ignore MIDI channel (omni).
                 Use this unless you've deliberately set a MIDI channel on the desk.
        0B     - apparatus ID for DDX3216 (fixed).
        0x20   - function code, direct parameter change ("here's data", not a request).
        nn     - number of parameters in this frame (1..23).
        module - the "channel" address (see ModuleAddress below).
        param  - parameter number (see per-section notes below).
        hi/lo  - 14-bit value split into two 7-bit bytes, HIGH BYTE FIRST, then low.
                 NOTE: earlier draft code in this project had this backwards (low
                 before high) and additionally mis-slotted an extra byte -- this
                 header is the corrected version, cross-checked directly against
                 the PDF wording ("3e = parameter high 7 bit ... 4e = parameter low
                 7 bit").

    Module address space (CONFIRMED vs INFERRED):

        0-31    Channels 1-32                          [CONFIRMED - existing code paths use this]
        32-47   Bus 1-16                                [INFERRED - contiguous after channels, matches
                                                          the PDF's section ordering, NOT yet verified
                                                          against a real desk]
        48-51   Aux master 1-4                          [INFERRED]
        52-55   FX send master 1-4                      [INFERRED]
        56-63   FX return 1-8                           [INFERRED]
        64-65   Master L / Master R                     [CONFIRMED - matches the working Node.js
                                                          implementation in this project, which sends
                                                          module 64 for its single "master" channel and
                                                          allocates 65 for a second master channel]

    Treat every INFERRED value as a hypothesis to be confirmed by sniffing real
    traffic (move a bus fader on the desk and see what module byte shows up).
    Do NOT ship user-facing behaviour that assumes an inferred value is correct
    without a "verified" flag somewhere obvious in the UI.
*/

namespace DDX3216
{
    constexpr int kBehringerManufacturerId[3] = { 0x00, 0x20, 0x32 };
    constexpr uint8_t kApparatusId       = 0x0B;   // DDX3216
    constexpr uint8_t kFunctionParamChange = 0x20; // direct parameter change
    constexpr uint8_t kIgnoreApparatusAndChannel = 0x60; // omni

    // ---- Module address space (see header comment above for confirmed/inferred) ----
    namespace Module
    {
        constexpr int kChannelBase   = 0;   // channels 1-32 -> 0-31
        constexpr int kChannelCount  = 32;

        constexpr int kBusBase       = 32;  // INFERRED
        constexpr int kBusCount      = 16;

        constexpr int kAuxMasterBase = 48;  // INFERRED
        constexpr int kAuxMasterCount = 4;

        constexpr int kFxSendBase    = 52;  // INFERRED
        constexpr int kFxSendCount   = 4;

        constexpr int kFxReturnBase  = 56;  // INFERRED
        constexpr int kFxReturnCount = 8;

        constexpr int kMasterLeft    = 64;  // CONFIRMED (see header comment)
        constexpr int kMasterRight   = 65;  // CONFIRMED
    }

    // ---- Parameter numbers, per the PDF's "Ch. 1-32" table (also reused for
    //      Bus/Aux/FX-master/FX-return where the same parameter exists there) ----
    namespace Param
    {
        constexpr uint8_t kVolume       = 1;   // 0-1472, dB = -80 + value/16
        constexpr uint8_t kMute         = 2;   // 0/1
        constexpr uint8_t kPan          = 3;   // 0-60, position = value - 30
        constexpr uint8_t kRouteToMain  = 4;
        constexpr uint8_t kRouteToBus   = 5;
        constexpr uint8_t kBusVolume    = 6;   // 0-1472, dB = -80 + value/16
        constexpr uint8_t kMasterBalance = 3;  // Master section only: 0-60, bal = value - 30

        constexpr uint8_t kAux1Send = 70, kAux1Pre = 71;
        constexpr uint8_t kAux2Send = 72, kAux2Pre = 73;
        constexpr uint8_t kAux3Send = 74, kAux3Pre = 75;
        constexpr uint8_t kAux4Send = 76, kAux4Pre = 77;
    }

    // ---- Value <-> physical unit conversions (from the PDF's "scale" column) ----
    double volumeRawToDb (int raw) noexcept;   // 0-1472 -> dB (-80..+12)
    int    volumeDbToRaw (double db) noexcept;

    double panRawToPosition (int raw) noexcept; // 0-60 -> -30..+30
    int    panPositionToRaw (double pos) noexcept;

    // ---- One changed parameter within a frame ----
    struct ParamChange
    {
        uint8_t module;
        uint8_t param;
        int     rawValue; // 0-16383 (14-bit), already combined from hi/lo
    };

    // Build a complete SysEx frame (including F0/F7) for 1-23 parameter changes.
    juce::MidiMessage buildParamChangeFrame (const std::vector<ParamChange>& changes,
                                              uint8_t deviceByte = kIgnoreApparatusAndChannel);

    // Convenience: single-parameter frame.
    juce::MidiMessage buildSingleParamChange (uint8_t module, uint8_t param, int rawValue,
                                               uint8_t deviceByte = kIgnoreApparatusAndChannel);

    // Parse an incoming SysEx message. Returns empty vector if it isn't a
    // recognised DDX3216 parameter-change frame.
    std::vector<ParamChange> parseIncomingSysEx (const juce::MidiMessage& message);
}
