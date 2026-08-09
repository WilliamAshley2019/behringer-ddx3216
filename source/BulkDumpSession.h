#pragma once
#include <JuceHeader.h>
#include "DDX3216Protocol.h"

/*
    Drives one settings/library/snapshot/firmware transfer to or from the
    DDX3216, using the block protocol documented in DDX3216Protocol.h's
    BulkDump namespace. Transport-agnostic: you give it a "send this frame"
    callback (wire it to MidiIOManager::sendFrame-equivalent or
    SerialPortManager::send) and feed it incoming frames yourself (from
    MidiIOManager::RawMessageListener / SerialPortManager::RawMessageListener).
    It doesn't know or care whether it's running over MIDI or RS232.

    NOT YET WIRED INTO THE GUI. This is the protocol-level engine; a
    MainComponent panel to drive it (pick a category, show progress, save/
    load a file) is the next visible step once this has been tested against
    real hardware.

    Retry policy: on a checksum failure, per the PDF's own protocol
    description, the correct response is to re-request the SAME block
    (download) or expect the desk to re-request the SAME block (upload).
    This class does that automatically up to maxRetriesPerBlock times before
    giving up and reporting an error.
*/
class BulkDumpSession : private juce::Timer
{
public:
    enum class Direction { download, upload };

    enum class Status { idle, running, succeeded, failed };

    struct Result
    {
        Status status = Status::idle;
        juce::String message; // human-readable status/error, for the log pane
        float progress = 0.0f; // 0..1
    };

    using SendFrameFn = std::function<void (const juce::MidiMessage&)>;
    using ProgressCallback = std::function<void (const Result&)>;
    using CompletionCallback = std::function<void (bool success, std::vector<uint8_t> data)>;

    explicit BulkDumpSession (SendFrameFn sendFrame) : send (std::move (sendFrame)) {}

    // ---- Download: request a file/settings blob FROM the desk ----
    // requestFunction is one of DDX3216::BulkDump::kFuncRequest*.
    // whatByte selects the file category (F_ALL/F_EQL/etc.) -- see
    // DDX3216::BulkDump::WhatByte. Its real numeric values aren't confirmed
    // yet, so callers currently have to supply a best guess.
    void startDownload (uint8_t requestFunction, uint8_t whatByte, ProgressCallback onProgress, CompletionCallback onComplete)
    {
        reset();
        direction = Direction::download;
        function = requestFunction;
        whatByte_ = whatByte;
        progressCb = std::move (onProgress);
        completeCb = std::move (onComplete);
        status = Status::running;
        requestBlock (0);
        startTimer (kTimeoutMs);
    }

    // ---- Upload: send a file/settings blob TO the desk ----
    // dumpFunction is one of DDX3216::BulkDump::kFuncDump*.
    void startUpload (uint8_t dumpFunction, uint8_t whatByte, std::vector<uint8_t> dataToSend,
                       ProgressCallback onProgress, CompletionCallback onComplete)
    {
        reset();
        direction = Direction::upload;
        function = dumpFunction;
        whatByte_ = whatByte;
        payload = std::move (dataToSend);
        progressCb = std::move (onProgress);
        completeCb = std::move (onComplete);
        status = Status::running;

        totalBlocks = (int) ((payload.size() + DDX3216::BulkDump::kBlockPayloadSize - 1)
                              / DDX3216::BulkDump::kBlockPayloadSize);
        if (totalBlocks == 0)
            totalBlocks = 1; // still send one (possibly empty) block

        sendBlock (0);
        startTimer (kTimeoutMs);
    }

    void cancel()
    {
        stopTimer();
        status = Status::idle;
    }

    // Feed every incoming SysEx message here (from your MIDI/RS232 listener)
    // while a session is running. Frames that aren't part of this transfer
    // are ignored, so it's safe to feed it everything.
    void handleIncomingFrame (const juce::MidiMessage& message)
    {
        if (status != Status::running)
            return;

        if (direction == Direction::download)
            handleIncomingForDownload (message);
        else
            handleIncomingForUpload (message);
    }

private:
    static constexpr int kTimeoutMs = 3000;
    static constexpr int kMaxRetriesPerBlock = 3;

    void reset()
    {
        stopTimer();
        status = Status::idle;
        currentBlock = 0;
        retriesThisBlock = 0;
        totalBlocks = 0;
        payload.clear();
        assembled.clear();
    }

    void requestBlock (int blockIndex)
    {
        currentBlock = blockIndex;
        send (DDX3216::BulkDump::buildRequestBlock (function, whatByte_, blockIndex));
        reportProgress ("Requesting block " + juce::String (blockIndex));
        startTimer (kTimeoutMs); // reset the timeout on every request
    }

    void sendBlock (int blockIndex)
    {
        currentBlock = blockIndex;
        auto offset = (size_t) blockIndex * (size_t) DDX3216::BulkDump::kBlockPayloadSize;
        auto remaining = payload.size() > offset ? payload.size() - offset : (size_t) 0;
        auto chunkSize = std::min<size_t> ((size_t) DDX3216::BulkDump::kBlockPayloadSize, remaining);

        std::vector<uint8_t> chunk (payload.begin() + (long) offset,
                                     payload.begin() + (long) (offset + chunkSize));

        send (DDX3216::BulkDump::buildDataBlock (function, whatByte_, 1, totalBlocks, blockIndex, chunk));
        reportProgress ("Sending block " + juce::String (blockIndex) + " of " + juce::String (totalBlocks));
        startTimer (kTimeoutMs);
    }

    void handleIncomingForDownload (const juce::MidiMessage& message)
    {
        auto block = DDX3216::BulkDump::parseDataBlock (message);
        if (! block.has_value() || block->blockIndex != currentBlock)
            return; // not the block we're waiting for -- ignore

        if (! block->checksumOk)
        {
            retryOrFail ("Checksum error on block " + juce::String (currentBlock));
            return;
        }

        retriesThisBlock = 0;
        assembled.insert (assembled.end(), block->decodedPayload.begin(), block->decodedPayload.end());
        totalBlocks = block->totalBlocks;

        reportProgress ("Received block " + juce::String (currentBlock) + " of " + juce::String (totalBlocks));

        if (currentBlock + 1 >= totalBlocks)
            finish (true, "Download complete (" + juce::String ((int) assembled.size()) + " bytes)");
        else
            requestBlock (currentBlock + 1);
    }

    void handleIncomingForUpload (const juce::MidiMessage& message)
    {
        // The desk ACKs by requesting the next block; a repeated request for
        // the SAME block we just sent means it wants a resend (checksum
        // failure or similar on its end).
        if (! message.isSysEx())
            return;

        auto* d = message.getSysExData();
        auto size = message.getSysExDataSize();
        if (size < 9) return;
        if (d[0] != DDX3216::kBehringerManufacturerId[0] || d[1] != DDX3216::kBehringerManufacturerId[1]
            || d[2] != DDX3216::kBehringerManufacturerId[2] || d[4] != DDX3216::kApparatusId)
            return;

        auto func = d[5];
        // request-function counterpart of whichever dump function we're using
        // (0x50<->0x10, 0x51<->0x11, 0x52<->0x12 -- all -0x40, so one line
        // covers all three).
        if (func != (uint8_t) (function - 0x40))
            return;
        // NOTE: not checking d[6] (whatByte) matches whatByte_ here -- the
        // desk's ACK should echo it back, but tolerating a mismatch for now
        // in case that assumption turns out wrong once tested on hardware.

        auto requestedBlock = (d[7] << 7) | d[8];

        if (requestedBlock == currentBlock)
        {
            retryOrFail ("Desk requested a resend of block " + juce::String (currentBlock));
            return;
        }

        retriesThisBlock = 0;

        if (requestedBlock >= totalBlocks)
        {
            finish (true, "Upload complete (" + juce::String (totalBlocks) + " blocks)");
            return;
        }

        sendBlock (requestedBlock);
    }

    void retryOrFail (const juce::String& reason)
    {
        if (++retriesThisBlock > kMaxRetriesPerBlock)
        {
            finish (false, reason + " -- giving up after " + juce::String (kMaxRetriesPerBlock) + " retries");
            return;
        }

        reportProgress (reason + " -- retrying (" + juce::String (retriesThisBlock) + "/"
                         + juce::String (kMaxRetriesPerBlock) + ")");

        if (direction == Direction::download)
            requestBlock (currentBlock);
        else
            sendBlock (currentBlock);
    }

    void timerCallback() override
    {
        retryOrFail ("Timed out waiting for block " + juce::String (currentBlock));
    }

    void reportProgress (const juce::String& msg)
    {
        if (progressCb)
        {
            Result r;
            r.status = status;
            r.message = msg;
            r.progress = totalBlocks > 0 ? (float) currentBlock / (float) totalBlocks : 0.0f;
            progressCb (r);
        }
    }

    void finish (bool success, const juce::String& msg)
    {
        stopTimer();
        status = success ? Status::succeeded : Status::failed;
        reportProgress (msg);
        if (completeCb)
            completeCb (success, direction == Direction::download ? assembled : payload);
    }

    SendFrameFn send;
    Direction direction = Direction::download;
    uint8_t function = 0;
    uint8_t whatByte_ = 0;
    Status status = Status::idle;

    int currentBlock = 0;
    int totalBlocks = 0;
    int retriesThisBlock = 0;

    std::vector<uint8_t> payload;   // upload: data being sent
    std::vector<uint8_t> assembled; // download: data being received

    ProgressCallback progressCb;
    CompletionCallback completeCb;
};
