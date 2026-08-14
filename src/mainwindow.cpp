#include "mainwindow.h"

#include "theme.h"
#include "version.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
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
    statusBar()->showMessage(QStringLiteral("No device connected"));

    restoreSettings();
}

QWidget *MainWindow::buildDeviceGroup()
{
    auto *box = new QGroupBox(tr("芯片选择"));
    auto *lay = new QGridLayout(box);
    lay->setHorizontalSpacing(6);
    lay->setVerticalSpacing(4);

    auto *findChipButton = new QPushButton(tr("查找选择芯片(&S)"), box);
    connect(findChipButton, &QPushButton::clicked, this, &MainWindow::showChipDialog);

    m_chipCombo = new QComboBox(box);
    m_chipCombo->setEditable(true);
    m_chipCombo->addItem(QStringLiteral("27C512A"));

    auto *interfaceLabel = new QLabel(tr("选择编程接口:"), box);
    m_interfaceCombo = new QComboBox(box);
    m_interfaceCombo->addItem(QStringLiteral("40PIN锁紧座"));
    m_interfaceCombo->addItem(QStringLiteral("ICSP串行接口"));

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

    auto *upgrade = new QPushButton(tr("Upgrade is avaliable"), box);
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
    auto *box = new QGroupBox(tr("芯片信息(No Project opened)"));
    auto *lay = new QGridLayout(box);
    lay->setHorizontalSpacing(8);
    lay->setVerticalSpacing(4);

    auto *typeTitle = new QLabel(tr("芯片类型:"), box);
    m_chipTypeLabel = new QLabel(tr("unkown"), box);
    auto *sumTitle = new QLabel(tr("累加和:"), box);
    m_checksumLabel = new QLabel(QStringLiteral(" 0000 0000"), box);
    auto *timeTitle = new QLabel(tr("时间:"), box);
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
    m_deviceTable->setHorizontalHeaderLabels({tr("芯片类型"), tr("芯片型号")});
    m_deviceTable->verticalHeader()->setVisible(false);
    m_deviceTable->horizontalHeader()->setStretchLastSection(true);
    m_deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *tabs = new QTabWidget(box);
    tabs->addTab(m_deviceTable, tr("设备列表"));

    auto *row = new QHBoxLayout;
    auto *pendingFile = new QLabel(tr(" 待写入文件:"), box);
    auto *pendingEdit = new QLineEdit(box);
    auto *chooseData = new QPushButton(tr("选择数据文件"), box);
    auto *saveFile = new QLabel(tr(" 保存到文件:"), box);
    auto *saveEdit = new QLineEdit(box);
    auto *chooseTarget = new QPushButton(tr("选择目标文件"), box);
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
    auto *box = new QGroupBox(tr("编程设置"));
    auto *lay = new QGridLayout(box);
    lay->setHorizontalSpacing(8);
    lay->setVerticalSpacing(4);

    m_pinDetect = new QCheckBox(tr("Pin Detect"), box);
    m_checkId = new QCheckBox(tr("检查ID"), box);
    m_eraseFirst = new QCheckBox(tr("编程前先擦除"), box);
    m_verifyAfter = new QCheckBox(tr("编程后校验"), box);
    m_eraseOtp = new QCheckBox(tr("EraseOTP"), box);
    m_blankCheck = new QCheckBox(tr("编程前查空"), box);
    m_skipWriteFF = new QCheckBox(tr("跳过写0xFF"), box);

    auto *rangeLabel = new QLabel(tr("编程范围:"), box);
    auto *partial = new QRadioButton(tr("部分"), box);
    auto *all = new QRadioButton(tr("全部"), box);
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

    auto *spi = new QGroupBox(tr("SPI EEPROM(25/35/95系列) 状态位"), box);
    auto *spiLay = new QHBoxLayout(spi);
    auto *readStatus = new QPushButton(tr("读状态"), spi);
    auto *writeStatus = new QPushButton(tr("写状态"), spi);
    connect(readStatus, &QPushButton::clicked, this,
            [this] { stubOperation(tr("Read SPI status")); });
    connect(writeStatus, &QPushButton::clicked, this,
            [this] { stubOperation(tr("Write SPI status")); });
    spiLay->addWidget(readStatus);
    spiLay->addWidget(writeStatus);
    spiLay->addStretch(1);

    auto *config = new QGroupBox(tr("芯片配置信息"), box);
    auto *confLay = new QGridLayout(config);
    auto *userIdLabel = new QLabel(tr("USERID:"), config);
    auto *userIdEdit = new QLineEdit(config);
    auto *unprotectBefore = new QCheckBox(tr("编程前取消写保护"), config);
    auto *protectAfter = new QCheckBox(tr("编程后加写保护"), config);
    auto *unprotect = new QPushButton(tr("解保护"), config);
    auto *protectedNotice = new QLabel(tr("保护模式，部分功能已被禁用!"), config);
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
    auto *fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    auto *openAction = fileMenu->addAction(tr("打开文件(&O)..."));
    openAction->setShortcut(QKeySequence::Open);
    auto *saveAction = fileMenu->addAction(tr("保存到文件(&S)"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(openAction, &QAction::triggered, this, [this] { stubOperation(tr("Open file")); });
    connect(saveAction, &QAction::triggered, this, [this] { stubOperation(tr("Save file")); });
    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(tr("退出(&X)"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &MainWindow::close);

    auto *opMenu = menuBar()->addMenu(tr("操作(&O)"));
    auto *readAction = opMenu->addAction(tr("读芯片内容(&R)"));
    auto *idAction = opMenu->addAction(tr("芯片ID识别(&I)"));
    auto *verifyAction = opMenu->addAction(tr("数据校验(&V)"));
    auto *programAction = opMenu->addAction(tr("芯片编程(&P)"));
    auto *eraseAction = opMenu->addAction(tr("擦除芯片内容(&E)"));
    auto *blankAction = opMenu->addAction(tr("芯片查空(&B)"));
    connect(readAction, &QAction::triggered, this, [this] { stubOperation(tr("Read")); });
    connect(idAction, &QAction::triggered, this, [this] { stubOperation(tr("Device ID")); });
    connect(verifyAction, &QAction::triggered, this, [this] { stubOperation(tr("Verify")); });
    connect(programAction, &QAction::triggered, this, [this] { stubOperation(tr("Program")); });
    connect(eraseAction, &QAction::triggered, this, [this] { stubOperation(tr("Erase")); });
    connect(blankAction, &QAction::triggered, this, [this] { stubOperation(tr("Blank check")); });

    auto *viewMenu = menuBar()->addMenu(tr("主题(&T)"));
    m_themeGroup = new QActionGroup(this);
    m_themeGroup->setExclusive(true);
    auto addThemeAction = [this, viewMenu](Theme::Mode mode, const QString &text) {
        auto *action = viewMenu->addAction(text);
        action->setCheckable(true);
        action->setData(static_cast<int>(mode));
        m_themeGroup->addAction(action);
        return action;
    };
    auto *systemAction = addThemeAction(Theme::Mode::System, tr("跟随系统(&S)"));
    auto *lightAction = addThemeAction(Theme::Mode::Light, tr("浅色(&L)"));
    auto *darkAction = addThemeAction(Theme::Mode::Dark, tr("深色(&D)"));
    systemAction->setChecked(true);
    connect(m_themeGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        setTheme(action->data().toInt());
    });

    auto *helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    auto *aboutAction = helpMenu->addAction(tr("关于 MiniPro(&A)"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::showChipDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("查找选择芯片"));
    auto *lay = new QVBoxLayout(&dlg);
    lay->addWidget(new QLabel(tr("在下方列表中查找芯片（暂未实现列表加载）"), &dlg));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    lay->addWidget(buttons);
    dlg.resize(420, 300);
    dlg.exec();
}

void MainWindow::stubOperation(const QString &what)
{
    statusBar()->showMessage(what + tr(" — not implemented yet"), 4000);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("关于 MiniPro"),
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
    statusBar()->showMessage(tr("主题: %1").arg(Theme::modeName(theme)), 3000);
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}
