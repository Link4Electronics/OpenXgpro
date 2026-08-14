#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// Category keys mirror the radio-button groups of the reference "Select
// Device" dialog (dialog 183). "All" is not a stored category but the
// "All types" pseudo-group.
namespace ChipCategory {
inline const QString kFlash = QStringLiteral("Flash");
inline const QString kMcu = QStringLiteral("MCU");
inline const QString kPld = QStringLiteral("PLD");
inline const QString kSram = QStringLiteral("SRAM");
inline const QString kNand = QStringLiteral("NAND");
inline const QString kEmmc = QStringLiteral("EMMC");
inline const QString kVga = QStringLiteral("VGA");
inline const QString kLogic = QStringLiteral("Logic");
} // namespace ChipCategory

struct ChipInfo {
    QString vendor;
    QString name;
    QString category;
    QString note;
};

// Chips are loaded from "<appdir>/chips.txt" when present (tab separated:
// vendor<tab>name<tab>category), otherwise a small built-in sample set is used
// so the UI works out of the box. A real database can be extracted from the
// reference distribution later.
class ChipDatabase
{
public:
    ChipDatabase();

    static QStringList categories();
    static QStringList categoryLabels();

    const QVector<ChipInfo> &all() const { return m_chips; }

    // Chips whose category is "All" or matches `category`, and whose name
    // matches `search`. Empty search matches everything; `exact` requires a
    // full (case-insensitive) match, otherwise a substring match.
    QVector<ChipInfo> matching(const QString &category, const QString &search,
                               bool exact) const;

    // Vendors having at least one chip in `matching(category, search, exact)`.
    QStringList vendorsFor(const QString &category, const QString &search,
                           bool exact) const;

    // Loads real device families from the reference distribution: algorithm
    // headers from "<referenceDir>/algorithm/*.alg" and logic parts from
    // "<referenceDir>/Logic.lst". Existing entries are kept. Returns the
    // number of chips added (0 when the directory is missing).
    int loadReferenceData(const QString &referenceDir);

    // Directory holding the reference "algorithm/*.alg" files, or empty when
    // no reference data was loaded.
    QString algorithmDir() const { return m_algorithmDir; }

    // The .alg file backing `chip` (full path), or empty when the chip has no
    // algorithm (built-in sample) or the file is missing.
    QString algorithmFile(const ChipInfo &chip) const;

private:
    void addBuiltinSamples();
    void add(const QString &vendor, const QString &name, const QString &category,
             const QString &note = QString());
    static QString categoryFor(const QString &family);

    QVector<ChipInfo> m_chips;
    QString m_algorithmDir;
};
