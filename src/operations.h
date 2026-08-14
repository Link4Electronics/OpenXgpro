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
#pragma once

#include "programmer.h"

#include <QByteArray>
#include <QString>

enum class OpType {
    Read,
    Program,
    Verify,
    Erase,
    BlankCheck,
    ChipId,
    FlashIdentify,
};

QString opName(OpType op);

struct OpResult {
    bool ok = false;
    bool completed = false; // true = actually executed; false = validated/pending
    QString message;
    QByteArray data;        // Read / Chip ID results
};

// Runs a chip operation against the connected programmer. When no programmer
// is present it returns a clear error (demo mode, like the reference); with a
// programmer connected but the low-level protocol not yet implemented it
// reports the op as pending. Validation (empty buffer, no chip selected,
// missing algorithm) is always performed. `algorithmFile` is the resolved
// path of the chip's .alg file (see ChipDatabase::algorithmFile); when empty
// the chip has no algorithm and the op cannot run.
OpResult runOperation(OpType op, ProgrammerModel model, const QString &chipName,
                      const QByteArray &buffer,
                      const QString &algorithmFile = QString());
