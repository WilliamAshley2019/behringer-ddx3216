#pragma once
#include <optional>
#include <JuceHeader.h>
#include "DDX3216Protocol.h"

#if JUCE_WINDOWS
 #include <windows.h>
#endif

/*
    RS232 transport for the DDX3216.

    Connection settings (115200 baud, no parity, 8 data bits, 1 stop bit) are
    NOT a guess -- they're read directly out of Behringer's own "DDX3216 File
    Exchange 1.1f.exe" tool, which builds its port config from the literal
    string "COM0:115200,n,8,1" (found via a strings scan + confirmed against
    its BuildCommDCBA call). See DDX3216Protocol.h's Serial namespace.

    The desk frames RS232 traffic the same way as MIDI SysEx (F0 ... F7),
    just without the MIDI transport wrapper -- so this class reuses
    DDX3216::buildSingleParamChange / parseIncomingSysEx directly rather than
    re-implementing framing. Only the byte-level transport (open a COM port,
    read/write raw bytes, find F0...F7 boundaries in the incoming stream)
    is new here.

    Windows-only for now (Win32 COM-port API). If cross-platform RS232 ever
    matters, swap this class's guts for a library like libserialport and
    keep the same public interface.
*/
class SerialPortManager : private juce::Thread
{
public:
    struct RawMessageListener
    {
        virtual ~RawMessageListener() = default;
        virtual void serialMessageLogged (const juce::MidiMessage& message, bool outgoing) {}
        virtual void serialStatusChanged (const juce::String& status) {}
    };

    SerialPortManager() : juce::Thread ("DDX3216 Serial Reader") {}
    ~SerialPortManager() override { close(); }

    // e.g. "COM3". On Windows, ports above COM9 need the "\\\\.\\COM10" form --
    // this handles that automatically.
    bool open (const juce::String& portName,
               int baudRate = DDX3216::Serial::kBaudRate)
    {
        close();

#if JUCE_WINDOWS
        auto path = portName.startsWith ("COM") && portName.substring (3).getIntValue() >= 10
                        ? "\\\\.\\" + portName
                        : portName;

        handle = CreateFileA (path.toRawUTF8(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                               OPEN_EXISTING, 0, nullptr);

        if (handle == INVALID_HANDLE_VALUE)
        {
            notifyStatus ("Failed to open " + portName);
            return false;
        }

        DCB dcb {};
        dcb.DCBlength = sizeof (DCB);
        if (! GetCommState (handle, &dcb))
        {
            notifyStatus ("GetCommState failed on " + portName);
            close();
            return false;
        }

        dcb.BaudRate = (DWORD) baudRate;
        dcb.ByteSize = (BYTE) DDX3216::Serial::kDataBits;
        dcb.Parity   = NOPARITY;   // matches confirmed "n" from the vendor tool
        dcb.StopBits = ONESTOPBIT; // matches confirmed "1"

        if (! SetCommState (handle, &dcb))
        {
            notifyStatus ("SetCommState failed on " + portName);
            close();
            return false;
        }

        COMMTIMEOUTS timeouts {};
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;
        SetCommTimeouts (handle, &timeouts);

        notifyStatus ("Connected to " + portName + " @ " + juce::String (baudRate));
        startThread();
        return true;
#else
        juce::ignoreUnused (portName, baudRate);
        notifyStatus ("SerialPortManager is Windows-only in this build");
        return false;
#endif
    }

    void close()
    {
        stopThread (1000);
#if JUCE_WINDOWS
        if (handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle (handle);
            handle = INVALID_HANDLE_VALUE;
        }
#endif
    }

    bool isOpen() const
    {
#if JUCE_WINDOWS
        return handle != INVALID_HANDLE_VALUE;
#else
        return false;
#endif
    }

    // Sends a pre-built DDX3216 SysEx MidiMessage's raw bytes (F0...F7)
    // straight over the serial port -- same frame content as the MIDI path,
    // different wire.
    void send (const juce::MidiMessage& frame)
    {
#if JUCE_WINDOWS
        if (handle == INVALID_HANDLE_VALUE)
            return;

        DWORD written = 0;
        WriteFile (handle, frame.getRawData(), (DWORD) frame.getRawDataSize(), &written, nullptr);
#endif
        rawListeners.call ([&frame] (RawMessageListener& l) { l.serialMessageLogged (frame, true); });
    }

    void addRawMessageListener (RawMessageListener* l)    { rawListeners.add (l); }
    void removeRawMessageListener (RawMessageListener* l) { rawListeners.remove (l); }

    // Best-effort port enumeration. Windows doesn't make this trivial without
    // WMI or SetupAPI; querying COM1-COM32 via CreateFile is a common,
    // simple-enough approach for a first pass.
    static juce::StringArray getAvailablePorts()
    {
        juce::StringArray result;
#if JUCE_WINDOWS
        for (int i = 1; i <= 32; ++i)
        {
            auto name = "COM" + juce::String (i);
            auto path = i >= 10 ? "\\\\.\\" + name : name;
            auto h = CreateFileA (path.toRawUTF8(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                   OPEN_EXISTING, 0, nullptr);
            if (h != INVALID_HANDLE_VALUE)
            {
                CloseHandle (h);
                result.add (name);
            }
            else if (GetLastError() == ERROR_ACCESS_DENIED)
            {
                result.add (name + " (in use)");
            }
        }
#endif
        return result;
    }

private:
    void notifyStatus (const juce::String& s)
    {
        rawListeners.call ([&s] (RawMessageListener& l) { l.serialStatusChanged (s); });
    }

    void run() override
    {
#if JUCE_WINDOWS
        std::vector<uint8_t> buffer;
        uint8_t byte;

        while (! threadShouldExit())
        {
            DWORD bytesRead = 0;
            if (ReadFile (handle, &byte, 1, &bytesRead, nullptr) && bytesRead == 1)
            {
                if (byte == 0xF0)
                    buffer.clear();

                buffer.push_back (byte);

                if (byte == 0xF7 && ! buffer.empty() && buffer.front() == 0xF0)
                {
                    auto message = juce::MidiMessage (buffer.data(), (int) buffer.size());
                    rawListeners.call ([&message] (RawMessageListener& l) { l.serialMessageLogged (message, false); });
                    buffer.clear();
                }
            }
        }
#endif
    }

#if JUCE_WINDOWS
    HANDLE handle = INVALID_HANDLE_VALUE;
#endif
    juce::ListenerList<RawMessageListener> rawListeners;
};
