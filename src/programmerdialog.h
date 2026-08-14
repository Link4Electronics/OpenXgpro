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
