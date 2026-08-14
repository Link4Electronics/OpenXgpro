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
