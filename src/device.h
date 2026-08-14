#pragma once

#include "programmer.h"

#include <QString>

// Device detection and connection state. Uses libusb to find the programmer
// (VID:PID 0xA466:0x0A53) and, when present, performs the reference
// handshake so the connected model (TL866II / T56 / T48) is reported.
//
// NOTE: the full Read/Program/Erase protocol is not yet implemented; the
// operations layer reports a clear error for in-progress work while device
// detection itself is real.
class DeviceManager
{
public:
    DeviceManager();
    ~DeviceManager();

    DeviceManager(const DeviceManager &) = delete;
    DeviceManager &operator=(const DeviceManager &) = delete;

    // Re-scan the bus. Returns true when a supported programmer is present.
    bool detect();

    bool isPresent() const { return m_present; }
    ProgrammerModel connectedModel() const { return m_model; }
    QString lastError() const { return m_error; }

private:
    bool m_present = false;
    ProgrammerModel m_model = ProgrammerModel::T56;
    QString m_error;
};
