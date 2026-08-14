#include "operations.h"

#include "device.h"

#include <QObject>

QString opName(OpType op)
{
    switch (op) {
    case OpType::Read:
        return QObject::tr("Read");
    case OpType::Program:
        return QObject::tr("Program");
    case OpType::Verify:
        return QObject::tr("Verify");
    case OpType::Erase:
        return QObject::tr("Erase");
    case OpType::BlankCheck:
        return QObject::tr("Blank check");
    case OpType::ChipId:
        return QObject::tr("Chip ID");
    case OpType::FlashIdentify:
        return QObject::tr("25 Flash Identify");
    }
    return QStringLiteral("Operation");
}

OpResult runOperation(OpType op, ProgrammerModel model, const QString &chipName,
                      const QByteArray &buffer)
{
    const QString opText = opName(op);

    // Validation that does not require hardware.
    if (chipName.isEmpty()) {
        return {false, false,
                QObject::tr("No chip selected — use Chip Select first."), {}};
    }
    if ((op == OpType::Program || op == OpType::Verify) && buffer.isEmpty()) {
        return {false, false,
                QObject::tr("Buffer is empty. Load a file before %1.").arg(opText),
                {}};
    }

    // Device detection.
    DeviceManager device;
    if (!device.detect()) {
        const QString why =
            device.lastError().isEmpty()
                ? QObject::tr("no programmer connected")
                : device.lastError();
        return {false, false,
                QObject::tr("%1 cannot run %2: %3.")
                    .arg(Programmer::modelName(model), opText, why),
                {}};
    }

    // Programmer present: the low-level read/program protocol is still being
    // implemented, so report the op as pending rather than pretending.
    return {false, false,
            QObject::tr("%1 on %2 requires the programmer protocol, which is "
                        "not implemented yet.")
                .arg(opText, chipName),
            {}};
}
