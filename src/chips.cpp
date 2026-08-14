#include "chips.h"

#include "algfile.h"
#include "logiclist.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

QStringList ChipDatabase::categories()
{
    return {ChipCategory::kFlash, ChipCategory::kMcu, ChipCategory::kPld,
            ChipCategory::kSram,  ChipCategory::kNand, ChipCategory::kEmmc,
            ChipCategory::kVga,   ChipCategory::kLogic};
}

QStringList ChipDatabase::categoryLabels()
{
    return {QStringLiteral("ROM/FLASH/NVRAM"), QStringLiteral("MCU/MPU"),
            QStringLiteral("PLD/GAL/CPLD"),    QStringLiteral("SRAM/NVRAM"),
            QStringLiteral("NAND"),            QStringLiteral("EMMC/EMCP"),
            QStringLiteral("VGA/HDMI"),        QStringLiteral("Logic IC")};
}

ChipDatabase::ChipDatabase()
{
    const QString path = QDir(QCoreApplication::applicationDirPath())
                             .filePath(QStringLiteral("chips.txt"));
    if (!QFile::exists(path)) {
        addBuiltinSamples();
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addBuiltinSamples();
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const QStringList parts = line.split(QLatin1Char('\t'));
        if (parts.size() >= 3 && !parts.at(0).isEmpty() && !parts.at(1).isEmpty())
            add(parts.at(0), parts.at(1), parts.at(2));
    }
}

void ChipDatabase::add(const QString &vendor, const QString &name,
                       const QString &category, const QString &note)
{
    ChipInfo info;
    info.vendor = vendor;
    info.name = name;
    info.category = category;
    info.note = note;
    m_chips.append(info);
}

QVector<ChipInfo> ChipDatabase::matching(const QString &category,
                                         const QString &search, bool exact) const
{
    const bool searchEmpty = search.trimmed().isEmpty();
    const bool allCategories = category.isEmpty() || category == QStringLiteral("All");
    QVector<ChipInfo> out;
    for (const ChipInfo &chip : m_chips) {
        if (!allCategories && chip.category != category)
            continue;
        if (!searchEmpty) {
            if (exact) {
                if (chip.name.compare(search.trimmed(), Qt::CaseInsensitive) != 0)
                    continue;
            } else if (!chip.name.contains(search.trimmed(), Qt::CaseInsensitive)) {
                continue;
            }
        }
        out.append(chip);
    }
    return out;
}

QStringList ChipDatabase::vendorsFor(const QString &category, const QString &search,
                                     bool exact) const
{
    QStringList out;
    const QVector<ChipInfo> chips = matching(category, search, exact);
    for (const ChipInfo &chip : chips)
        if (!out.contains(chip.vendor))
            out.append(chip.vendor);
    out.sort(Qt::CaseInsensitive);
    return out;
}

QString ChipDatabase::categoryFor(const QString &family)
{
    using namespace ChipCategory;
    const QString upper = family.toUpper();

    const QStringList mcuMarkers = {QStringLiteral("ATMEGA"),
                                    QStringLiteral("ATTINY"),
                                    QStringLiteral("ATT2313"),
                                    QStringLiteral("ATTINY"),
                                    QStringLiteral("AT89"),
                                    QStringLiteral("AT90"),
                                    QStringLiteral("AT91"),
                                    QStringLiteral("PIC"),
                                    QStringLiteral("AVR"),
                                    QStringLiteral("STC"),
                                    QStringLiteral("8051"),
                                    QStringLiteral("HT48"),
                                    QStringLiteral("HT66"),
                                    QStringLiteral("HT46"),
                                    QStringLiteral("MSP"),
                                    QStringLiteral("STM8"),
                                    QStringLiteral("STM32"),
                                    QStringLiteral("MEGA")};
    for (const QString &m : mcuMarkers)
        if (upper.contains(m))
            return kMcu;

    const QStringList pldMarkers = {QStringLiteral("GAL"),   QStringLiteral("CPLD"),
                                    QStringLiteral("EPM"),   QStringLiteral("PLD"),
                                    QStringLiteral("ISPLSI"), QStringLiteral("MAX7000"),
                                    QStringLiteral("MACH")};
    for (const QString &m : pldMarkers)
        if (upper.contains(m))
            return kPld;

    if (upper.contains(QStringLiteral("EMMC"))
        || upper.contains(QStringLiteral("EMCP"))
        || upper.startsWith(QStringLiteral("THGB"))
        || upper.startsWith(QStringLiteral("KLM"))
        || upper.startsWith(QStringLiteral("SDIN")))
        return kEmmc;

    if (upper.contains(QStringLiteral("NAND"))
        || upper.startsWith(QStringLiteral("K9F"))
        || upper.startsWith(QStringLiteral("TC58"))
        || upper.startsWith(QStringLiteral("MT29"))
        || upper.startsWith(QStringLiteral("HY27")))
        return kNand;

    const QStringList vgaMarkers = {QStringLiteral("VGA"),  QStringLiteral("HDMI"),
                                    QStringLiteral("RTD"),  QStringLiteral("MST"),
                                    QStringLiteral("NT685"), QStringLiteral("TSUM"),
                                    QStringLiteral("CH703"), QStringLiteral("EP953")};
    for (const QString &m : vgaMarkers)
        if (upper.contains(m))
            return kVga;

    if (upper.contains(QStringLiteral("SRAM"))
        || upper.contains(QStringLiteral("NVRAM"))
        || upper.contains(QStringLiteral("DS12")))
        return kSram;

    return kFlash;
}

int ChipDatabase::loadReferenceData(const QString &referenceDir)
{
    const QDir dir(referenceDir);
    if (!dir.exists())
        return 0;
    m_algorithmDir =
        QDir(dir.filePath(QStringLiteral("algorithm"))).absolutePath();

    int added = 0;
    const QVector<AlgInfo> algs = scanAlgorithms(m_algorithmDir);
    for (const AlgInfo &alg : algs) {
        QString name = alg.file;
        name.chop(QStringLiteral(".alg").size());
        const QString cat = categoryFor(alg.familyName);
        if (!name.isEmpty()) {
            add(alg.familyName, name, cat, alg.file);
            ++added;
        }
    }

    const QStringList logic = scanLogicList(
        dir.filePath(QStringLiteral("Logic.lst")));
    for (const QString &part : logic) {
        add(QStringLiteral("Logic"), part, ChipCategory::kLogic);
        ++added;
    }
    return added;
}

QString ChipDatabase::algorithmFile(const ChipInfo &chip) const
{
    if (m_algorithmDir.isEmpty() || !chip.note.endsWith(QStringLiteral(".alg")))
        return QString();
    const QFileInfo fi(QDir(m_algorithmDir).filePath(chip.note));
    return fi.exists() ? fi.absoluteFilePath() : QString();
}

void ChipDatabase::addBuiltinSamples()
{
    using namespace ChipCategory;

    add(QStringLiteral("AMD"), QStringLiteral("AM27C512"), kFlash);
    add(QStringLiteral("Atmel"), QStringLiteral("AT28C256"), kFlash);
    add(QStringLiteral("Atmel"), QStringLiteral("AT29C040A"), kFlash);
    add(QStringLiteral("Atmel"), QStringLiteral("AT49F040"), kFlash);
    add(QStringLiteral("Atmel"), QStringLiteral("AT25F080A"), kFlash);
    add(QStringLiteral("Atmel"), QStringLiteral("AT45DB041D"), kFlash);
    add(QStringLiteral("Atmel"), QStringLiteral("AT25080"), kFlash);
    add(QStringLiteral("Winbond"), QStringLiteral("W27C512"), kFlash);
    add(QStringLiteral("Winbond"), QStringLiteral("W25Q64"), kFlash);
    add(QStringLiteral("Winbond"), QStringLiteral("W25Q128"), kFlash);
    add(QStringLiteral("Winbond"), QStringLiteral("W39L040"), kFlash);
    add(QStringLiteral("ST"), QStringLiteral("M27C512"), kFlash);
    add(QStringLiteral("ST"), QStringLiteral("M29F010"), kFlash);
    add(QStringLiteral("ST"), QStringLiteral("M25P80"), kFlash);
    add(QStringLiteral("ST"), QStringLiteral("M25PX16"), kFlash);
    add(QStringLiteral("Intel"), QStringLiteral("27C64"), kFlash);
    add(QStringLiteral("Intel"), QStringLiteral("D27C010"), kFlash);
    add(QStringLiteral("SST"), QStringLiteral("39SF010"), kFlash);
    add(QStringLiteral("SST"), QStringLiteral("39VF040"), kFlash);
    add(QStringLiteral("SST"), QStringLiteral("49LF002A"), kFlash);
    add(QStringLiteral("Macronix"), QStringLiteral("MX29LV040C"), kFlash);
    add(QStringLiteral("Microchip"), QStringLiteral("25LC256"), kFlash);
    add(QStringLiteral("Giantec"), QStringLiteral("GT24C16"), kFlash);

    add(QStringLiteral("Atmel"), QStringLiteral("AT89C2051"), kMcu);
    add(QStringLiteral("Atmel"), QStringLiteral("AT89C51"), kMcu);
    add(QStringLiteral("Atmel"), QStringLiteral("AT89S52"), kMcu);
    add(QStringLiteral("Atmel"), QStringLiteral("AT90S2313"), kMcu);
    add(QStringLiteral("Atmel"), QStringLiteral("ATmega8"), kMcu);
    add(QStringLiteral("Atmel"), QStringLiteral("ATmega48"), kMcu);
    add(QStringLiteral("Atmel"), QStringLiteral("ATmega328P"), kMcu);
    add(QStringLiteral("Microchip"), QStringLiteral("PIC12F508"), kMcu);
    add(QStringLiteral("Microchip"), QStringLiteral("PIC16F84A"), kMcu);
    add(QStringLiteral("Microchip"), QStringLiteral("PIC16F877A"), kMcu);
    add(QStringLiteral("Microchip"), QStringLiteral("PIC18F452"), kMcu);
    add(QStringLiteral("STC"), QStringLiteral("STC89C52"), kMcu);
    add(QStringLiteral("STC"), QStringLiteral("STC12C5410AD"), kMcu);
    add(QStringLiteral("NXP"), QStringLiteral("P89C51RD2"), kMcu);
    add(QStringLiteral("Winbond"), QStringLiteral("W79E632"), kMcu);
    add(QStringLiteral("Holtek"), QStringLiteral("HT48R50A"), kMcu);
    add(QStringLiteral("Holtek"), QStringLiteral("HT66F018"), kMcu);
    add(QStringLiteral("Microchip"), QStringLiteral("PIC18F14K50"), kMcu);

    add(QStringLiteral("Lattice"), QStringLiteral("GAL16V8"), kPld);
    add(QStringLiteral("Lattice"), QStringLiteral("GAL22V10"), kPld);
    add(QStringLiteral("Atmel"), QStringLiteral("ATF16V8B"), kPld);
    add(QStringLiteral("Atmel"), QStringLiteral("ATF22V10C"), kPld);
    add(QStringLiteral("Lattice"), QStringLiteral("ispLSI 2032"), kPld);

    add(QStringLiteral("Cypress"), QStringLiteral("CY62256"), kSram);
    add(QStringLiteral("ISSI"), QStringLiteral("IS62C256"), kSram);
    add(QStringLiteral("Dallas"), QStringLiteral("DS1220Y"), kSram);
    add(QStringLiteral("Dallas"), QStringLiteral("DS1230Y"), kSram);
    add(QStringLiteral("ST"), QStringLiteral("M48T58"), kSram);

    add(QStringLiteral("Samsung"), QStringLiteral("K9F5608U0C"), kNand);
    add(QStringLiteral("Samsung"), QStringLiteral("K9F1208U0C"), kNand);
    add(QStringLiteral("Samsung"), QStringLiteral("K9F1G08U0C"), kNand);
    add(QStringLiteral("Toshiba"), QStringLiteral("TC58NVG0S3ETA00"), kNand);
    add(QStringLiteral("Micron"), QStringLiteral("MT29F2G08AAD"), kNand);
    add(QStringLiteral("Hynix"), QStringLiteral("HY27UF081G2A"), kNand);

    add(QStringLiteral("Samsung"), QStringLiteral("KLM8G1GETF-B041"), kEmmc);
    add(QStringLiteral("Toshiba"), QStringLiteral("THGBMJG6C1LBAIL"), kEmmc);
    add(QStringLiteral("SanDisk"), QStringLiteral("SDIN8DE2-8G"), kEmmc);
    add(QStringLiteral("Kingston"), QStringLiteral("EMMC04G-W150"), kEmmc);

    add(QStringLiteral("Realtek"), QStringLiteral("RTD2270"), kVga);
    add(QStringLiteral("Realtek"), QStringLiteral("RTD2120"), kVga);
    add(QStringLiteral("MStar"), QStringLiteral("MST785"), kVga);
    add(QStringLiteral("MStar"), QStringLiteral("MST9E19"), kVga);
    add(QStringLiteral("Novatek"), QStringLiteral("NT68563"), kVga);
    add(QStringLiteral("TSUM"), QStringLiteral("TSUMV59XUS"), kVga);

    add(QStringLiteral("TI"), QStringLiteral("74HC244"), kLogic);
    add(QStringLiteral("TI"), QStringLiteral("74HC245"), kLogic);
    add(QStringLiteral("TI"), QStringLiteral("74HC373"), kLogic);
    add(QStringLiteral("TI"), QStringLiteral("74HC595"), kLogic);
    add(QStringLiteral("Fairchild"), QStringLiteral("CD4021"), kLogic);
    add(QStringLiteral("TI"), QStringLiteral("CD4017"), kLogic);
}
