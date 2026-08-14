#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// Category keys mirror the radio-button groups of the reference "Select
// Device" dialog (dialog 183). "All" is not a stored category but the "所有类型"
// pseudo-group.
namespace ChipCategory {
constexpr auto kFlash = QStringLiteral("Flash");
constexpr auto kMcu = QStringLiteral("MCU");
constexpr auto kPld = QStringLiteral("PLD");
constexpr auto kSram = QStringLiteral("SRAM");
constexpr auto kNand = QStringLiteral("NAND");
constexpr auto kEmmc = QStringLiteral("EMMC");
constexpr auto kVga = QStringLiteral("VGA");
constexpr auto kLogic = QStringLiteral("Logic");
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

private:
    void addBuiltinSamples();
    void add(const QString &vendor, const QString &name, const QString &category,
             const QString &note = QString());

    QVector<ChipInfo> m_chips;
};
