// Copyright (C) 2026 OpenXgpro contributors
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program. If not, see <https://www.gnu.org/licenses/>.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "operations.h"

#include "device.h"

#include <QFileInfo>
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
                      const QByteArray &buffer, const QString &algorithmFile)
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
    if (algorithmFile.isEmpty()) {
        return {false, false,
                QObject::tr("No algorithm file is associated with %1. "
                            "Load the reference data or pick a reference chip.")
                    .arg(chipName),
                {}};
    }
    if (!QFileInfo::exists(algorithmFile)) {
        return {false, false,
                QObject::tr("Algorithm file for %1 is missing: %2")
                    .arg(chipName, algorithmFile),
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
                        "not implemented yet (algorithm %3).")
                .arg(opText, chipName, QFileInfo(algorithmFile).fileName()),
            {}};
}
