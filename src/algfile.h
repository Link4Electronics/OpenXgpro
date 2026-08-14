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

struct AlgInfo {
    QString file;       // base name, e.g. "AT45D31.alg"
    QString familyName; // 16-byte ASCII family header, e.g. "AT45DB"
    qint64 size = 0;    // file size in bytes
    quint16 magic = 0;  // header magic at offset 0x220 (0x327C observed, LE "7c 32")
    quint16 version = 0; // header sub-field at offset 0x222 (5 observed)
};

// Scans a directory of reference algorithm files ("algorithm/*.alg") and
// extracts the header metadata: 16-byte family name, header magic and file
// size. Returns entries in name order; empty when the directory is missing.
QVector<AlgInfo> scanAlgorithms(const QString &dir);
