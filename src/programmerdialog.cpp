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
