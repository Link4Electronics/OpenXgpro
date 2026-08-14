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
