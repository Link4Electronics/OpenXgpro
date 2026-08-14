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

#include "programmer.h"

#include <QDialog>

class QRadioButton;

// Demo-mode model chooser shown when no programmer is detected. Matches the
// reference "System Demo(Programmer not Found)" dialog (resource 211): a
// "Please select model:" label, three model radio buttons and an OK button.
class ProgrammerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProgrammerDialog(ProgrammerModel initial, QWidget *parent = nullptr);

    ProgrammerModel selectedModel() const;

    static ProgrammerModel getModel(ProgrammerModel initial, QWidget *parent);

private:
    QRadioButton *m_radios[3] = {nullptr, nullptr, nullptr};
};
