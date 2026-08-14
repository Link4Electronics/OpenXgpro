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
