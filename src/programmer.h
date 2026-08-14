#pragma once

#include <QString>
#include <QVector>

enum class ProgrammerModel {
    T56,
    T48,
    TL866_II_Plus,
};

namespace Programmer {
// Names exactly as shown in the reference "Programmer not found" dialog (211).
QVector<QString> modelNames();
QString modelName(ProgrammerModel model);
ProgrammerModel modelFromName(const QString &name);
} // namespace Programmer
