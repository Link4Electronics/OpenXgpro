#include "programmer.h"

namespace Programmer {

QVector<QString> modelNames()
{
    return {QStringLiteral("XGecu T56"), QStringLiteral("XGecu T48  (TL866-3G)"),
            QStringLiteral("TL866-II Plus")};
}

QString modelName(ProgrammerModel model)
{
    const auto names = modelNames();
    return names.at(static_cast<int>(model));
}

ProgrammerModel modelFromName(const QString &name)
{
    const auto names = modelNames();
    for (int i = 0; i < names.size(); ++i) {
        if (names.at(i) == name)
            return static_cast<ProgrammerModel>(i);
    }
    return ProgrammerModel::T56;
}

} // namespace Programmer
