#include "hexview.h"

#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

#include <algorithm>
#include <cmath>

namespace {
constexpr int kHexGroupGap = 2;     // chars between 4-byte groups
constexpr int kHexAsciiGap = 12;    // chars between hex and ascii columns
constexpr int kRowPadding = 4;      // px top/bottom in a row
constexpr int kGutterPad = 6;       // px left/right of the address gutter
constexpr int kBorder = 2;          // 3D sunken border thickness
} // namespace

HexView::HexView(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(12);
    setFont(f);

    setFocusPolicy(Qt::StrongFocus);
    setFrameShape(QFrame::NoFrame);
    viewport()->setCursor(Qt::IBeamCursor);
    verticalScrollBar()->setSingleStep(1);
    recalcMetrics();
}

void HexView::setData(const QByteArray &data)
{
    m_data = data;
    m_cursor = 0;
    m_anchor = -1;
    m_findNeedle.clear();
    recalcMetrics();
    updateScrollBars();
    viewport()->update();
    emit cursorChanged(m_cursor);
}

void HexView::clear()
{
    setData(QByteArray());
}

void HexView::setWordMode(bool words)
{
    if (m_wordMode == words)
        return;
    m_wordMode = words;
    m_cursor = std::min(m_cursor,
                        static_cast<quint64>(std::max(0, static_cast<int>(m_data.size()) - 1)));
    m_anchor = -1;
    recalcMetrics();
    updateScrollBars();
    ensureCursorVisible();
    viewport()->update();
}

void HexView::setBaseAddress(quint64 base)
{
    if (m_base == base)
        return;
    m_base = base;
    recalcMetrics();
    updateScrollBars();
    viewport()->update();
}

void HexView::setCursorIndex(quint64 index)
{
    if (m_data.isEmpty())
        return;
    m_cursor = std::min(index, static_cast<quint64>(m_data.size() - 1));
    m_anchor = -1;
    ensureCursorVisible();
    viewport()->update();
    emit cursorChanged(m_cursor);
}

void HexView::recalcMetrics()
{
    const QFontMetrics fm = fontMetrics();
    m_charW = fm.horizontalAdvance(QStringLiteral("0"));
    m_rowH = fm.height() + kRowPadding * 2;

    const int bytes = bytesPerRow();
    const int hexChars = m_wordMode ? bytes * 4 : bytes * 2;
    const int groups = m_wordMode ? 0 : bytes / 4;
    const int hexW = (hexChars + groups * kHexGroupGap) * m_charW;
    const int asciiW = bytes * m_charW;

    const int digits = addressDigits();
    m_addrW = digits * m_charW + kGutterPad * 2;
    m_hexX = kBorder + m_addrW + kGutterPad;
    m_asciiX = m_hexX + hexW + kHexAsciiGap * m_charW;
    m_contentW = m_asciiX + asciiW + kGutterPad + kBorder;
}

int HexView::addressDigits() const
{
    const quint64 last = m_data.isEmpty() ? m_base : m_base + m_data.size() - 1;
    int digits = 8;
    if (last > 0) {
        int d = 1;
        quint64 v = last;
        while (v > 0) {
            v >>= 4;
            ++d;
        }
        digits = std::max(digits, d);
    }
    return digits;
}

QString HexView::addressText(quint64 index) const
{
    return QStringLiteral("%1").arg(m_base + index, addressDigits(), 16,
                                    QLatin1Char('0')).toUpper();
}

void HexView::updateScrollBars()
{
    const int rows = m_data.isEmpty()
                         ? 0
                         : (m_data.size() + bytesPerRow() - 1) / bytesPerRow();
    const int contentH = rows * m_rowH;
    const int visibleH = viewport()->height();
    const int maxV = std::max(0, contentH - visibleH);
    verticalScrollBar()->setRange(0, maxV);
    verticalScrollBar()->setPageStep(std::max(1, visibleH / m_rowH));

    const int maxH = std::max(0, m_contentW - viewport()->width());
    horizontalScrollBar()->setRange(0, maxH);
    horizontalScrollBar()->setPageStep(std::max(1, m_contentW));
}

void HexView::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollBars();
}

QString printableChar(quint8 byte)
{
    return byte >= 0x20 && byte <= 0x7e ? QString(QChar(byte)) : QStringLiteral(".");
}

void HexView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(viewport());
    const QRect vr = viewport()->rect();

    // Classic sunken 3D border (matches the reference editor frame).
    p.setPen(QPen(palette().color(QPalette::Dark), 1));
    p.drawRect(vr.adjusted(0, 0, -1, -1));
    p.setPen(QPen(palette().color(QPalette::Light), 1));
    p.drawRect(vr.adjusted(1, 1, -2, -2));

    const QRect content = vr.adjusted(kBorder, kBorder, -kBorder, -kBorder);
    p.fillRect(content, palette().color(QPalette::Base));

    if (m_data.isEmpty()) {
        p.setPen(palette().color(QPalette::PlaceholderText));
        p.drawText(content, Qt::AlignCenter, tr("Buffer is empty — load a file to view it."));
        return;
    }

    const int yOff = verticalScrollBar()->value();
    const int xOff = horizontalScrollBar()->value();
    const int bytes = bytesPerRow();
    const int baseX = kBorder;
    const int baseY = kBorder;

    // Determine selected range.
    qint64 selStart = -1, selEnd = -1;
    if (m_anchor >= 0) {
        selStart = std::min(m_anchor, static_cast<qint64>(m_cursor));
        selEnd = std::max(m_anchor, static_cast<qint64>(m_cursor));
    }

    const int firstRow = std::max(0, yOff / m_rowH);
    const int lastRow = (yOff + vr.height()) / m_rowH;
    const int lastIndex = m_data.size() - 1;

    p.setFont(font());
    for (int row = firstRow; row <= lastRow; ++row) {
        const int y = baseY + row * m_rowH - yOff + kRowPadding;
        const int first = row * bytes;
        if (first > lastIndex)
            break;

        // Address gutter.
        p.setPen(palette().color(QPalette::Dark));
        p.drawText(baseX + m_addrW - kGutterPad - addressDigits() * m_charW - xOff, y,
                   m_addrW - kGutterPad, m_rowH, Qt::AlignVCenter | Qt::AlignRight,
                   addressText(first));

        // Hex column.
        for (int col = 0; col < bytes; ++col) {
            const int index = first + col;
            if (index > lastIndex)
                break;
            const int groups = m_wordMode ? 0 : bytes / 4;
            int colsBefore = col;
            if (!m_wordMode)
                colsBefore += col / 4 * kHexGroupGap;
            else
                colsBefore = col * 5;
            const int cellW = m_wordMode ? 5 * m_charW : 2 * m_charW + m_charW;
            const int x = m_hexX + colsBefore * m_charW - xOff;

            const bool selected = selStart >= 0 && index >= selStart && index <= selEnd;
            const bool cursorCell = index == static_cast<qint64>(m_cursor);

            QRect cell(x, y - kRowPadding, cellW, m_rowH);
            if (selected) {
                p.fillRect(cell, palette().color(QPalette::Highlight));
                p.setPen(palette().color(QPalette::HighlightedText));
            } else if (cursorCell) {
                p.fillRect(cell, palette().color(QPalette::Highlight).darker(115));
                p.setPen(palette().color(QPalette::HighlightedText));
            } else {
                p.setPen(palette().color(QPalette::Text));
            }

            if (m_wordMode) {
                const quint16 word = static_cast<quint16>(m_data.at(index))
                                     | (index + 1 <= lastIndex
                                            ? static_cast<quint16>(m_data.at(index + 1)) << 8
                                            : 0);
                p.drawText(cell, Qt::AlignCenter,
                           QStringLiteral("%1").arg(word, 4, 16, QLatin1Char('0')).toUpper());
            } else {
                p.drawText(cell, Qt::AlignCenter,
                           QStringLiteral("%1").arg(
                               static_cast<quint8>(m_data.at(index)), 2, 16,
                               QLatin1Char('0')).toUpper());
            }
        }

        // ASCII column (8-bit mode only).
        if (!m_wordMode) {
            p.setPen(palette().color(QPalette::Text));
            for (int col = 0; col < bytes; ++col) {
                const int index = first + col;
                if (index > lastIndex)
                    break;
                const bool selected =
                    selStart >= 0 && index >= selStart && index <= selEnd;
                const int x = m_asciiX + col * m_charW - xOff;
                QRect cell(x, y - kRowPadding, m_charW, m_rowH);
                if (selected) {
                    p.fillRect(cell, palette().color(QPalette::Highlight));
                    p.setPen(palette().color(QPalette::HighlightedText));
                }
                p.drawText(cell, Qt::AlignCenter,
                           printableChar(static_cast<quint8>(m_data.at(index))));
            }
        }
    }

    // Caret.
    if (!m_data.isEmpty() && hasFocus()) {
        const int row = m_cursor / bytes;
        const int col = m_cursor % bytes;
        const int y = baseY + row * m_rowH - yOff;
        int colsBefore = col;
        if (!m_wordMode)
            colsBefore += col / 4 * kHexGroupGap;
        else
            colsBefore = col * 5;
        const int cellW = m_wordMode ? 5 * m_charW : 2 * m_charW + m_charW;
        const int x = m_hexX + colsBefore * m_charW - xOff;
        const int caretX = m_wordMode ? x + 2 * m_charW + 1 : x + m_charW + 1;
        p.setPen(palette().color(QPalette::Highlight));
        p.drawLine(caretX, y, caretX, y + m_rowH);
    }
}

quint64 HexView::indexAt(const QPoint &pos) const
{
    if (m_data.isEmpty())
        return 0;
    const int xOff = horizontalScrollBar()->value();
    const int yOff = verticalScrollBar()->value();
    const int row = (pos.y() - kBorder + yOff) / m_rowH;
    const int bytes = bytesPerRow();
    const int x = pos.x() - kBorder + xOff;

    int col = -1;
    if (x >= m_hexX && !m_wordMode) {
        int cx = x - m_hexX;
        const int cellW = 3 * m_charW;
        col = cx / cellW;
        // account for group gaps
        if (col >= 0 && col < bytes) {
            int eff = col - col / 4 * kHexGroupGap;
            if (eff < 0)
                eff = 0;
            col = eff;
        }
    } else if (x >= m_hexX && m_wordMode) {
        int cx = x - m_hexX;
        col = cx / (5 * m_charW);
        col = std::min(col, bytes - 1);
        col *= 2; // byte column within the word
    } else if (x >= m_asciiX && !m_wordMode) {
        col = (x - m_asciiX) / m_charW;
    }

    if (col < 0 || col >= bytes)
        return m_cursor;
    quint64 index = static_cast<quint64>(row) * bytes + col;
    index = std::min(index, static_cast<quint64>(m_data.size() - 1));
    return index;
}

void HexView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_anchor = indexAt(event->pos());
        setCursorIndexInternal(indexAt(event->pos()), false);
    }
    QAbstractScrollArea::mousePressEvent(event);
}

void HexView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        setCursorIndexInternal(indexAt(event->pos()), true);
    QAbstractScrollArea::mouseMoveEvent(event);
}

void HexView::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractScrollArea::mouseReleaseEvent(event);
}

void HexView::keyPressEvent(QKeyEvent *event)
{
    if (m_data.isEmpty()) {
        QAbstractScrollArea::keyPressEvent(event);
        return;
    }
    const int bytes = bytesPerRow();
    const int visibleRows = std::max(1, viewport()->height() / m_rowH);
    quint64 next = m_cursor;
    bool handled = true;
    switch (event->key()) {
    case Qt::Key_Left:
        next = m_cursor > 0 ? m_cursor - 1 : 0;
        break;
    case Qt::Key_Right:
        next = std::min(m_cursor + 1, static_cast<quint64>(m_data.size() - 1));
        break;
    case Qt::Key_Up:
        next = m_cursor >= static_cast<quint64>(bytes) ? m_cursor - bytes : 0;
        break;
    case Qt::Key_Down:
        next = std::min(m_cursor + bytes, static_cast<quint64>(m_data.size() - 1));
        break;
    case Qt::Key_PageUp:
        next = m_cursor >= static_cast<quint64>(visibleRows * bytes)
                   ? m_cursor - visibleRows * bytes
                   : 0;
        break;
    case Qt::Key_PageDown:
        next = std::min(m_cursor + visibleRows * bytes,
                        static_cast<quint64>(m_data.size() - 1));
        break;
    case Qt::Key_Home:
        next = event->modifiers() & Qt::ControlModifier
                   ? 0
                   : (m_cursor / bytes) * bytes;
        break;
    case Qt::Key_End:
        if (event->modifiers() & Qt::ControlModifier)
            next = m_data.size() - 1;
        else {
            const quint64 rowStart = (m_cursor / bytes) * bytes;
            next = std::min(rowStart + bytes - 1,
                            static_cast<quint64>(m_data.size() - 1));
        }
        break;
    case Qt::Key_G:
        if (event->modifiers() & Qt::ControlModifier) {
            showGotoDialog();
            return;
        }
        handled = false;
        break;
    case Qt::Key_F:
        if (event->modifiers() & Qt::ControlModifier) {
            showFindDialog();
            return;
        }
        handled = false;
        break;
    case Qt::Key_F3:
        findNextFromCursor();
        return;
    default:
        handled = false;
        break;
    }
    if (handled)
        setCursorIndexInternal(next, event->modifiers() & Qt::ShiftModifier);
    else
        QAbstractScrollArea::keyPressEvent(event);
}

void HexView::wheelEvent(QWheelEvent *event)
{
    QAbstractScrollArea::wheelEvent(event);
}

void HexView::setCursorIndexInternal(quint64 index, bool extend)
{
    const quint64 clamped = std::min(index, static_cast<quint64>(m_data.size() - 1));
    const bool changed = clamped != m_cursor;
    if (extend) {
        if (m_anchor < 0)
            m_anchor = m_cursor;
    } else {
        m_anchor = -1;
    }
    m_cursor = clamped;
    ensureCursorVisible();
    viewport()->update();
    if (changed) {
        emit cursorChanged(m_cursor);
        emit selectionChanged();
    }
}

void HexView::ensureCursorVisible()
{
    if (m_data.isEmpty())
        return;
    const int bytes = bytesPerRow();
    const int row = m_cursor / bytes;
    const int col = m_cursor % bytes;
    const int y = row * m_rowH;
    int colsBefore = col;
    if (!m_wordMode)
        colsBefore += col / 4 * kHexGroupGap;
    else
        colsBefore = col * 5;
    const int x = m_hexX + colsBefore * m_charW;

    const int yOff = verticalScrollBar()->value();
    if (y < yOff)
        verticalScrollBar()->setValue(y);
    else if (y + m_rowH > yOff + viewport()->height())
        verticalScrollBar()->setValue(y + m_rowH - viewport()->height());

    const int xOff = horizontalScrollBar()->value();
    if (x < xOff)
        horizontalScrollBar()->setValue(x);
    else if (x + m_charW * 3 > xOff + viewport()->width())
        horizontalScrollBar()->setValue(x + m_charW * 3 - viewport()->width());
}

QByteArray HexView::selectedBytes() const
{
    if (m_anchor < 0)
        return QByteArray();
    const quint64 a = std::min(m_anchor, static_cast<qint64>(m_cursor));
    const quint64 b = std::max(m_anchor, static_cast<qint64>(m_cursor));
    if (a > b || a >= static_cast<quint64>(m_data.size()))
        return QByteArray();
    return m_data.mid(a, b - a + 1);
}

void HexView::copySelectionAsHex()
{
    const QByteArray sel = selectedBytes();
    if (sel.isEmpty())
        return;
    QString hex;
    for (int i = 0; i < sel.size(); ++i) {
        if (i)
            hex += QLatin1Char(' ');
        hex += QStringLiteral("%1").arg(static_cast<quint8>(sel.at(i)), 2, 16,
                                        QLatin1Char('0')).toUpper();
    }
    QApplication::clipboard()->setText(hex);
}

void HexView::fill(quint8 value, bool onlySelection)
{
    if (m_data.isEmpty())
        return;
    if (onlySelection && m_anchor < 0)
        return;
    if (onlySelection) {
        const quint64 a = std::min(m_anchor, static_cast<qint64>(m_cursor));
        const quint64 b = std::max(m_anchor, static_cast<qint64>(m_cursor));
        std::fill(m_data.begin() + a, m_data.begin() + b + 1, value);
    } else {
        std::fill(m_data.begin(), m_data.end(), value);
    }
    viewport()->update();
    emit selectionChanged();
}

bool HexView::find(const QByteArray &needle, quint64 from, bool wrap, quint64 *foundAt)
{
    if (needle.isEmpty() || m_data.isEmpty())
        return false;
    const qint64 start = std::min(static_cast<qint64>(from),
                                  static_cast<qint64>(m_data.size() - 1));
    const qint64 hit = m_data.indexOf(needle, start);
    if (hit >= 0) {
        if (foundAt)
            *foundAt = static_cast<quint64>(hit);
        return true;
    }
    if (wrap) {
        const qint64 wrapped = m_data.indexOf(needle, 0);
        if (wrapped >= 0) {
            if (foundAt)
                *foundAt = static_cast<quint64>(wrapped);
            return true;
        }
    }
    return false;
}

bool HexView::findNext()
{
    if (m_findNeedle.isEmpty())
        return false;
    quint64 hit = 0;
    const quint64 from = m_cursor + 1;
    if (find(m_findNeedle, from, true, &hit)) {
        setCursorIndex(hit);
        return true;
    }
    return false;
}

void HexView::findNextFromCursor()
{
    if (findNext()) {
        m_anchor = m_cursor + static_cast<qint64>(m_findNeedle.size()) - 1;
        viewport()->update();
        emit selectionChanged();
        statusMessage(tr("Found at 0x%1").arg(m_base + m_cursor, addressDigits(), 16,
                                              QLatin1Char('0')).toUpper());
    } else {
        statusMessage(tr("Pattern not found"));
    }
}

void HexView::showGotoDialog()
{
    bool ok = false;
    const quint64 maxAddr = m_base + m_data.size() - 1;
    const quint64 current = m_base + m_cursor;
    const quint64 addr = QInputDialog::getInt(
        this, tr("Go to Address"), tr("Address (hex): 0x"), current, 0,
        maxAddr > 0 ? maxAddr : INT_MAX, 16, &ok);
    if (ok)
        gotoAddress(addr);
}

void HexView::gotoAddress(quint64 address)
{
    if (m_data.isEmpty() || address < m_base)
        return;
    const quint64 index = address - m_base;
    if (index >= static_cast<quint64>(m_data.size()))
        return;
    setCursorIndex(index);
}

void HexView::showFindDialog()
{
    bool ok = false;
    const QString text = QInputDialog::getText(
        this, tr("Find"), tr("Pattern (hex bytes or text, e.g. FF 00 or 'ABC'):"),
        QLineEdit::Normal, m_findNeedle.isEmpty()
                               ? QStringLiteral("FF FF")
                               : QString::fromLatin1(m_findNeedle.toHex(' ').toUpper()),
        &ok);
    if (!ok)
        return;

    const QString trimmed = text.trimmed();
    if (trimmed.startsWith(QLatin1Char('\'')) && trimmed.endsWith(QLatin1Char('\'')))
        m_findNeedle = trimmed.mid(1, trimmed.size() - 2).toUtf8();
    else {
        const QStringList parts = trimmed.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        QByteArray bytes;
        bool valid = true;
        for (const QString &part : parts) {
            if (part.size() != 2 || !part[0].isDigit() || !part[1].isDigit()) {
                valid = false;
                break;
            }
            bytes.append(static_cast<char>(part.toInt(nullptr, 16)));
        }
        if (valid && !bytes.isEmpty())
            m_findNeedle = bytes;
        else
            m_findNeedle = trimmed.toUtf8();
    }
    if (!m_findNeedle.isEmpty())
        findNextFromCursor();
    else
        statusMessage(tr("Nothing to search for"));
}
