#pragma once

#include <QStringList>

// Parses the reference "Logic.lst" file: a DWORD offset table at offset 0x10
// points into a block of null-terminated ASCII part names (e.g. "4000",
// "4001" ...). Returns the part names in table order, deduplicated; empty
// when the file is missing or malformed.
QStringList scanLogicList(const QString &path);
