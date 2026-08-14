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

#include <QDialog>
#include <QMap>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QRadioButton;

class ChipDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChipDialog(const ChipDatabase &db, QWidget *parent = nullptr);

    const ChipInfo &selectedChip() const { return m_selected; }
    bool hasSelection() const { return m_hasSelection; }

    static ChipInfo getChip(const ChipDatabase &db, QWidget *parent = nullptr);

private slots:
    void rebuildVendors();
    void onVendorChanged(int row);
    void onChipActivated(QListWidgetItem *item);

private:
    void rebuildChips();

    const ChipDatabase &m_db;
    QString m_category = QStringLiteral("All");
    QLineEdit *m_searchEdit = nullptr;
    QRadioButton *m_exactRadio = nullptr;
    QListWidget *m_vendorList = nullptr;
    QListWidget *m_chipList = nullptr;
    QLabel *m_manufactorLabel = nullptr;
    QLabel *m_totalLabel = nullptr;
    QMap<QString, QString> m_categoryKeys;
    ChipInfo m_selected;
    bool m_hasSelection = false;
};
