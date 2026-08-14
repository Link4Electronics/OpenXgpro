#include "mainwindow.h"

#include "chipdialog.h"
#include "theme.h"
#include "version.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
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
#include <QToolButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("%1 %2").arg(QLatin1String(APP_NAME),
                                               QLatin1String(APP_VERSION)));

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
}

QWidget *MainWindow::buildDeviceGroup()
{
    auto *box = new QGroupBox(tr("Device"), this);
    auto *lay = new QGridLayout(box);
    lay->setHorizontalSpacing(6);
    lay->setVerticalSpacing(4);

    auto *findChipButton = new QPushButton(tr("Find &Select Chip..."), box);
    connect(findChipButton, &QPushButton::clicked, this, &MainWindow::showChipDialog);

    m_chipCombo = new QComboBox(box);
    m_chipCombo->setEditable(true);
    m_chipCombo->setInsertPolicy(QComboBox::NoInsert);
    connect(m_chipCombo, &QComboBox::activated, this, &MainWindow::onChipComboEdited);
    connect(m_chipCombo->lineEdit(), &QLineEdit::editingFinished, this,
            &MainWindow::onChipComboEdited);

    auto *interfaceLabel = new QLabel(tr("Interface:"), box);
    m_interfaceCombo = new QComboBox(box);
    m_interfaceCombo->addItem(QStringLiteral("40-pin ZIF socket"));
    m_interfaceCombo->addItem(QStringLiteral("ICSP serial"));

    auto *icspEnable = new QCheckBox(tr("ICSP_VCC Enable"), box);
    auto *bits8 = new QRadioButton(tr("8 Bits"), box);
    auto *bits16 = new QRadioButton(tr("16 Bits"), box);
    bits8->setChecked(true);
    bits8->setVisible(false);
    bits16->setVisible(false);

    auto *imaxLabel = new QLabel(tr("Vcc current Imax:"), box);
    m_imaxCombo = new QComboBox(box);
    for (const QString &v : {QStringLiteral("Default"), QStringLiteral("50mA"),
                             QStringLiteral("100mA"), QStringLiteral("150mA"),
                             QStringLiteral("200mA")})
        m_imaxCombo->addItem(v);

    auto *saveLog = new QCheckBox(tr("Save Log"), box);
    auto *clearLog = new QPushButton(tr("Clear"), box);
    clearLog->setEnabled(false);

    auto *upgrade = new QPushButton(tr("Upgrade available"), box);
    upgrade->setVisible(false);

    lay->addWidget(findChipButton, 0, 0, 1, 2);
    lay->addWidget(m_chipCombo, 0, 2, 1, 3);
    lay->addWidget(interfaceLabel, 1, 0);
    lay->addWidget(m_interfaceCombo, 1, 1, 1, 3);
    lay->addWidget(icspEnable, 1, 4);
    lay->addWidget(bits8, 2, 0);
    lay->addWidget(bits16, 2, 1);
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
    m_chipTypeLabel = new QLabel(tr("unknown"), box);
    auto *sumTitle = new QLabel(tr("Checksum:"), box);
    m_checksumLabel = new QLabel(QStringLiteral(" 0000 0000"), box);
    auto *timeTitle = new QLabel(tr("Time:"), box);
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

    m_deviceTable = new QTableWidget(0, 2, box);
    m_deviceTable->setHorizontalHeaderLabels({tr("Type"), tr("Device")});
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
    tabs->addTab(m_deviceTable, tr("Device list"));

    auto *row = new QHBoxLayout;
    auto *pendingFile = new QLabel(tr(" File to write:"), box);
    auto *pendingEdit = new QLineEdit(box);
    auto *chooseData = new QPushButton(tr("Load file..."), box);
    auto *saveFile = new QLabel(tr(" Save to file:"), box);
    auto *saveEdit = new QLineEdit(box);
    auto *chooseTarget = new QPushButton(tr("Save file as..."), box);
    row->addWidget(pendingFile);
    row->addWidget(pendingEdit, 1);
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
    auto *partial = new QRadioButton(tr("Partial"), box);
    auto *all = new QRadioButton(tr("Full"), box);
    all->setChecked(true);
    auto *fromLabel = new QLabel(QStringLiteral("0x"), box);
    auto *fromEdit = new QLineEdit(QStringLiteral("0"), box);
    auto *toLabel = new QLabel(QStringLiteral("->"), box);
    auto *toEdit = new QLineEdit(QStringLiteral("0"), box);

    auto *blockLabel = new QLabel(tr("Block:"), box);
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
    auto *spiLay = new QHBoxLayout(spi);
    auto *readStatus = new QPushButton(tr("Read status"), spi);
    auto *writeStatus = new QPushButton(tr("Write status"), spi);
    connect(readStatus, &QPushButton::clicked, this,
            [this] { stubOperation(tr("Read SPI status")); });
    connect(writeStatus, &QPushButton::clicked, this,
            [this] { stubOperation(tr("Write SPI status")); });
    spiLay->addWidget(readStatus);
    spiLay->addWidget(writeStatus);
    spiLay->addStretch(1);

    auto *config = new QGroupBox(tr("Chip configuration"), box);
    auto *confLay = new QGridLayout(config);
    auto *userIdLabel = new QLabel(tr("USERID:"), config);
    auto *userIdEdit = new QLineEdit(config);
    auto *unprotectBefore = new QCheckBox(tr("Unprotect before programming"), config);
    auto *protectAfter = new QCheckBox(tr("Protect after programming"), config);
    auto *unprotect = new QPushButton(tr("Unprotect"), config);
    auto *protectedNotice = new QLabel(tr("Protected mode — some functions are disabled!"),
                                       config);
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
    auto *openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setShortcut(QKeySequence::Open);
    auto *saveAction = fileMenu->addAction(tr("&Save to file"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(openAction, &QAction::triggered, this, [this] { stubOperation(tr("Open file")); });
    connect(saveAction, &QAction::triggered, this, [this] { stubOperation(tr("Save file")); });
    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &MainWindow::close);

    auto *chipMenu = menuBar()->addMenu(tr("&Chip Select"));
    auto *findChipAction = chipMenu->addAction(tr("Find &Select Chip..."));
    connect(findChipAction, &QAction::triggered, this, &MainWindow::showChipDialog);
    chipMenu->addSeparator();
    auto *flashIdentifyAction = chipMenu->addAction(tr("25 Flash Identify"));
    connect(flashIdentifyAction, &QAction::triggered, this,
            [this] { stubOperation(tr("25 Flash Identify")); });

    auto *opMenu = menuBar()->addMenu(tr("&Operations"));
    auto *readAction = opMenu->addAction(tr("&Read chip"));
    auto *idAction = opMenu->addAction(tr("Chip &ID"));
    auto *verifyAction = opMenu->addAction(tr("&Verify"));
    opMenu->addSeparator();
    auto *programAction = opMenu->addAction(tr("&Program"));
    auto *eraseAction = opMenu->addAction(tr("&Erase"));
    auto *blankAction = opMenu->addAction(tr("&Blank check"));
    connect(readAction, &QAction::triggered, this, [this] { stubOperation(tr("Read")); });
    connect(idAction, &QAction::triggered, this, [this] { stubOperation(tr("Device ID")); });
    connect(verifyAction, &QAction::triggered, this, [this] { stubOperation(tr("Verify")); });
    connect(programAction, &QAction::triggered, this, [this] { stubOperation(tr("Program")); });
    connect(eraseAction, &QAction::triggered, this, [this] { stubOperation(tr("Erase")); });
    connect(blankAction, &QAction::triggered, this, [this] { stubOperation(tr("Blank check")); });

    auto *toolsMenu = menuBar()->addMenu(tr("System &Tools"));
    auto *calculatorAction = toolsMenu->addAction(tr("&Calculator"));
    connect(calculatorAction, &QAction::triggered, this,
            [this] { stubOperation(tr("Calculator")); });
    toolsMenu->addSeparator();
    auto *selftestAction = toolsMenu->addAction(tr("Programmer &Self-Test"));
    connect(selftestAction, &QAction::triggered, this,
            [this] { stubOperation(tr("Self-test")); });
    toolsMenu->addSeparator();
    auto *firmwareAction = toolsMenu->addAction(tr("&Firmware Flash Update"));
    connect(firmwareAction, &QAction::triggered, this,
            [this] { stubOperation(tr("Firmware update")); });
    auto *adapterAction = toolsMenu->addAction(tr("&Adapter Test"));
    connect(adapterAction, &QAction::triggered, this,
            [this] { stubOperation(tr("Adapter test")); });

    auto *themeMenu = menuBar()->addMenu(tr("&Theme"));
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
    auto *lightAction = addThemeAction(Theme::Mode::Light, tr("&Light"));
    auto *darkAction = addThemeAction(Theme::Mode::Dark, tr("&Dark"));
    Q_UNUSED(lightAction);
    Q_UNUSED(darkAction);
    systemAction->setChecked(true);
    connect(m_themeGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        setTheme(action->data().toInt());
    });

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    auto *aboutAction = helpMenu->addAction(tr("&About MiniPro"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addSeparator();
    auto *upgradeAction = helpMenu->addAction(tr("&Online Upgrade"));
    connect(upgradeAction, &QAction::triggered, this,
            [this] { stubOperation(tr("Online upgrade")); });
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

    m_deviceTable->setRowCount(1);
    m_deviceTable->setItem(0, 0, new QTableWidgetItem(chip.category));
    m_deviceTable->setItem(0, 1, new QTableWidgetItem(chip.name));

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
    auto add = [this, menu](const QString &text, const QString &what) {
        auto *action = menu->addAction(text);
        connect(action, &QAction::triggered, this, [this, what] { stubOperation(what); });
        return action;
    };
    auto *copy = add(tr("Copy"), tr("Copy"));
    auto *paste = add(tr("Paste"), tr("Paste"));
    auto *blockSave = add(tr("Block Save As... (txt file)"), tr("Block save as"));
    auto *blockDefine = add(tr("Block Define"), tr("Block define"));
    copy->setShortcut(QKeySequence::Copy);
    paste->setShortcut(QKeySequence::Paste);
    blockDefine->setShortcut(QKeySequence(QStringLiteral("Ctrl+B")));
    menu->addSeparator();
    add(tr("Block Fill"), tr("Block fill"));
    add(tr("Clear Current Buffer"), tr("Clear current buffer"));
    add(tr("Clear All Buffers"), tr("Clear all buffers"));
    menu->addSeparator();
    auto *find = add(tr("Find"), tr("Find"));
    auto *findNext = add(tr("Find Next"), tr("Find next"));
    auto *goTo = add(tr("Go to Address"), tr("Go to address"));
    find->setShortcut(QKeySequence::Find);
    findNext->setShortcut(QKeySequence(Qt::Key_F3));
    goTo->setShortcut(QKeySequence(QStringLiteral("Ctrl+G")));
    menu->exec(m_deviceTable->viewport()->mapToGlobal(pos));
    delete menu;
}

void MainWindow::stubOperation(const QString &what)
{
    statusBar()->showMessage(what + tr(" — not implemented yet"), 4000);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About MiniPro"),
                       tr("<b>%1 %2</b><br/>"
                          "Native Linux port of Xgpro for TL866II / T48 / T56 "
                          "programmers.<br/>Open-source reimplementation.")
                           .arg(QLatin1String(APP_NAME), QLatin1String(APP_VERSION)));
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
