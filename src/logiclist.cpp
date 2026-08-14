#include "logiclist.h"

#include <QFile>

#include <algorithm>

namespace {

constexpr qint64 kTableOffset = 0x10;

quint32 le32(const uchar *p)
{
    return quint32(p[0]) | (quint32(p[1]) << 8)
           | (quint32(p[2]) << 16) | (quint32(p[3]) << 24);
}

} // namespace

QStringList scanLogicList(const QString &path)
{
    QStringList out;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return out;
    const QByteArray data = f.readAll();

    // The offset table runs from the table start until the first entry, whose
    // value is the byte offset of the first name.
    if (data.size() <= int(kTableOffset + 4))
        return out;
    const quint32 first = le32(reinterpret_cast<const uchar *>(
        data.constData() + kTableOffset));
    if (first <= kTableOffset || first > quint32(data.size()))
        return out;
    const qint64 tableEnd = qMin<qint64>(data.size(), first);

    QStringList seen;
    for (qint64 off = kTableOffset; off + 4 <= tableEnd; off += 4) {
        const quint32 pos = le32(reinterpret_cast<const uchar *>(
            data.constData() + off));
        // Each record: 4-byte header, then the null-terminated part name.
        const qint64 namePos = qint64(pos) + 4;
        if (namePos >= data.size())
            continue;
        const char *start = data.constData() + namePos;
        const char *end = static_cast<const char *>(
            memchr(start, '\0', data.size() - namePos));
        if (!end)
            continue;
        const QString name = QString::fromLatin1(start, end - start).trimmed();
        if (!name.isEmpty() && !seen.contains(name)) {
            seen.append(name);
            out.append(name);
        }
    }
    return out;
}
