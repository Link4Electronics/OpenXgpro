#include "device.h"

#include <libusb-1.0/libusb.h>

#include <QByteArray>

namespace {
constexpr quint16 kXgproVid = 0xA466;
constexpr quint16 kXgproPid = 0x0A53;

int modelFromDeviceType(unsigned char type)
{
    switch (type) {
    case 5:
        return static_cast<int>(ProgrammerModel::TL866_II_Plus);
    case 6:
        return static_cast<int>(ProgrammerModel::T56);
    case 7:
        return static_cast<int>(ProgrammerModel::T48);
    default:
        return -1;
    }
}

// Reference handshake: write an 8-byte command on pipe 0x01, read up to 64
// bytes on 0x81; device type is response[0x0a] (5=TL866II, 6=T56, 7=T48).
// Returns the device type, or -1 on failure.
int handshakeType(libusb_device_handle *handle)
{
    const unsigned char cmd[8] = {0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00};
    int transferred = 0;
    int rc = libusb_bulk_transfer(handle, 0x01, const_cast<unsigned char *>(cmd),
                                  sizeof(cmd), &transferred, 500);
    if (rc != LIBUSB_SUCCESS)
        return -1;

    unsigned char buf[64] = {0};
    rc = libusb_bulk_transfer(handle, 0x81, buf, sizeof(buf), &transferred, 500);
    if (rc != LIBUSB_SUCCESS || transferred < 0x0b)
        return -1;
    return modelFromDeviceType(buf[0x0a]);
}
} // namespace

DeviceManager::DeviceManager() = default;

DeviceManager::~DeviceManager()
{
    libusb_exit(nullptr);
}

bool DeviceManager::detect()
{
    m_present = false;
    m_error.clear();

    int rc = libusb_init(nullptr);
    if (rc != LIBUSB_SUCCESS) {
        m_error = QStringLiteral("libusb init failed (%1)").arg(rc);
        return false;
    }

    libusb_device **devices = nullptr;
    const ssize_t count = libusb_get_device_list(nullptr, &devices);
    if (count < 0) {
        m_error = QStringLiteral("libusb_get_device_list failed");
        libusb_exit(nullptr);
        return false;
    }

    for (ssize_t i = 0; i < count; ++i) {
        libusb_device *device = devices[i];
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(device, &desc) != LIBUSB_SUCCESS)
            continue;
        if (desc.idVendor != kXgproVid || desc.idProduct != kXgproPid)
            continue;

        libusb_device_handle *handle = nullptr;
        if (libusb_open(device, &handle) != LIBUSB_SUCCESS) {
            m_error = QStringLiteral("Programmer found but could not be opened (permissions?)");
            break;
        }

        const int type = handshakeType(handle);
        libusb_close(handle);
        if (type >= 0) {
            m_model = static_cast<ProgrammerModel>(type);
            m_present = true;
        } else {
            m_error = QStringLiteral("Programmer handshake failed");
        }
        break;
    }

    libusb_free_device_list(devices, 1);
    libusb_exit(nullptr);
    return m_present;
}
