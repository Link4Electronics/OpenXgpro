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
#include "blockfilldialog.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include <QRandomGenerator>

namespace {
bool parseHex(const QString &text, quint64 *out)
{
    bool ok = false;
    *out = text.trimmed().toULongLong(&ok, 16);
    return ok;
}
} // namespace

BlockFillDialog::BlockFillDialog(const QByteArray &buffer, QWidget *parent)
    : QDialog(parent)
    , m_buffer(buffer)
{
    setWindowTitle(tr("Fill Buffer"));
    setModal(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    // Fill area ---------------------------------------------------------
    auto *areaGroup = new QGroupBox(tr("Fill area"), this);
    auto *areaLay = new QHBoxLayout(areaGroup);
    auto *codeRadio = new QRadioButton(tr("Code Memory"), areaGroup);
    auto *dataRadio = new QRadioButton(tr("Data Memory"), areaGroup);
    codeRadio->setChecked(true);
    areaLay->addWidget(codeRadio);
    areaLay->addWidget(dataRadio);
    root->addWidget(areaGroup);

    // Addresses + value -------------------------------------------------
    auto *rangeLay = new QGridLayout;
    auto *startLabel = new QLabel(tr("Start address: 0x"), this);
    m_startEdit = new QLineEdit(QStringLiteral("0"), this);
    auto *endLabel = new QLabel(tr("End address: 0x"), this);
    m_endEdit = new QLineEdit(this);
    auto *valueLabel = new QLabel(tr("Fill value (HEX): 0x"), this);
    m_valueEdit = new QLineEdit(QStringLiteral("FF"), this);
    rangeLay->addWidget(startLabel, 0, 0);
    rangeLay->addWidget(m_startEdit, 0, 1);
    rangeLay->addWidget(endLabel, 1, 0);
    rangeLay->addWidget(m_endEdit, 1, 1);
    rangeLay->addWidget(valueLabel, 2, 0);
    rangeLay->addWidget(m_valueEdit, 2, 1);
    root->addLayout(rangeLay);

    if (!m_buffer.isEmpty())
        m_endEdit->setText(QStringLiteral("%1").arg(m_buffer.size() - 1, 0, 16));

    // Fill mode ---------------------------------------------------------
    auto *modeGroup = new QGroupBox(tr("Fill mode"), this);
    auto *modeLay = new QHBoxLayout(modeGroup);
    const char *labels[] = {"1 byte", "2 bytes", "3 bytes", "4 bytes"};
    for (int i = 0; i < 4; ++i) {
        m_widthRadios[i] = new QRadioButton(tr(labels[i]), modeGroup);
        modeLay->addWidget(m_widthRadios[i]);
    }
    m_randomRadio = new QRadioButton(tr("Random"), modeGroup);
    modeLay->addWidget(m_randomRadio);
    m_widthRadios[0]->setChecked(true);
    root->addWidget(modeGroup);

    // Buttons -----------------------------------------------------------
    auto *buttons = new QHBoxLayout;
    auto *fillBtn = new QPushButton(tr("Fill"), this);
    auto *backBtn = new QPushButton(tr("Back"), this);
    connect(fillBtn, &QPushButton::clicked, this, &BlockFillDialog::onFill);
    connect(backBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttons->addStretch(1);
    buttons->addWidget(fillBtn);
    buttons->addWidget(backBtn);
    root->addLayout(buttons);

    if (!m_buffer.isEmpty())
        m_startEdit->setFocus();
}

void BlockFillDialog::onFill()
{
    Params params;
    quint64 value = 0;
    if (!parseHex(m_startEdit->text(), &params.start)
        || !parseHex(m_endEdit->text(), &params.end)
        || !parseHex(m_valueEdit->text(), &value)) {
        QMessageBox::warning(this, tr("Fill Buffer"),
                             tr("Enter addresses and the fill value in hex."));
        return;
    }
    params.valid = true;
    params.value = static_cast<quint32>(value);
    for (int i = 0; i < 4; ++i) {
        if (m_widthRadios[i] && m_widthRadios[i]->isChecked())
            params.width = i + 1;
    }
    params.random = m_randomRadio->isChecked();

    if (!m_buffer.isEmpty()) {
        if (params.start >= static_cast<quint64>(m_buffer.size())
            || params.end >= static_cast<quint64>(m_buffer.size())) {
            QMessageBox::warning(
                this, tr("Fill Buffer"),
                tr("Addresses are outside the current buffer (%1 bytes).")
                    .arg(m_buffer.size()));
            return;
        }
        if (params.end < params.start) {
            QMessageBox::warning(this, tr("Fill Buffer"),
                                 tr("End address must not be below the start address."));
            return;
        }
    }

    m_params = params;
    accept();
}

BlockFillDialog::Params BlockFillDialog::getFill(const QByteArray &buffer,
                                                 QWidget *parent)
{
    BlockFillDialog dlg(buffer, parent);
    return dlg.exec() == QDialog::Accepted ? dlg.m_params : Params();
}

void BlockFillDialog::apply(QByteArray &data, const Params &params)
{
    if (!params.valid || data.isEmpty())
        return;
    const qsizetype start = qBound<qsizetype>(0, params.start, data.size() - 1);
    const qsizetype end = qBound<qsizetype>(start, params.end, data.size() - 1);

    if (params.random) {
        for (qsizetype i = start; i <= end; ++i)
            data[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
        return;
    }

    const int width = qBound(1, params.width, 4);
    qsizetype i = start;
    while (i <= end) {
        for (int b = 0; b < width && i <= end; ++b, ++i)
            data[i] = static_cast<char>((params.value >> (8 * b)) & 0xFF);
    }
}
