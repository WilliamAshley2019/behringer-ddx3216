#pragma once
#include <optional>
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
        32-47   Bus 1-16                                [MEDIUM-HIGH - contiguous after channels,
                                                          matches the PDF's section ordering AND
                                                          independently cross-corroborated by an
                                                          unrelated third-party project (a web-based
                                                          DDX3216 controller UI) using identical
                                                          ranges. Two independent inferences agreeing
                                                          is good evidence, still short of a direct
                                                          hardware-traffic capture]
        48-51   Aux master 1-4                          [MEDIUM-HIGH, see above]
        52-55   FX send master 1-4                      [MEDIUM-HIGH, see above]
        56-63   FX return 1-8                           [MEDIUM-HIGH, see above]
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
    constexpr uint8_t kFunctionChannelAttenuation = 0x22; // attenuates all channels in the same mute group at once

    // Newly cross-referenced from a second SysEx doc extraction, not
    // previously in this file. Named here for completeness but NOT wired
    // into any encode/decode logic yet -- payload formats are unconfirmed.
    constexpr uint8_t kFunctionConnectionTest = 0x00; // r=1 -> 0x40 (matches the "ping" already used elsewhere)
    constexpr uint8_t kFunctionMeterData      = 0x04; // genuinely new capability -- level/VU meter data.
                                                        // Could drive real-time meters in the app once the
                                                        // payload format is worked out; worth investigating.
    constexpr uint8_t kFunctionMemoryDump     = 0x0F; // distinct from the 0x50/0x51/0x52 file-category dump
                                                        // family above -- appears to be a lower-level raw memory
                                                        // access function. Possibly relevant to firmware work.
                                                        // Payload format unconfirmed.
    constexpr uint8_t kIgnoreApparatusAndChannel = 0x60; // omni

    // Build the "ic" device/channel byte. midiChannel1to16 == nullopt means
    // omni (0x60) -- matches getDeviceByte() in aldipower/bitwig-ddx3216-controller,
    // an independently-built, real-world-tested implementation for this exact
    // desk. That script defaults to omni; only switch to an explicit channel
    // if you've confirmed (by sniffing) the desk needs it.
    inline uint8_t deviceByteForChannel (std::optional<int> midiChannel1to16 = std::nullopt) noexcept
    {
        if (! midiChannel1to16.has_value())
            return kIgnoreApparatusAndChannel;
        return (uint8_t) (0x40 | ((*midiChannel1to16 - 1) & 0x0F));
    }

    // ---- RS232 transport settings, confirmed from Behringer's own
    // "DDX3216 File Exchange 1.1f.exe" tool (Borland C++, WinXP era). Its
    // BuildCommDCBA call uses the literal string "COM0:115200,n,8,1" --
    // i.e. 115200 baud, no parity, 8 data bits, 1 stop bit. Use this rather
    // than guessing; it's the vendor's own configuration, not inferred.
    namespace Serial
    {
        constexpr int kBaudRate  = 115200;
        constexpr char kParity   = 'N'; // none
        constexpr int kDataBits  = 8;
        constexpr int kStopBits  = 1;
    }

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
        // kVolume/kMute/kPan (1-3) are the same parameter numbers for Channel,
        // Bus, and Aux Master sections alike, per the official doc's separate
        // per-section tables -- only the module address changes.
        constexpr uint8_t kRouteToMain  = 4;
        constexpr uint8_t kRouteToBus   = 5;
        constexpr uint8_t kBusVolume    = 6;   // 0-1472, dB = -80 + value/16
        constexpr uint8_t kMasterBalance = 3;  // Master section only: 0-60, bal = value - 30

        constexpr uint8_t kAux1Send = 70, kAux1Pre = 71;
        constexpr uint8_t kAux2Send = 72, kAux2Pre = 73;
        constexpr uint8_t kAux3Send = 74, kAux3Pre = 75;
        constexpr uint8_t kAux4Send = 76, kAux4Pre = 77;

        // ---- Sourced externally: aldipower/bitwig-ddx3216-controller (GPLv3,
        // Felix Gertz) -- an independently-built, apparently-working Bitwig
        // controller script for this exact desk. Its hex function codes are
        // reproduced here since they cross-check against a second real-world
        // implementation, which is stronger evidence than anything we've
        // derived ourselves from the PDF alone. Not yet wired into the GUI --
        // next step is per-channel EQ/dynamics pages using these.
        namespace Eq5 // 5-band parametric EQ. NOTE the script's own comment:
        {             // "highest band on the DDX is index 0" -- i.e. band
                       // numbering is reversed vs the intuitive low-to-high order.
            constexpr uint8_t kEnable = 0x14;
            // per band, low->high as stored here (Bitwig index 4..0):
            constexpr uint8_t kBand1Type = 0x21, kBand1Freq = 0x22, kBand1Gain = 0x23, kBand1Q = 0x24;
            constexpr uint8_t kBand2Freq = 0x1E, kBand2Gain = 0x1F, kBand2Q = 0x20;
            constexpr uint8_t kBand3Freq = 0x1A, kBand3Gain = 0x1B, kBand3Q = 0x1C;
            // band 4 freq/gain/q codes not present in the source script (blank in its table)
            constexpr uint8_t kBand5Type = 0x15, kBand5Freq = 0x16, kBand5Gain = 0x17, kBand5Q = 0x18;
        }

        namespace HighPass // "EQ-2" in the source script
        {
            constexpr uint8_t kEnable = 0x25;
            constexpr uint8_t kFreq   = 0x26;
        }

        namespace Gate
        {
            constexpr uint8_t kEnable    = 0x32;
            constexpr uint8_t kAttack    = 0x34;
            constexpr uint8_t kRelease   = 0x35;
            constexpr uint8_t kDepth     = 0x36; // "DDX Range"
            constexpr uint8_t kThreshold = 0x37;
        }

        namespace Compressor
        {
            constexpr uint8_t kEnable    = 0x28;
            constexpr uint8_t kAttack    = 0x2A;
            constexpr uint8_t kRelease   = 0x2B;
            constexpr uint8_t kRatio     = 0x2C;
            constexpr uint8_t kThreshold = 0x2E;
            constexpr uint8_t kOutput    = 0x2F;
        }

        namespace Delay
        {
            constexpr uint8_t kEnable   = 0x3C;
            constexpr uint8_t kPhase    = 0x3D;
            constexpr uint8_t kTime     = 0x3E;
            constexpr uint8_t kFeedback = 0x3F;
            constexpr uint8_t kMix      = 0x40;
        }
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

    // Function 0x22 -- distinct from parameter-change: 3 bytes per entry
    // (channel, high, low -- no separate parameter-number byte), and per the
    // official doc it attenuates ALL channels in the same mute group at
    // once, rather than one channel's own volume. channel 0 == "channel 1"
    // per the doc's own wording.
    juce::MidiMessage buildChannelAttenuation (uint8_t channel, int rawValue,
                                                uint8_t deviceByte = kIgnoreApparatusAndChannel);

    // Parse an incoming SysEx message. Returns empty vector if it isn't a
    // recognised DDX3216 parameter-change frame.
    std::vector<ParamChange> parseIncomingSysEx (const juce::MidiMessage& message);

    // ---- Bulk file-dump protocol (settings/library/snapshot/firmware
    // transfer -- this is what "DDX3216 File Exchange.exe" and the
    // Operating-System/firmware-update path use). Same F0...F7 SysEx framing
    // as parameter-change, just a different function-code family, and it
    // runs over EITHER MIDI or the RS232 port (per the PDF's own wording,
    // and confirmed by the vendor tool supporting both transports for this
    // same file-category list).
    //
    // Frame shape: F0 00 20 32 <ic> 0B <function> ... F7
    //   function 0x50 (send) / 0x10 (request) -- current settings
    //   function 0x51 (send) / 0x11 (request) -- PC-card file list
    //   function 0x52 (send) / 0x12 (request) -- a named file
    //
    // Data-frame body after the function byte (when NOT a request):
    //   vv        data file version (1..0x7F)
    //   hh ll     total number of 1000-byte blocks (hh*128+ll)
    //   hh ll     this block's index (0 = first)
    //   dd        byte count of the ENCODED payload that follows (always a
    //             multiple of 8 -- see the 7/8 packing note below)
    //   ..        encoded payload
    //   cc        checksum = NOT(sum of all preceding data bytes) & 0x7F
    //
    // Request-frame body (request bit set): just the block index (hh ll),
    // or a C-string filename for the file-list/single-file functions.
    //
    // 7/8 packing: SysEx data bytes must be 7-bit clean (top bit clear), so
    // 7 arbitrary 8-bit payload bytes are sent as 8 encoded bytes: encoded
    // bytes 0-6 carry the low 7 bits of payload bytes 0-6, and encoded byte 7
    // carries their 7 high bits packed together (bit i = high bit of payload
    // byte i). This is the same scheme as the standard MIDI Sample Dump
    // Standard. It's why the PDF says "byte count is always modulo 8" --
    // that's the ENCODED count, 8 bytes per 7 real payload bytes.
    namespace BulkDump
    {
        constexpr uint8_t kFuncDumpCurrentSettings    = 0x50;
        constexpr uint8_t kFuncRequestCurrentSettings = 0x10;
        constexpr uint8_t kFuncDumpFileList           = 0x51;
        constexpr uint8_t kFuncRequestFileList        = 0x11;
        constexpr uint8_t kFuncDumpFile               = 0x52;
        constexpr uint8_t kFuncRequestFile            = 0x12;

        // File-category ("what") byte, sent right after the function code in
        // every request/dump header per the official doc's byte-7 field --
        // BUG FIX: earlier versions of this file omitted this byte entirely,
        // which would have made every bulk-dump frame malformed. The doc
        // names these symbolically (F_ALL, F_SETUP, F_CHANL, F_EQL, F_DYNL,
        // F_FXL, F_AUT, F_SNAPS) but never gives their actual numeric values
        // -- those are NOT yet known/confirmed. Callers must supply the
        // right byte once it's been determined (e.g. by sniffing a real
        // File Exchange session, or trial-and-error against the console).
        namespace WhatByte
        {
            // Placeholder names only -- values are UNKNOWN. Do not trust 0x00
            // as a real default; it's just an explicit placeholder so this
            // compiles, not a documented value.
            constexpr uint8_t kUnknown = 0x00;
        }

        constexpr int kBlockPayloadSize = 1000; // decoded bytes of payload per block, per the PDF

        // checksum = NOT(sum of the preceding data bytes) & 0x7F
        inline uint8_t computeChecksum (const uint8_t* data, size_t length) noexcept
        {
            uint32_t sum = 0;
            for (size_t i = 0; i < length; ++i)
                sum += data[i];
            return (uint8_t) ((~sum) & 0x7F);
        }

        // Encode up to 7 raw bytes into 8 7-bit-clean bytes. `in` must have
        // inLength <= 7 valid bytes; `out` must have room for 8 bytes.
        // Returns the number of encoded bytes written (always 8, even for a
        // short final group -- pad `in` with zeros for a partial group and
        // track the real payload length separately).
        inline int encode7Bit (const uint8_t* in, int inLength, uint8_t* out) noexcept
        {
            jassert (inLength >= 0 && inLength <= 7);
            uint8_t highBits = 0;
            for (int i = 0; i < 7; ++i)
            {
                uint8_t b = (i < inLength) ? in[i] : 0;
                out[i] = b & 0x7F;
                if (b & 0x80)
                    highBits |= (uint8_t) (1 << i);
            }
            out[7] = highBits;
            return 8;
        }

        // Decode 8 7-bit-clean bytes back into 7 raw bytes.
        inline void decode7Bit (const uint8_t* in, uint8_t* out) noexcept
        {
            uint8_t highBits = in[7];
            for (int i = 0; i < 7; ++i)
                out[i] = (uint8_t) ((in[i] & 0x7F) | (((highBits >> i) & 1) << 7));
        }

        // Encode an arbitrary-length buffer using the 7/8 scheme above.
        inline std::vector<uint8_t> encodeBuffer (const std::vector<uint8_t>& data)
        {
            std::vector<uint8_t> encoded;
            encoded.reserve ((data.size() / 7 + 1) * 8);
            size_t i = 0;
            while (i < data.size())
            {
                int chunk = (int) std::min<size_t> (7, data.size() - i);
                uint8_t out[8];
                encode7Bit (data.data() + i, chunk, out);
                encoded.insert (encoded.end(), out, out + 8);
                i += (size_t) chunk;
            }
            return encoded;
        }

        // Decode a 7/8-packed buffer. decodedLength is the real (unpadded)
        // payload length, since the last group may be padded with zeros.
        inline std::vector<uint8_t> decodeBuffer (const std::vector<uint8_t>& encoded, size_t decodedLength)
        {
            std::vector<uint8_t> decoded;
            decoded.reserve (decodedLength);
            for (size_t i = 0; i + 8 <= encoded.size() && decoded.size() < decodedLength; i += 8)
            {
                uint8_t out[7];
                decode7Bit (encoded.data() + i, out);
                size_t remaining = decodedLength - decoded.size();
                decoded.insert (decoded.end(), out, out + std::min<size_t> (7, remaining));
            }
            return decoded;
        }

        // A single parsed data block (function 0x50/0x51/0x52 response, i.e.
        // request bit == 0).
        struct DataBlock
        {
            uint8_t whatByte = 0;
            uint8_t version = 1;
            int totalBlocks = 0;
            int blockIndex = 0;
            std::vector<uint8_t> decodedPayload;
            bool checksumOk = false;
        };

        // Build a "request block N" frame for the settings/current-config
        // path (function 0x10). filename-based file requests (0x11/0x12)
        // aren't built here yet -- add a buildFileRequest() alongside this
        // when the file-list/single-file paths are wired up.
        //
        // whatByte: the file-category selector (F_ALL/F_EQL/etc. -- see
        // WhatByte namespace above). BUG FIX: earlier versions of this
        // function omitted this required byte entirely.
        inline juce::MidiMessage buildRequestBlock (uint8_t requestFunction, uint8_t whatByte, int blockIndex,
                                                     uint8_t deviceByte = kIgnoreApparatusAndChannel)
        {
            std::vector<uint8_t> body;
            body.push_back (kBehringerManufacturerId[0]);
            body.push_back (kBehringerManufacturerId[1]);
            body.push_back (kBehringerManufacturerId[2]);
            body.push_back (deviceByte);
            body.push_back (kApparatusId);
            body.push_back (requestFunction);
            body.push_back (whatByte);
            body.push_back ((uint8_t) ((blockIndex >> 7) & 0x7F)); // hh
            body.push_back ((uint8_t) (blockIndex & 0x7F));        // ll
            return juce::MidiMessage::createSysExMessage (body.data(), (int) body.size());
        }

        // Build a data-frame carrying one block of an outgoing (PC -> desk)
        // transfer.
        inline juce::MidiMessage buildDataBlock (uint8_t dumpFunction, uint8_t whatByte, uint8_t version,
                                                  int totalBlocks, int blockIndex,
                                                  const std::vector<uint8_t>& rawPayload,
                                                  uint8_t deviceByte = kIgnoreApparatusAndChannel)
        {
            auto encodedPayload = encodeBuffer (rawPayload);
            jassert (encodedPayload.size() % 8 == 0);

            std::vector<uint8_t> data; // everything the checksum covers (after the function byte)
            data.push_back (whatByte);
            data.push_back (version);
            data.push_back ((uint8_t) ((totalBlocks >> 7) & 0x7F));
            data.push_back ((uint8_t) (totalBlocks & 0x7F));
            data.push_back ((uint8_t) ((blockIndex >> 7) & 0x7F));
            data.push_back ((uint8_t) (blockIndex & 0x7F));
            data.push_back ((uint8_t) (encodedPayload.size() & 0x7F)); // byte count, mod 8 per the PDF
            data.insert (data.end(), encodedPayload.begin(), encodedPayload.end());
            data.push_back (computeChecksum (data.data(), data.size()));

            std::vector<uint8_t> body;
            body.push_back (kBehringerManufacturerId[0]);
            body.push_back (kBehringerManufacturerId[1]);
            body.push_back (kBehringerManufacturerId[2]);
            body.push_back (deviceByte);
            body.push_back (kApparatusId);
            body.push_back (dumpFunction);
            body.insert (body.end(), data.begin(), data.end());

            return juce::MidiMessage::createSysExMessage (body.data(), (int) body.size());
        }

        // Parse an incoming data-frame (request bit == 0) for any of the
        // dump functions. Returns std::nullopt if the message isn't a
        // recognised DDX3216 bulk-dump data frame.
        inline std::optional<DataBlock> parseDataBlock (const juce::MidiMessage& message)
        {
            if (! message.isSysEx())
                return std::nullopt;

            auto* d = message.getSysExData();
            auto size = message.getSysExDataSize();

            // 00 20 32 <ic> 0B <func> <ww> <vv> <hh> <ll> <hh> <ll> <dd> ...payload... <cc>
            if (size < 13)
                return std::nullopt;

            if (d[0] != kBehringerManufacturerId[0] || d[1] != kBehringerManufacturerId[1]
                || d[2] != kBehringerManufacturerId[2] || d[4] != kApparatusId)
                return std::nullopt;

            auto func = d[5];
            if (func != kFuncDumpCurrentSettings && func != kFuncDumpFileList && func != kFuncDumpFile)
                return std::nullopt;

            DataBlock block;
            block.whatByte = d[6];
            block.version = d[7];
            block.totalBlocks = (d[8] << 7) | d[9];
            block.blockIndex  = (d[10] << 7) | d[11];
            auto encodedByteCount = d[12];

            auto payloadStart = 13;
            if (size < payloadStart + encodedByteCount + 1) // +1 for checksum byte
                return std::nullopt;

            std::vector<uint8_t> checksummedData (d + 6, d + payloadStart + encodedByteCount);
            auto expectedChecksum = computeChecksum (checksummedData.data(), checksummedData.size());
            auto actualChecksum = d[payloadStart + encodedByteCount];
            block.checksumOk = (expectedChecksum == actualChecksum);

            std::vector<uint8_t> encodedPayload (d + payloadStart, d + payloadStart + encodedByteCount);
            // Real (unpadded) decoded length isn't carried in the frame itself for
            // a partial final block -- callers that know the true file size should
            // trim block.decodedPayload themselves; this gives the full decode of
            // whatever was sent, including any zero-padding on the last group.
            block.decodedPayload = decodeBuffer (encodedPayload, (encodedPayload.size() / 8) * 7);

            return block;
        }
    }
}
