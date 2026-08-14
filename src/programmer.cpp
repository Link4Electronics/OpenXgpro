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
