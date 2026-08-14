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
class QLabel;
class QLineEdit;
class QRadioButton;
class QSpinBox;
class QTableWidget;
class QTabWidget;
class HexView;

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

    ChipDatabase m_chips;
    QByteArray m_buffer;
    QString m_currentChip;
    QString m_currentAlgorithmFile;
    bool m_deviceConnected = false;
    QActionGroup *m_themeGroup = nullptr;
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
    QTableWidget *m_deviceTable = nullptr;
    HexView *m_hexView = nullptr;
    ProgrammerModel m_model = ProgrammerModel::T56;
};
