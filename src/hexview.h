#pragma once

#include <QAbstractScrollArea>
#include <QByteArray>

class QContextMenuEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;

// Hex buffer view matching the reference's CmyHexEdit + CEditAscii two-pane
// editor: an address gutter, a hex byte/word column and (8-bit mode only) a
// printable-ASCII column. Byte-accurate cursor, drag selection, keyboard
// navigation, Go-to-address and pattern search.
class HexView : public QAbstractScrollArea
{
    Q_OBJECT

public:
    explicit HexView(QWidget *parent = nullptr);

    void setData(const QByteArray &data);
    const QByteArray &data() const { return m_data; }
    void clear();

    bool wordMode() const { return m_wordMode; }
    void setWordMode(bool words);

    quint64 baseAddress() const { return m_base; }
    void setBaseAddress(quint64 base);

    quint64 cursorIndex() const { return m_cursor; }
    void setCursorIndex(quint64 index);

    QByteArray selectedBytes() const;
    void copySelectionAsHex();
    void fill(quint8 value, bool onlySelection);

    bool find(const QByteArray &needle, quint64 from, bool wrap, quint64 *foundAt);
    bool findNext();

signals:
    void cursorChanged(quint64 index);
    void selectionChanged();
    void statusMessage(const QString &message);

public slots:
    void showGotoDialog();
    void showFindDialog();
    void findNextFromCursor();

public:
    void gotoAddress(quint64 address);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void recalcMetrics();
    void updateScrollBars();
    void ensureCursorVisible();
    void setCursorIndexInternal(quint64 index, bool extend);
    int bytesPerRow() const { return m_wordMode ? 8 : 16; }
    int addressDigits() const;
    QString addressText(quint64 index) const;
    quint64 indexAt(const QPoint &pos) const;
    void drawByteCell(QPainter &p, quint64 index, int colX, int y);

    QByteArray m_data;
    bool m_wordMode = false;
    quint64 m_base = 0;
    quint64 m_cursor = 0;
    qint64 m_anchor = -1;

    int m_charW = 0;
    int m_rowH = 0;
    int m_addrW = 0;
    int m_hexX = 0;
    int m_asciiX = 0;
    int m_contentW = 0;

    QByteArray m_findNeedle;
};
