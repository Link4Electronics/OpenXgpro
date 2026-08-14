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
#include "programmerdialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

ProgrammerDialog::ProgrammerDialog(ProgrammerModel initial, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("System Demo (Programmer not Found)"));
    setModal(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    auto *label = new QLabel(tr("Please select model:"), this);
    root->addWidget(label);

    const auto names = Programmer::modelNames();
    for (int i = 0; i < names.size(); ++i) {
        auto *radio = new QRadioButton(names.at(i), this);
        m_radios[i] = radio;
        radio->setChecked(static_cast<ProgrammerModel>(i) == initial);
        root->addWidget(radio);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    root->addWidget(buttons);
}

ProgrammerModel ProgrammerDialog::selectedModel() const
{
    for (int i = 0; i < 3; ++i) {
        if (m_radios[i] && m_radios[i]->isChecked())
            return static_cast<ProgrammerModel>(i);
    }
    return ProgrammerModel::T56;
}

ProgrammerModel ProgrammerDialog::getModel(ProgrammerModel initial, QWidget *parent)
{
    ProgrammerDialog dlg(initial, parent);
    return dlg.exec() == QDialog::Accepted ? dlg.selectedModel() : initial;
}
