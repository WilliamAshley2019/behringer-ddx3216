#include "DDX3216Protocol.h"

namespace DDX3216
{
    double volumeRawToDb (int raw) noexcept
    {
        return -80.0 + (double) raw / 16.0;
    }

    int volumeDbToRaw (double db) noexcept
    {
        auto raw = juce::roundToInt ((db + 80.0) * 16.0);
        return juce::jlimit (0, 1472, raw);
    }

    double panRawToPosition (int raw) noexcept
    {
        return (double) raw - 30.0;
    }

    int panPositionToRaw (double pos) noexcept
    {
        auto raw = juce::roundToInt (pos + 30.0);
        return juce::jlimit (0, 60, raw);
    }

    juce::MidiMessage buildParamChangeFrame (const std::vector<ParamChange>& changes,
                                              uint8_t deviceByte)
    {
        jassert (! changes.empty() && changes.size() <= 23);

        std::vector<uint8_t> data;
        data.reserve (2 + 4 * changes.size());

        data.push_back (kFunctionParamChange);
        data.push_back ((uint8_t) changes.size());

        for (auto& c : changes)
        {
            auto hi = (uint8_t) ((c.rawValue >> 7) & 0x7F);
            auto lo = (uint8_t) (c.rawValue & 0x7F);

            data.push_back (c.module);
            data.push_back (c.param);
            data.push_back (hi); // high byte FIRST, per the PDF
            data.push_back (lo);
        }

        // Assemble full SysEx body (without F0/F7 - JUCE adds those for us
        // when we pass the raw inner bytes to MidiMessage::createSysExMessage).
        std::vector<uint8_t> body;
        body.push_back (kBehringerManufacturerId[0]);
        body.push_back (kBehringerManufacturerId[1]);
        body.push_back (kBehringerManufacturerId[2]);
        body.push_back (deviceByte);
        body.push_back (kApparatusId);
        for (auto b : data)
            body.push_back (b);

        return juce::MidiMessage::createSysExMessage (body.data(), (int) body.size());
    }

    juce::MidiMessage buildSingleParamChange (uint8_t module, uint8_t param, int rawValue,
                                               uint8_t deviceByte)
    {
        return buildParamChangeFrame ({ ParamChange { module, param, rawValue } }, deviceByte);
    }

    std::vector<ParamChange> parseIncomingSysEx (const juce::MidiMessage& message)
    {
        std::vector<ParamChange> result;

        if (! message.isSysEx())
            return result;

        auto* data = message.getSysExData();
        auto size  = message.getSysExDataSize();

        // Expect: 00 20 32 <ic> 0B 20 <nn> [module param hi lo]*nn
        if (size < 7)
            return result;

        if (data[0] != kBehringerManufacturerId[0] ||
            data[1] != kBehringerManufacturerId[1] ||
            data[2] != kBehringerManufacturerId[2])
            return result;

        // data[3] = ic (device/channel byte) - not filtered here; do that at
        // the MidiIOManager level if you've set a specific MIDI channel.
        auto apparatus = data[4];
        auto function  = data[5];

        if (apparatus != kApparatusId || function != kFunctionParamChange)
            return result; // not a direct-param-change frame from a DDX3216

        auto nn = data[6];
        auto expectedSize = 7 + 4 * (int) nn;
        if (size < expectedSize)
            return result; // truncated / malformed

        for (int i = 0; i < nn; ++i)
        {
            auto base = 7 + 4 * i;
            uint8_t module = data[base];
            uint8_t param  = data[base + 1];
            uint8_t hi     = data[base + 2];
            uint8_t lo     = data[base + 3];

            int raw = (hi << 7) | lo;
            result.push_back ({ module, param, raw });
        }

        return result;
    }
}
