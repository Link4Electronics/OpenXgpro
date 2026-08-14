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
#include "algfile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {

// Fixed header layout observed across all reference .alg files.
constexpr qint64 kNameOffset = 0x00;
constexpr int kNameBytes = 16;
constexpr qint64 kHeaderOffset = 0x220;
constexpr quint16 kExpectedVersion = 0x0005;

} // namespace

QVector<AlgInfo> scanAlgorithms(const QString &dir)
{
    QVector<AlgInfo> out;
    const QDir d(dir);
    if (!d.exists())
        return out;

    const QFileInfoList files = d.entryInfoList({QStringLiteral("*.alg")},
                                                QDir::Files | QDir::Readable,
                                                QDir::Name);
    for (const QFileInfo &fi : files) {
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly))
            continue;
        const QByteArray head = f.read(kHeaderOffset + 4);
        if (head.size() < kHeaderOffset + 4)
            continue;

        const quint16 magic = static_cast<quint16>(
            (static_cast<quint8>(head.at(kHeaderOffset + 1)) << 8)
            | static_cast<quint8>(head.at(kHeaderOffset)));
        const quint16 version = static_cast<quint16>(
            (static_cast<quint8>(head.at(kHeaderOffset + 3)) << 8)
            | static_cast<quint8>(head.at(kHeaderOffset + 2)));
        // The magic varies by algorithm family (0x327C/0x3394/0x34A8/0x35BC
        // observed); only the version is common.
        if (magic == 0 || version != kExpectedVersion)
            continue;

        const int nul = head.indexOf('\0', kNameOffset);
        const int len = (nul < 0) ? kNameBytes : (nul - kNameOffset);
        const QString name = QString::fromLatin1(
            head.constData() + kNameOffset,
            (len > kNameBytes) ? kNameBytes : len).trimmed();
        if (name.isEmpty())
            continue;

        AlgInfo info;
        info.file = fi.fileName();
        info.familyName = name;
        info.size = fi.size();
        info.magic = magic;
        info.version = version;
        out.append(info);
    }
    return out;
}
