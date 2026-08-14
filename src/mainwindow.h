#pragma once

#include "chips.h"

#include <QMainWindow>

class QAction;
class QActionGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QTableWidget;
class QTabWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void showChipDialog();
    void onChipComboEdited();
    void applySelectedChip(const ChipInfo &chip);
    void showDeviceContextMenu(const QPoint &pos);
    void stubOperation(const QString &what);
    void showAbout();
    void setTheme(int mode);

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

    ChipDatabase m_chips;
    QActionGroup *m_themeGroup = nullptr;
    QComboBox *m_chipCombo = nullptr;
    QComboBox *m_interfaceCombo = nullptr;
    QComboBox *m_imaxCombo = nullptr;
    QLabel *m_chipTypeLabel = nullptr;
    QLabel *m_checksumLabel = nullptr;
    QLabel *m_timeLabel = nullptr;
    QCheckBox *m_pinDetect = nullptr;
    QCheckBox *m_checkId = nullptr;
    QCheckBox *m_eraseFirst = nullptr;
    QCheckBox *m_verifyAfter = nullptr;
    QCheckBox *m_eraseOtp = nullptr;
    QCheckBox *m_blankCheck = nullptr;
    QCheckBox *m_skipWriteFF = nullptr;
    QTableWidget *m_deviceTable = nullptr;
};
