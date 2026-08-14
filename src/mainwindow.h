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
#pragma once

#include "chips.h"
#include "operations.h"
#include "programmer.h"

#include <QByteArray>
#include <QMainWindow>

class QAction;
class QActionGroup;
class QCheckBox;
class QComboBox;
class QEvent;
class QGroupBox;
class QLabel;
class QLineEdit;
class QMenu;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTableWidget;
class QTabWidget;
class QTranslator;
class HexView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void showChipDialog();
    void onChipComboEdited();
    void applySelectedChip(const ChipInfo &chip);
    void showDeviceContextMenu(const QPoint &pos);
    void stubOperation(const QString &what);
    void showAbout();
    void setTheme(int mode);
    void selectProgrammer();
    void loadDataFile();
    void saveDataFile();
    void updateBuffer(const QByteArray &data);
    void onHexCursorMoved(quint64 index);
    void onHexStatusMessage(const QString &message);
    void blockFill();
    void saveBlockAs();

private:
    QWidget *buildDeviceGroup();
    QWidget *buildChipInfoGroup();
    QWidget *buildChipListArea();
    QWidget *buildProgramSettingsGroup();
    QWidget *buildChipConfigGroup();
    void buildMenus();
    void restoreSettings();
    void saveSettings();
    void loadLastChip();
    void updateChecksumLabel();
    void performOperation(OpType op);
    void refreshDeviceStatus();
    void retranslateUi();
    void applyLanguage(const QString &code, bool persist);

    ChipDatabase m_chips;
    QByteArray m_buffer;
    QString m_currentChip;
    QString m_currentAlgorithmFile;
    bool m_deviceConnected = false;
    bool m_uiBuilt = false;
    QTranslator *m_translator = nullptr;
    QActionGroup *m_themeGroup = nullptr;
    QActionGroup *m_languageGroup = nullptr;
    QMenu *m_fileMenu = nullptr;
    QMenu *m_chipMenu = nullptr;
    QMenu *m_opMenu = nullptr;
    QMenu *m_toolsMenu = nullptr;
    QMenu *m_themeMenu = nullptr;
    QMenu *m_helpMenu = nullptr;
    QMenu *m_languageMenu = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_clearBufferAction = nullptr;
    QAction *m_clearAllAction = nullptr;
    QAction *m_quitAction = nullptr;
    QAction *m_findChipAction = nullptr;
    QAction *m_flashIdentifyAction = nullptr;
    QAction *m_readAction = nullptr;
    QAction *m_idAction = nullptr;
    QAction *m_verifyAction = nullptr;
    QAction *m_programAction = nullptr;
    QAction *m_eraseAction = nullptr;
    QAction *m_blankAction = nullptr;
    QAction *m_selectProgAction = nullptr;
    QAction *m_calculatorAction = nullptr;
    QAction *m_selftestAction = nullptr;
    QAction *m_firmwareAction = nullptr;
    QAction *m_adapterAction = nullptr;
    QAction *m_themeSystemAction = nullptr;
    QAction *m_themeLightAction = nullptr;
    QAction *m_themeDarkAction = nullptr;
    QAction *m_aboutAction = nullptr;
    QAction *m_upgradeAction = nullptr;
    QAction *m_languageSystemAction = nullptr;
    QPushButton *m_findChipButton = nullptr;
    QPushButton *m_clearLog = nullptr;
    QPushButton *m_upgrade = nullptr;
    QPushButton *m_chooseData = nullptr;
    QPushButton *m_chooseTarget = nullptr;
    QPushButton *m_readStatus = nullptr;
    QPushButton *m_writeStatus = nullptr;
    QPushButton *m_unprotect = nullptr;
    QCheckBox *m_icspEnable = nullptr;
    QCheckBox *m_saveLog = nullptr;
    QLabel *m_interfaceLabel = nullptr;
    QLabel *m_imaxLabel = nullptr;
    QLabel *m_chipTypeTitle = nullptr;
    QLabel *m_checksumTitle = nullptr;
    QLabel *m_timeTitle = nullptr;
    QLabel *m_pendingFileLabel = nullptr;
    QLabel *m_saveFileLabel = nullptr;
    QLabel *m_rangeLabel = nullptr;
    QLabel *m_blockLabel = nullptr;
    QLabel *m_userIdLabel = nullptr;
    QLabel *m_protectedNotice = nullptr;
    QRadioButton *m_partial = nullptr;
    QRadioButton *m_all = nullptr;
    QGroupBox *m_programSettingsGroup = nullptr;
    QGroupBox *m_spiGroup = nullptr;
    QGroupBox *m_configGroup = nullptr;
    QTabWidget *m_tabs = nullptr;
    QComboBox *m_chipCombo = nullptr;
    QComboBox *m_interfaceCombo = nullptr;
    QComboBox *m_imaxCombo = nullptr;
    QRadioButton *m_bits8 = nullptr;
    QRadioButton *m_bits16 = nullptr;
    QLabel *m_chipTypeLabel = nullptr;
    QLabel *m_checksumLabel = nullptr;
    QLabel *m_timeLabel = nullptr;
    QLineEdit *m_pendingEdit = nullptr;
    QCheckBox *m_pinDetect = nullptr;
    QCheckBox *m_checkId = nullptr;
    QCheckBox *m_eraseFirst = nullptr;
    QCheckBox *m_verifyAfter = nullptr;
    QCheckBox *m_eraseOtp = nullptr;
    QCheckBox *m_blankCheck = nullptr;
    QCheckBox *m_skipWriteFF = nullptr;
    QCheckBox *m_unprotectBefore = nullptr;
    QCheckBox *m_protectAfter = nullptr;
    QTableWidget *m_deviceTable = nullptr;
    HexView *m_hexView = nullptr;
    ProgrammerModel m_model = ProgrammerModel::T56;
};
