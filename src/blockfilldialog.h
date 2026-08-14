#pragma once

#include <QByteArray>
#include <QDialog>

class QLineEdit;
class QRadioButton;

// "Fill Buffer" dialog matching the reference dialog (resource 160):
// a fill area (Code/Data memory), start/end addresses in hex, a fill value
// in hex and a fill mode of 1..4 bytes or random.
class BlockFillDialog : public QDialog
{
    Q_OBJECT

public:
    struct Params {
        bool valid = false;
        quint64 start = 0;
        quint64 end = 0;
        quint32 value = 0x000000FF;
        int width = 1;      // 1, 2, 3, 4 bytes per cell
        bool random = false;
    };

    explicit BlockFillDialog(const QByteArray &buffer, QWidget *parent = nullptr);

    static Params getFill(const QByteArray &buffer, QWidget *parent);

    // Applies the fill pattern to `data` over [start, end] (clamped).
    static void apply(QByteArray &data, const Params &params);

private slots:
    void onFill();

private:
    QByteArray m_buffer;
    Params m_params;
    QLineEdit *m_startEdit = nullptr;
    QLineEdit *m_endEdit = nullptr;
    QLineEdit *m_valueEdit = nullptr;
    QRadioButton *m_widthRadios[4] = {nullptr, nullptr, nullptr, nullptr};
    QRadioButton *m_randomRadio = nullptr;
};
