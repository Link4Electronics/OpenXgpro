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

#include <QStringList>

// Parses the reference "Logic.lst" file: a DWORD offset table at offset 0x10
// points into a block of null-terminated ASCII part names (e.g. "4000",
// "4001" ...). Returns the part names in table order, deduplicated; empty
// when the file is missing or malformed.
QStringList scanLogicList(const QString &path);
