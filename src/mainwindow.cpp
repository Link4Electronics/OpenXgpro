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
#include "mainwindow.h"

#include "blockfilldialog.h"
#include "chipdialog.h"
#include "device.h"
#include "hexview.h"
#include "programmerdialog.h"
#include "theme.h"
#include "version.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextStream>
#include <QToolButton>
#include <QTranslator>
#include <QVBoxLayout>
#include <QDateTime>

#include <algorithm>

namespace {
// Standard CRC-32 (IEEE 802.3), used for the Buffer/File checksum shown in the
// "Checksum:" info line (mirrors the reference's CRC32 of the Buffer/File).
quint32 crc32(const QByteArray &data)
{
    quint32 crc = 0xFFFFFFFF;
    for (int i = 0; i < data.size(); ++i) {
        crc ^= static_cast<quint8>(data.at(i));
        for (int b = 0; b < 8; ++b)
            crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1));
    }
    return ~crc;
}

int addressDigits(quint64 bytes)
{
    int digits = 6;
    if (bytes > 1) {
        quint64 v = bytes - 1;
        int d = 0;
        while (v) {
            v >>= 4;
            ++d;
        }
        digits = std::max(digits, d);
    }
    return digits;
}

// Minimal Intel HEX (Intel-8X, .hex) loader supporting record types 00, 01,
// 02 (segment) and 04 (linear) with checksum validation.
QByteArray parseIntelHex(const QByteArray &raw, bool *ok)
{
    QByteArray out;
    quint32 base = 0;
    *ok = true;

    const QList<QByteArray> lines = raw.split('\n');
    for (QByteArray line : lines) {
        line = line.trimmed();
        if (line.isEmpty())
            continue;
        if (!line.startsWith(':')) {
            *ok = false;
            break;
        }
        const QByteArray hex = line.mid(1);
        if (hex.size() < 11 || (hex.size() % 2) != 0) {
            *ok = false;
            break;
        }
        bool conv = false;
        const QByteArray bytes = QByteArray::fromHex(hex);
        quint8 sum = 0;
        for (int i = 0; i < bytes.size(); ++i)
            sum += static_cast<quint8>(bytes.at(i));
        if (sum != 0) {
            *ok = false;
            break;
        }
        Q_UNUSED(conv);

        const int count = bytes.at(0);
        const int type = bytes.at(3);
        const quint16 addr = (static_cast<quint16>(bytes.at(1)) << 8)
                             | static_cast<quint16>(bytes.at(2));
        if (type == 0) { // data
            const quint32 target = base + addr;
            if (target + count > static_cast<quint32>(out.size()))
                out.resize(target + count);
            for (int i = 0; i < count; ++i)
                out[target + i] = bytes.at(4 + i);
        } else if (type == 1) { // EOF
            break;
        } else if (type == 2) { // extended segment address
            base = (static_cast<quint32>(bytes.at(4)) << 8
                    | static_cast<quint32>(bytes.at(5))) << 4;
        } else if (type == 4) { // extended linear address
            base = (static_cast<quint32>(bytes.at(4)) << 8
                    | static_cast<quint32>(bytes.at(5))) << 16;
        }
    }
    return out;
}

// Locates the reference Xgpro distribution holding "algorithm/*.alg" and
// "Logic.lst". Candidates: $OPENXGPRO_REFERENCE, then common relative paths
// and the home directory.
QString findReferenceDir()
{
    if (qEnvironmentVariableIsSet("OPENXGPRO_REFERENCE")) {
        const QString env = qEnvironmentVariable("OPENXGPRO_REFERENCE");
        if (QDir(env).exists())
            return env;
    }
    const QStringList candidates = {
        QStringLiteral("XgproV1316"),
        QStringLiteral("../XgproV1316"),
        QStringLiteral("../../XgproV1316"),
    };
    for (const QString &c : candidates) {
        const QDir dir(QDir::current().filePath(c));
        if (dir.exists())
            return dir.absolutePath();
    }
    const QDir home(QDir::home().filePath(QStringLiteral("XgproV1316")));
    if (home.exists())
        return home.absolutePath();
    return QString();
}

// Persisted UI language preference ("system" to follow the system locale).
QString savedLanguage()
{
    QSettings settings;
    return settings.value(QStringLiteral("ui/language"),
                          QStringLiteral("system")).toString();
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("%1 %2").arg(QLatin1String(APP_NAME),
                                               QLatin1String(APP_VERSION)));

    m_translator = new QTranslator(this);
    applyLanguage(savedLanguage(), false);

    const QString referenceDir = findReferenceDir();
    if (!referenceDir.isEmpty())
        m_chips.loadReferenceData(referenceDir);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(6, 6, 6, 6);
    rootLayout->setSpacing(6);

    auto *topLayout = new QHBoxLayout;
    topLayout->addWidget(buildDeviceGroup(), 5);
    topLayout->addWidget(buildChipInfoGroup(), 4);

    auto *centerLayout = new QHBoxLayout;
    centerLayout->addWidget(buildChipListArea(), 1);

    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(buildProgramSettingsGroup(), 1);
    bottomLayout->addWidget(buildChipConfigGroup(), 1);

    rootLayout->addLayout(topLayout);
    rootLayout->addLayout(centerLayout, 1);
    rootLayout->addLayout(bottomLayout);

    setCentralWidget(central);

    buildMenus();
    statusBar()->showMessage(tr("No device connected"));

    restoreSettings();
    loadLastChip();

    QSettings settings;
    m_model = Programmer::modelFromName(
        settings.value(QStringLiteral("programmer/model"),
                       Programmer::modelName(ProgrammerModel::T56))
            .toString());

    DeviceManager device;
    if (device.detect()) {
        m_deviceConnected = true;
        m_model = device.connectedModel();
    }
    settings.setValue(QStringLiteral("programmer/model"),
                      Programmer::modelName(m_model));
    refreshDeviceStatus();
    m_uiBuilt = true;
}

QWidget *MainWindow::buildDeviceGroup()
{
    auto *box = new QGroupBox(tr("Device"), this);
    auto *lay = new QGridLayout(box);
    lay->setHorizontalSpacing(6);
    lay->setVerticalSpacing(4);

    auto *findChipButton = new QPushButton(tr("Find &Select Chip..."), box);
    connect(findChipButton, &QPushButton::clicked, this, &MainWindow::showChipDialog);
    m_findChipButton = findChipButton;

    m_chipCombo = new QComboBox(box);
    m_chipCombo->setEditable(true);
    m_chipCombo->setInsertPolicy(QComboBox::NoInsert);
    connect(m_chipCombo, &QComboBox::activated, this, &MainWindow::onChipComboEdited);
    connect(m_chipCombo->lineEdit(), &QLineEdit::editingFinished, this,
            &MainWindow::onChipComboEdited);

    auto *interfaceLabel = new QLabel(tr("Interface:"), box);
    m_interfaceLabel = interfaceLabel;
    m_interfaceCombo = new QComboBox(box);
    m_interfaceCombo->addItem(QStringLiteral("40-pin ZIF socket"));
    m_interfaceCombo->addItem(QStringLiteral("ICSP serial"));

    auto *icspEnable = new QCheckBox(tr("ICSP_VCC Enable"), box);
    m_icspEnable = icspEnable;
    m_bits8 = new QRadioButton(tr("8 Bits"), box);
    m_bits16 = new QRadioButton(tr("16 Bits"), box);
    m_bits8->setChecked(true);
    connect(m_bits8, &QRadioButton::toggled, this, [this](bool checked) {
        if (m_hexView)
            m_hexView->setWordMode(!checked);
    });
    connect(m_bits16, &QRadioButton::toggled, this, [this](bool checked) {
        if (m_hexView)
            m_hexView->setWordMode(checked);
    });

    auto *imaxLabel = new QLabel(tr("Vcc current Imax:"), box);
    m_imaxLabel = imaxLabel;
    m_imaxCombo = new QComboBox(box);
    for (const QString &v : {QStringLiteral("Default"), QStringLiteral("50mA"),
                             QStringLiteral("100mA"), QStringLiteral("150mA"),
                             QStringLiteral("200mA")})
        m_imaxCombo->addItem(v);

    auto *saveLog = new QCheckBox(tr("Save Log"), box);
    m_saveLog = saveLog;
    auto *clearLog = new QPushButton(tr("Clear"), box);
    m_clearLog = clearLog;
    clearLog->setEnabled(false);

    auto *upgrade = new QPushButton(tr("Upgrade available"), box);
    m_upgrade = upgrade;
    upgrade->setVisible(false);

    lay->addWidget(findChipButton, 0, 0, 1, 2);
    lay->addWidget(m_chipCombo, 0, 2, 1, 3);
    lay->addWidget(interfaceLabel, 1, 0);
    lay->addWidget(m_interfaceCombo, 1, 1, 1, 3);
    lay->addWidget(icspEnable, 1, 4);
    lay->addWidget(m_bits8, 2, 0);
    lay->addWidget(m_bits16, 2, 1);
    lay->addWidget(imaxLabel, 2, 2);
    lay->addWidget(m_imaxCombo, 2, 3, 1, 2);
    lay->addWidget(saveLog, 3, 0);
    lay->addWidget(clearLog, 3, 1);
    lay->addWidget(upgrade, 3, 3, 1, 2);
    return box;
}

QWidget *MainWindow::buildChipInfoGroup()
{
    auto *box = new QGroupBox(tr("Chip info (No project opened)"), this);
    auto *lay = new QGridLayout(box);
    lay->setHorizontalSpacing(8);
    lay->setVerticalSpacing(4);

    auto *typeTitle = new QLabel(tr("Chip type:"), box);
    m_chipTypeTitle = typeTitle;
    m_chipTypeLabel = new QLabel(tr("unknown"), box);
    auto *sumTitle = new QLabel(tr("Checksum:"), box);
    m_checksumTitle = sumTitle;
    m_checksumLabel = new QLabel(QStringLiteral(" 0000 0000"), box);
    auto *timeTitle = new QLabel(tr("Time:"), box);
    m_timeTitle = timeTitle;
    m_timeLabel = new QLabel(QStringLiteral(" 2000-00-00"), box);

    lay->addWidget(typeTitle, 0, 0);
    lay->addWidget(m_chipTypeLabel, 0, 1);
    lay->addWidget(sumTitle, 0, 2);
    lay->addWidget(m_checksumLabel, 0, 3);
    lay->addWidget(timeTitle, 1, 0);
    lay->addWidget(m_timeLabel, 1, 1);
    return box;
}

QWidget *MainWindow::buildChipListArea()
{
    auto *box = new QWidget(this);
    auto *lay = new QVBoxLayout(box);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);

    m_deviceTable = new QTableWidget(0, 3, box);
    m_deviceTable->setHorizontalHeaderLabels(
        {tr("Type"), tr("Device"), tr("Algorithm")});
    m_deviceTable->verticalHeader()->setVisible(false);
    m_deviceTable->horizontalHeader()->setStretchLastSection(true);
    m_deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_deviceTable->setAlternatingRowColors(true);
    m_deviceTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_deviceTable, &QTableWidget::customContextMenuRequested, this,
            &MainWindow::showDeviceContextMenu);

    auto *tabs = new QTabWidget(box);
    m_tabs = tabs;
    tabs->addTab(m_deviceTable, tr("Device list"));

    m_hexView = new HexView(box);
    connect(m_hexView, &HexView::cursorChanged, this, &MainWindow::onHexCursorMoved);
    connect(m_hexView, &HexView::statusMessage, this,
            &MainWindow::onHexStatusMessage);
    tabs->addTab(m_hexView, tr("Buffer"));

    auto *row = new QHBoxLayout;
    auto *pendingFile = new QLabel(tr(" File to write:"), box);
    m_pendingFileLabel = pendingFile;
    m_pendingEdit = new QLineEdit(box);
    auto *chooseData = new QPushButton(tr("Load file..."), box);
    m_chooseData = chooseData;
    connect(chooseData, &QPushButton::clicked, this, &MainWindow::loadDataFile);
    auto *saveFile = new QLabel(tr(" Save to file:"), box);
    m_saveFileLabel = saveFile;
    auto *saveEdit = new QLineEdit(box);
    auto *chooseTarget = new QPushButton(tr("Save file as..."), box);
    m_chooseTarget = chooseTarget;
    connect(chooseTarget, &QPushButton::clicked, this, &MainWindow::saveDataFile);
    row->addWidget(pendingFile);
    row->addWidget(m_pendingEdit, 1);
    row->addWidget(chooseData);
    row->addWidget(saveFile);
    row->addWidget(saveEdit, 1);
    row->addWidget(chooseTarget);

    lay->addWidget(tabs, 1);
    lay->addLayout(row);
    return box;
}

QWidget *MainWindow::buildProgramSettingsGroup()
{
    auto *box = new QGroupBox(tr("Program settings"), this);
    m_programSettingsGroup = box;
    auto *lay = new QGridLayout(box);
    lay->setHorizontalSpacing(8);
    lay->setVerticalSpacing(4);

    m_pinDetect = new QCheckBox(tr("Pin Detect"), box);
    m_checkId = new QCheckBox(tr("Check ID"), box);
    m_eraseFirst = new QCheckBox(tr("Erase before programming"), box);
    m_verifyAfter = new QCheckBox(tr("Verify after programming"), box);
    m_eraseOtp = new QCheckBox(tr("Erase OTP"), box);
    m_blankCheck = new QCheckBox(tr("Blank check before programming"), box);
    m_skipWriteFF = new QCheckBox(tr("Skip 0xFF writes"), box);

    auto *rangeLabel = new QLabel(tr("Range:"), box);
    m_rangeLabel = rangeLabel;
    auto *partial = new QRadioButton(tr("Partial"), box);
    m_partial = partial;
    auto *all = new QRadioButton(tr("Full"), box);
    m_all = all;
    all->setChecked(true);
    auto *fromLabel = new QLabel(QStringLiteral("0x"), box);
    auto *fromEdit = new QLineEdit(QStringLiteral("0"), box);
    auto *toLabel = new QLabel(QStringLiteral("->"), box);
    auto *toEdit = new QLineEdit(QStringLiteral("0"), box);

    auto *blockLabel = new QLabel(tr("Block:"), box);
    m_blockLabel = blockLabel;
    auto *blockSpin = new QSpinBox(box);
    blockSpin->setMaximum(0x7fffffff);

    lay->addWidget(m_pinDetect, 0, 0);
    lay->addWidget(m_checkId, 0, 1);
    lay->addWidget(m_eraseFirst, 1, 0);
    lay->addWidget(m_verifyAfter, 1, 1);
    lay->addWidget(m_eraseOtp, 2, 0);
    lay->addWidget(m_blankCheck, 2, 1);
    lay->addWidget(m_skipWriteFF, 3, 0);
    lay->addWidget(rangeLabel, 4, 0);
    lay->addWidget(partial, 4, 1);
    lay->addWidget(all, 5, 1);
    lay->addWidget(fromLabel, 5, 2);
    lay->addWidget(fromEdit, 5, 3);
    lay->addWidget(toLabel, 5, 4);
    lay->addWidget(toEdit, 5, 5);
    lay->addWidget(blockLabel, 6, 0);
    lay->addWidget(blockSpin, 6, 1, 1, 3);
    lay->setColumnStretch(6, 1);
    return box;
}

QWidget *MainWindow::buildChipConfigGroup()
{
    auto *box = new QWidget(this);
    auto *lay = new QVBoxLayout(box);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);

    auto *spi = new QGroupBox(tr("SPI EEPROM (25/35/95 series) status bits"), box);
    m_spiGroup = spi;
    auto *spiLay = new QHBoxLayout(spi);
    auto *readStatus = new QPushButton(tr("Read status"), spi);
    m_readStatus = readStatus;
    auto *writeStatus = new QPushButton(tr("Write status"), spi);
    m_writeStatus = writeStatus;
    connect(readStatus, &QPushButton::clicked, this,
            [this] { stubOperation(tr("Read SPI status")); });
    connect(writeStatus, &QPushButton::clicked, this,
            [this] { stubOperation(tr("Write SPI status")); });
    spiLay->addWidget(readStatus);
    spiLay->addWidget(writeStatus);
    spiLay->addStretch(1);

    auto *config = new QGroupBox(tr("Chip configuration"), box);
    m_configGroup = config;
    auto *confLay = new QGridLayout(config);
    auto *userIdLabel = new QLabel(tr("USERID:"), config);
    m_userIdLabel = userIdLabel;
    auto *userIdEdit = new QLineEdit(config);
    auto *unprotectBefore = new QCheckBox(tr("Unprotect before programming"), config);
    m_unprotectBefore = unprotectBefore;
    auto *protectAfter = new QCheckBox(tr("Protect after programming"), config);
    m_protectAfter = protectAfter;
    auto *unprotect = new QPushButton(tr("Unprotect"), config);
    m_unprotect = unprotect;
    auto *protectedNotice = new QLabel(tr("Protected mode — some functions are disabled!"),
                                       config);
    m_protectedNotice = protectedNotice;
    protectedNotice->setEnabled(false);

    confLay->addWidget(userIdLabel, 0, 0);
    confLay->addWidget(userIdEdit, 0, 1);
    confLay->addWidget(unprotectBefore, 1, 0, 1, 2);
    confLay->addWidget(protectAfter, 2, 0, 1, 2);
    confLay->addWidget(unprotect, 3, 0);
    confLay->addWidget(protectedNotice, 3, 1);

    lay->addWidget(spi);
    lay->addWidget(config);
    lay->addStretch(1);
    return box;
}

void MainWindow::buildMenus()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu = fileMenu;
    auto *openAction = fileMenu->addAction(tr("&Open..."));
    m_openAction = openAction;
    openAction->setShortcut(QKeySequence::Open);
    auto *saveAction = fileMenu->addAction(tr("&Save to file"));
    m_saveAction = saveAction;
    saveAction->setShortcut(QKeySequence::Save);
    connect(openAction, &QAction::triggered, this, &MainWindow::loadDataFile);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveDataFile);
    fileMenu->addSeparator();
    auto *clearBufferAction = fileMenu->addAction(tr("Clear &Current Buffer"));
    m_clearBufferAction = clearBufferAction;
    connect(clearBufferAction, &QAction::triggered, this,
            [this] { updateBuffer(QByteArray()); });
    auto *clearAllAction = fileMenu->addAction(tr("Clear &All Buffers"));
    m_clearAllAction = clearAllAction;
    connect(clearAllAction, &QAction::triggered, this,
            [this] { updateBuffer(QByteArray()); });
    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(tr("E&xit"));
    m_quitAction = quitAction;
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &MainWindow::close);

    auto *chipMenu = menuBar()->addMenu(tr("&Chip Select"));
    m_chipMenu = chipMenu;
    auto *findChipAction = chipMenu->addAction(tr("Find &Select Chip..."));
    m_findChipAction = findChipAction;
    connect(findChipAction, &QAction::triggered, this, &MainWindow::showChipDialog);
    chipMenu->addSeparator();
    auto *flashIdentifyAction = chipMenu->addAction(tr("25 Flash Identify"));
    m_flashIdentifyAction = flashIdentifyAction;
    connect(flashIdentifyAction, &QAction::triggered, this,
            [this] { performOperation(OpType::FlashIdentify); });

    auto *opMenu = menuBar()->addMenu(tr("&Operations"));
    m_opMenu = opMenu;
    auto *readAction = opMenu->addAction(tr("&Read chip"));
    m_readAction = readAction;
    auto *idAction = opMenu->addAction(tr("Chip &ID"));
    m_idAction = idAction;
    auto *verifyAction = opMenu->addAction(tr("&Verify"));
    m_verifyAction = verifyAction;
    opMenu->addSeparator();
    auto *programAction = opMenu->addAction(tr("&Program"));
    m_programAction = programAction;
    auto *eraseAction = opMenu->addAction(tr("&Erase"));
    m_eraseAction = eraseAction;
    auto *blankAction = opMenu->addAction(tr("&Blank check"));
    m_blankAction = blankAction;
    connect(readAction, &QAction::triggered, this, [this] { performOperation(OpType::Read); });
    connect(idAction, &QAction::triggered, this, [this] { performOperation(OpType::ChipId); });
    connect(verifyAction, &QAction::triggered, this, [this] { performOperation(OpType::Verify); });
    connect(programAction, &QAction::triggered, this, [this] { performOperation(OpType::Program); });
    connect(eraseAction, &QAction::triggered, this, [this] { performOperation(OpType::Erase); });
    connect(blankAction, &QAction::triggered, this, [this] { performOperation(OpType::BlankCheck); });

    auto *toolsMenu = menuBar()->addMenu(tr("System &Tools"));
    m_toolsMenu = toolsMenu;
    auto *selectProgAction = toolsMenu->addAction(tr("Select &Programmer..."));
    m_selectProgAction = selectProgAction;
    selectProgAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+P")));
    connect(selectProgAction, &QAction::triggered, this,
            &MainWindow::selectProgrammer);
    toolsMenu->addSeparator();
    auto *calculatorAction = toolsMenu->addAction(tr("&Calculator"));
    m_calculatorAction = calculatorAction;
    connect(calculatorAction, &QAction::triggered, this,
            [this] { stubOperation(tr("Calculator")); });
    toolsMenu->addSeparator();
    auto *selftestAction = toolsMenu->addAction(tr("Programmer &Self-Test"));
    m_selftestAction = selftestAction;
    connect(selftestAction, &QAction::triggered, this,
            [this] { stubOperation(tr("Self-test")); });
    toolsMenu->addSeparator();
    auto *firmwareAction = toolsMenu->addAction(tr("&Firmware Flash Update"));
    m_firmwareAction = firmwareAction;
    connect(firmwareAction, &QAction::triggered, this,
            [this] { stubOperation(tr("Firmware update")); });
    auto *adapterAction = toolsMenu->addAction(tr("&Adapter Test"));
    m_adapterAction = adapterAction;
    connect(adapterAction, &QAction::triggered, this,
            [this] { stubOperation(tr("Adapter test")); });

    auto *themeMenu = menuBar()->addMenu(tr("&Theme"));
    m_themeMenu = themeMenu;
    m_themeGroup = new QActionGroup(this);
    m_themeGroup->setExclusive(true);
    auto addThemeAction = [this, themeMenu](Theme::Mode mode, const QString &text) {
        auto *action = themeMenu->addAction(text);
        action->setCheckable(true);
        action->setData(static_cast<int>(mode));
        m_themeGroup->addAction(action);
        return action;
    };
    auto *systemAction = addThemeAction(Theme::Mode::System, tr("Follow &system"));
    m_themeSystemAction = systemAction;
    auto *lightAction = addThemeAction(Theme::Mode::Light, tr("&Light"));
    m_themeLightAction = lightAction;
    auto *darkAction = addThemeAction(Theme::Mode::Dark, tr("&Dark"));
    m_themeDarkAction = darkAction;
    Q_UNUSED(lightAction);
    Q_UNUSED(darkAction);
    systemAction->setChecked(true);
    connect(m_themeGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        setTheme(action->data().toInt());
    });

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu = helpMenu;
    auto *aboutAction = helpMenu->addAction(tr("&About"));
    m_aboutAction = aboutAction;
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addSeparator();
    auto *upgradeAction = helpMenu->addAction(tr("&Online Upgrade"));
    m_upgradeAction = upgradeAction;
    connect(upgradeAction, &QAction::triggered, this,
            [this] { stubOperation(tr("Online upgrade")); });

    auto *languageMenu = menuBar()->addMenu(tr("&Language"));
    m_languageMenu = languageMenu;
    m_languageGroup = new QActionGroup(this);
    m_languageGroup->setExclusive(true);
    const auto addLanguage = [this, languageMenu](const QString &code, const QString &label) {
        auto *action = languageMenu->addAction(label);
        action->setCheckable(true);
        action->setData(code);
        m_languageGroup->addAction(action);
        connect(action, &QAction::triggered, this,
                [this, code] { applyLanguage(code, true); });
        return action;
    };
    auto *languageSystemAction = addLanguage(QStringLiteral("system"), tr("&System language"));
    m_languageSystemAction = languageSystemAction;
    addLanguage(QStringLiteral("en"), QStringLiteral("English"));
    addLanguage(QStringLiteral("pl"), QStringLiteral("Polski"));
    addLanguage(QStringLiteral("ru"), QStringLiteral("Русский"));
    addLanguage(QStringLiteral("cs"), QStringLiteral("Česky"));
    addLanguage(QStringLiteral("es"), QStringLiteral("Español"));
    addLanguage(QStringLiteral("de"), QStringLiteral("Deutsch"));
    addLanguage(QStringLiteral("pt_BR"), QStringLiteral("Português (Brasil)"));
    addLanguage(QStringLiteral("fr"), QStringLiteral("Français"));
    addLanguage(QStringLiteral("hu"), QStringLiteral("Magyar"));
    addLanguage(QStringLiteral("it"), QStringLiteral("Italiano"));
    addLanguage(QStringLiteral("tr"), QStringLiteral("Türkçe"));

    const QString current = savedLanguage();
    for (auto *action : m_languageGroup->actions())
        action->setChecked(action->data().toString() == current);
}

void MainWindow::showChipDialog()
{
    const ChipInfo chip = ChipDialog::getChip(m_chips, this);
    if (!chip.name.isEmpty())
        applySelectedChip(chip);
}

void MainWindow::onChipComboEdited()
{
    const QString text = m_chipCombo->currentText().trimmed();
    if (text.isEmpty())
        return;
    const QVector<ChipInfo> matches = m_chips.matching(QString(), text, true);
    if (matches.isEmpty())
        return;
    applySelectedChip(matches.first());
}

void MainWindow::applySelectedChip(const ChipInfo &chip)
{
    m_chipCombo->setCurrentText(chip.name);
    m_currentChip = chip.name;
    m_currentAlgorithmFile = m_chips.algorithmFile(chip);

    m_deviceTable->setRowCount(1);
    m_deviceTable->setItem(0, 0, new QTableWidgetItem(chip.category));
    m_deviceTable->setItem(0, 1, new QTableWidgetItem(chip.name));
    m_deviceTable->setItem(
        0, 2, new QTableWidgetItem(
                  m_currentAlgorithmFile.isEmpty()
                      ? QStringLiteral("—")
                      : QFileInfo(m_currentAlgorithmFile).fileName()));

    m_chipTypeLabel->setText(chip.name);

    const QStringList labels = ChipDatabase::categoryLabels();
    const QStringList keys = ChipDatabase::categories();
    const int idx = keys.indexOf(chip.category);
    const QString label = idx >= 0 ? labels.at(idx) : chip.category;
    m_chipTypeLabel->setText(QStringLiteral("%1 (%2)").arg(chip.name, label));
    m_deviceTable->item(0, 0)->setText(label);

    setWindowTitle(QStringLiteral("%1 %2 — %3")
                       .arg(QLatin1String(APP_NAME), QLatin1String(APP_VERSION), chip.name));
    statusBar()->showMessage(tr("Chip selected: %1").arg(chip.name), 4000);

    QSettings settings;
    settings.setValue(QStringLiteral("chip/last"), chip.name);
}

void MainWindow::showDeviceContextMenu(const QPoint &pos)
{
    auto *menu = new QMenu(this);
    auto addStub = [this, menu](const QString &text, const QString &what) {
        auto *action = menu->addAction(text);
        connect(action, &QAction::triggered, this, [this, what] { stubOperation(what); });
        return action;
    };

    if (m_hexView) {
        auto *copy = menu->addAction(tr("Copy Hex"));
        copy->setShortcut(QKeySequence::Copy);
        connect(copy, &QAction::triggered, m_hexView, &HexView::copySelectionAsHex);
        auto *paste = addStub(tr("Paste"), tr("Paste"));
        paste->setShortcut(QKeySequence::Paste);
        menu->addSeparator();
        auto *blockFill = menu->addAction(tr("Block Fill..."));
        connect(blockFill, &QAction::triggered, this, &MainWindow::blockFill);
        auto *blockSave = menu->addAction(tr("Block Save As... (txt file)"));
        connect(blockSave, &QAction::triggered, this, &MainWindow::saveBlockAs);
        auto *blockClear = menu->addAction(tr("Clear Current Buffer"));
        connect(blockClear, &QAction::triggered, this,
                [this] { updateBuffer(QByteArray()); });
        menu->addSeparator();
        auto *find = menu->addAction(tr("Find..."));
        find->setShortcut(QKeySequence::Find);
        connect(find, &QAction::triggered, m_hexView, &HexView::showFindDialog);
        auto *findNext = menu->addAction(tr("Find Next"));
        findNext->setShortcut(QKeySequence(Qt::Key_F3));
        connect(findNext, &QAction::triggered, m_hexView, &HexView::findNextFromCursor);
        auto *goTo = menu->addAction(tr("Go to Address..."));
        goTo->setShortcut(QKeySequence(QStringLiteral("Ctrl+G")));
        connect(goTo, &QAction::triggered, m_hexView, &HexView::showGotoDialog);
    } else {
        addStub(tr("Copy"), tr("Copy"));
        addStub(tr("Paste"), tr("Paste"));
    }
    menu->exec(m_deviceTable->viewport()->mapToGlobal(pos));
    delete menu;
}

void MainWindow::blockFill()
{
    const BlockFillDialog::Params params =
        BlockFillDialog::getFill(m_buffer, this);
    if (!params.valid)
        return;
    QByteArray data = m_buffer;
    BlockFillDialog::apply(data, params);
    updateBuffer(data);
    statusBar()->showMessage(
        tr("Buffer filled from 0x%1 to 0x%2")
            .arg(params.start, 0, 16)
            .arg(params.end, 0, 16)
            .toUpper(),
        4000);
}

void MainWindow::saveBlockAs()
{
    if (m_buffer.isEmpty()) {
        QMessageBox::information(this, tr("Block Save As"),
                                 tr("Buffer is empty — nothing to save."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Block Save As"), QStringLiteral("block.txt"),
        tr("Text file (*.txt);;Binary file (*.bin);;All files (*)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("Block Save As"),
                             tr("Could not write %1:\n%2").arg(path, file.errorString()));
        return;
    }

    if (path.toLower().endsWith(QStringLiteral(".bin"))) {
        file.write(m_buffer);
    } else {
        // Text form: 16 bytes per line, ASCII dump on the right.
        QTextStream out(&file);
        out.setFieldAlignment(QTextStream::AlignRight);
        for (qsizetype i = 0; i < m_buffer.size(); i += 16) {
            QString hex;
            QString ascii;
            for (qsizetype b = 0; b < 16 && i + b < m_buffer.size(); ++b) {
                const char c = m_buffer.at(i + b);
                if (b)
                    hex += QLatin1Char(' ');
                hex += QStringLiteral("%1").arg(static_cast<quint8>(c), 2, 16,
                                                QLatin1Char('0')).toUpper();
                ascii += (c >= 0x20 && c <= 0x7e) ? QLatin1Char(c)
                                                  : QLatin1Char('.');
            }
            out << QStringLiteral("%1  %2  |%3|\n")
                       .arg(i, 8, 16, QLatin1Char('0'))
                       .arg(hex, -48)
                       .arg(ascii, -16)
                       .toUpper();
        }
    }
    file.close();
    statusBar()->showMessage(
        tr("Saved block (%1 bytes) to %2").arg(m_buffer.size()).arg(path), 5000);
}

void MainWindow::performOperation(OpType op)
{
    const OpResult result =
        runOperation(op, m_model, m_currentChip, m_buffer, m_currentAlgorithmFile);
    if (result.ok) {
        statusBar()->showMessage(result.message, 5000);
        return;
    }
    QMessageBox::warning(this, opName(op), result.message);
}

void MainWindow::refreshDeviceStatus()
{
    if (m_deviceConnected)
        statusBar()->showMessage(
            tr("%1 connected").arg(Programmer::modelName(m_model)));
    else
        statusBar()->showMessage(
            tr("%1 — no programmer connected").arg(Programmer::modelName(m_model)));
}

void MainWindow::loadDataFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open file"), QString(),
        tr("Binary and HEX files (*.bin *.hex *.rom *.dat);;Intel HEX (*.hex);;"
           "All files (*)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Open file"),
                             tr("Could not open %1:\n%2").arg(path, file.errorString()));
        return;
    }
    const QByteArray raw = file.readAll();
    file.close();

    const QString lower = path.toLower();
    QByteArray data;
    bool ok = true;
    if (lower.endsWith(QStringLiteral(".hex"))) {
        data = parseIntelHex(raw, &ok);
        if (!ok) {
            QMessageBox::warning(this, tr("Open file"),
                                 tr("Invalid Intel HEX data in %1.").arg(path));
            return;
        }
    } else {
        data = raw;
    }

    m_pendingEdit->setText(path);
    updateBuffer(data);
    statusBar()->showMessage(
        tr("Loaded %1 — %2 bytes").arg(path).arg(data.size()), 5000);
}

void MainWindow::saveDataFile()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save file"), QString(),
        tr("Binary file (*.bin);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("Save file"),
                             tr("Could not write %1:\n%2").arg(path, file.errorString()));
        return;
    }
    file.write(m_buffer);
    file.close();
    statusBar()->showMessage(
        tr("Saved %1 — %2 bytes").arg(path).arg(m_buffer.size()), 5000);
}

void MainWindow::updateBuffer(const QByteArray &data)
{
    m_buffer = data;
    if (m_hexView)
        m_hexView->setData(data);
    updateChecksumLabel();
}

void MainWindow::onHexCursorMoved(quint64 index)
{
    if (m_buffer.isEmpty() || index >= static_cast<quint64>(m_buffer.size()))
        return;
    statusBar()->showMessage(
        tr("Addr: 0x%1  Value: 0x%2")
            .arg(index, addressDigits(index + 1), 16, QLatin1Char('0'))
            .arg(static_cast<quint8>(m_buffer.at(index)), 2, 16, QLatin1Char('0'))
            .toUpper(),
        3000);
}

void MainWindow::onHexStatusMessage(const QString &message)
{
    statusBar()->showMessage(message, 5000);
}

void MainWindow::updateChecksumLabel()
{
    const quint32 crc = crc32(m_buffer);
    m_checksumLabel->setText(
        QStringLiteral(" %1").arg(crc, 8, 16, QLatin1Char('0')).toUpper());
    m_timeLabel->setText(
        tr(" %1").arg(QDateTime::currentDateTime().toString(
                          QStringLiteral("yyyy-MM-dd hh:mm:ss")),
                      -1));
}

void MainWindow::stubOperation(const QString &what)
{
    statusBar()->showMessage(what + tr(" — not implemented yet"), 4000);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About"),
                       tr("<b>%1 %2</b><br/>"
                          "Open-source programmer for TL866II / T48 / T56.")
                           .arg(QLatin1String(APP_NAME), QLatin1String(APP_VERSION)));
}

void MainWindow::selectProgrammer()
{
    const ProgrammerModel model = ProgrammerDialog::getModel(m_model, this);
    if (model == m_model)
        return;
    m_model = model;
    QSettings settings;
    settings.setValue(QStringLiteral("programmer/model"),
                      Programmer::modelName(m_model));
    statusBar()->showMessage(
        tr("Programmer: %1").arg(Programmer::modelName(m_model)), 4000);
}

void MainWindow::setTheme(int mode)
{
    const auto theme = static_cast<Theme::Mode>(mode);
    Theme::setMode(theme);
    if (m_themeGroup) {
        const auto actions = m_themeGroup->actions();
        for (auto *action : actions)
            action->setChecked(action->data().toInt() == mode);
    }
    statusBar()->showMessage(tr("Theme: %1").arg(Theme::modeName(theme)), 3000);
}

void MainWindow::restoreSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("mainwindow"));
    restoreGeometry(settings.value(QStringLiteral("geometry")).toByteArray());
    settings.endGroup();

    Theme::restoreSavedMode();
    const Theme::Mode saved = Theme::mode();
    if (m_themeGroup) {
        const auto actions = m_themeGroup->actions();
        for (auto *action : actions)
            action->setChecked(action->data().toInt() == static_cast<int>(saved));
    }
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("mainwindow"));
    settings.setValue(QStringLiteral("geometry"), saveGeometry());
    settings.endGroup();
}

void MainWindow::loadLastChip()
{
    QSettings settings;
    const QString name = settings.value(QStringLiteral("chip/last")).toString();
    if (name.isEmpty())
        return;
    const QVector<ChipInfo> matches = m_chips.matching(QString(), name, true);
    if (!matches.isEmpty())
        applySelectedChip(matches.first());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange && m_uiBuilt)
        retranslateUi();
    QMainWindow::changeEvent(event);
}

void MainWindow::applyLanguage(const QString &code, bool persist)
{
    QApplication::removeTranslator(m_translator);

    QString loc = code;
    if (loc == QStringLiteral("system"))
        loc = QLocale::system().name();
    if (!loc.isEmpty() && loc != QLatin1String("en")) {
        // Try the full locale (e.g. "pt_BR") first, then the bare language
        // code (e.g. "pt").
        const QStringList candidates = {loc, loc.section(QLatin1Char('_'), 0, 0)};
        for (const QString &c : candidates) {
            if (m_translator->load(
                    QStringLiteral(":/i18n/OpenXgpro_%1.qm").arg(c))) {
                QApplication::installTranslator(m_translator);
                break;
            }
        }
    }

    if (persist) {
        QSettings settings;
        settings.setValue(QStringLiteral("ui/language"), code);
    }
    if (m_languageGroup) {
        for (auto *action : m_languageGroup->actions())
            action->setChecked(action->data().toString() == code);
    }
}

void MainWindow::retranslateUi()
{
    m_findChipButton->setText(tr("Find &Select Chip..."));
    m_interfaceLabel->setText(tr("Interface:"));
    m_icspEnable->setText(tr("ICSP_VCC Enable"));
    m_bits8->setText(tr("8 Bits"));
    m_bits16->setText(tr("16 Bits"));
    m_imaxLabel->setText(tr("Vcc current Imax:"));
    m_saveLog->setText(tr("Save Log"));
    m_clearLog->setText(tr("Clear"));
    m_upgrade->setText(tr("Upgrade available"));

    m_chipTypeTitle->setText(tr("Chip type:"));
    m_checksumTitle->setText(tr("Checksum:"));
    m_timeTitle->setText(tr("Time:"));

    m_deviceTable->setHorizontalHeaderLabels(
        {tr("Type"), tr("Device"), tr("Algorithm")});
    m_tabs->setTabText(0, tr("Device list"));
    m_tabs->setTabText(1, tr("Buffer"));
    m_pendingFileLabel->setText(tr(" File to write:"));
    m_chooseData->setText(tr("Load file..."));
    m_saveFileLabel->setText(tr(" Save to file:"));
    m_chooseTarget->setText(tr("Save file as..."));

    m_programSettingsGroup->setTitle(tr("Program settings"));
    m_pinDetect->setText(tr("Pin Detect"));
    m_checkId->setText(tr("Check ID"));
    m_eraseFirst->setText(tr("Erase before programming"));
    m_verifyAfter->setText(tr("Verify after programming"));
    m_eraseOtp->setText(tr("Erase OTP"));
    m_blankCheck->setText(tr("Blank check before programming"));
    m_skipWriteFF->setText(tr("Skip 0xFF writes"));
    m_rangeLabel->setText(tr("Range:"));
    m_partial->setText(tr("Partial"));
    m_all->setText(tr("Full"));
    m_blockLabel->setText(tr("Block:"));

    m_spiGroup->setTitle(tr("SPI EEPROM (25/35/95 series) status bits"));
    m_readStatus->setText(tr("Read status"));
    m_writeStatus->setText(tr("Write status"));
    m_configGroup->setTitle(tr("Chip configuration"));
    m_userIdLabel->setText(tr("USERID:"));
    m_unprotectBefore->setText(tr("Unprotect before programming"));
    m_protectAfter->setText(tr("Protect after programming"));
    m_unprotect->setText(tr("Unprotect"));
    m_protectedNotice->setText(
        tr("Protected mode — some functions are disabled!"));

    m_fileMenu->setTitle(tr("&File"));
    m_openAction->setText(tr("&Open..."));
    m_saveAction->setText(tr("&Save to file"));
    m_clearBufferAction->setText(tr("Clear &Current Buffer"));
    m_clearAllAction->setText(tr("Clear &All Buffers"));
    m_quitAction->setText(tr("E&xit"));

    m_chipMenu->setTitle(tr("&Chip Select"));
    m_findChipAction->setText(tr("Find &Select Chip..."));
    m_flashIdentifyAction->setText(tr("25 Flash Identify"));

    m_opMenu->setTitle(tr("&Operations"));
    m_readAction->setText(tr("&Read chip"));
    m_idAction->setText(tr("Chip &ID"));
    m_verifyAction->setText(tr("&Verify"));
    m_programAction->setText(tr("&Program"));
    m_eraseAction->setText(tr("&Erase"));
    m_blankAction->setText(tr("&Blank check"));

    m_toolsMenu->setTitle(tr("System &Tools"));
    m_selectProgAction->setText(tr("Select &Programmer..."));
    m_calculatorAction->setText(tr("&Calculator"));
    m_selftestAction->setText(tr("Programmer &Self-Test"));
    m_firmwareAction->setText(tr("&Firmware Flash Update"));
    m_adapterAction->setText(tr("&Adapter Test"));

    m_themeMenu->setTitle(tr("&Theme"));
    m_themeSystemAction->setText(tr("Follow &system"));
    m_themeLightAction->setText(tr("&Light"));
    m_themeDarkAction->setText(tr("&Dark"));

    m_helpMenu->setTitle(tr("&Help"));
    m_aboutAction->setText(tr("&About"));
    m_upgradeAction->setText(tr("&Online Upgrade"));

    m_languageMenu->setTitle(tr("&Language"));
    m_languageSystemAction->setText(tr("&System language"));

    setWindowTitle(m_currentChip.isEmpty()
                       ? QStringLiteral("%1 %2").arg(QLatin1String(APP_NAME),
                                                     QLatin1String(APP_VERSION))
                       : QStringLiteral("%1 %2 — %3").arg(QLatin1String(APP_NAME),
                                                          QLatin1String(APP_VERSION),
                                                          m_currentChip));
    refreshDeviceStatus();
}
