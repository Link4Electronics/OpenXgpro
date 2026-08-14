#include "chipdialog.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QVBoxLayout>

ChipDialog::ChipDialog(const ChipDatabase &db, QWidget *parent)
    : QDialog(parent)
    , m_db(db)
{
    setWindowTitle(tr("Find & Select Chip"));

    const QStringList keys = ChipDatabase::categories();
    const QStringList labels = ChipDatabase::categoryLabels();
    for (int i = 0; i < keys.size(); ++i)
        m_categoryKeys.insert(labels.at(i), keys.at(i));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // Search group -----------------------------------------------------
    auto *searchGroup = new QGroupBox(tr("Search chip"), this);
    auto *searchLay = new QHBoxLayout(searchGroup);
    m_searchEdit = new QLineEdit(searchGroup);
    m_searchEdit->setPlaceholderText(tr("Type a chip name to search..."));
    auto *exactLabel = new QLabel(tr("Match:"), searchGroup);
    m_exactRadio = new QRadioButton(tr("Exact"), searchGroup);
    auto *fuzzyRadio = new QRadioButton(tr("Fuzzy"), searchGroup);
    m_exactRadio->setChecked(true);
    searchLay->addWidget(m_searchEdit, 1);
    searchLay->addWidget(exactLabel);
    searchLay->addWidget(m_exactRadio);
    searchLay->addWidget(fuzzyRadio);
    root->addWidget(searchGroup);

    // Category group ---------------------------------------------------
    auto *categoryGroup = new QGroupBox(tr("Chip type"), this);
    auto *catLay = new QHBoxLayout(categoryGroup);
    auto addCategory = [this, catLay](const QString &label, const QString &key) {
        auto *radio = new QRadioButton(label, this);
        radio->setProperty("category", key);
        catLay->addWidget(radio);
        return radio;
    };
    auto *allRadio = addCategory(tr("All types"), QStringLiteral("All"));
    allRadio->setChecked(true);
    for (const QString &label : labels)
        addCategory(label, m_categoryKeys.value(label));
    root->addWidget(categoryGroup);

    // Vendor + chip lists ---------------------------------------------
    auto *listsLay = new QHBoxLayout;
    auto *vendorGroup = new QGroupBox(tr("Manufacturers"), this);
    auto *vendorLay = new QVBoxLayout(vendorGroup);
    m_vendorList = new QListWidget(vendorGroup);
    vendorLay->addWidget(m_vendorList);

    auto *chipGroup = new QGroupBox(tr("Devices"), this);
    auto *chipLay = new QVBoxLayout(chipGroup);
    m_chipList = new QListWidget(chipGroup);
    chipLay->addWidget(m_chipList);

    listsLay->addWidget(vendorGroup, 1);
    listsLay->addWidget(chipGroup, 2);
    root->addLayout(listsLay, 1);

    // Bottom status + buttons ------------------------------------------
    auto *bottomLay = new QHBoxLayout;
    m_manufactorLabel = new QLabel(this);
    m_totalLabel = new QLabel(this);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Select"));
    bottomLay->addWidget(m_manufactorLabel);
    bottomLay->addStretch(1);
    bottomLay->addWidget(m_totalLabel);
    bottomLay->addSpacing(16);
    bottomLay->addWidget(buttons);
    root->addLayout(bottomLay);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &ChipDialog::rebuildVendors);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this] {
        if (m_chipList->currentItem())
            onChipActivated(m_chipList->currentItem());
    });
    connect(m_exactRadio, &QRadioButton::toggled, this, &ChipDialog::rebuildVendors);
    connect(fuzzyRadio, &QRadioButton::toggled, this, &ChipDialog::rebuildVendors);
    for (QRadioButton *radio : categoryGroup->findChildren<QRadioButton *>()) {
        connect(radio, &QRadioButton::toggled, this, [this, radio](bool checked) {
            if (checked)
                m_category = radio->property("category").toString();
            rebuildVendors();
        });
    }
    connect(m_vendorList, &QListWidget::currentRowChanged, this,
            &ChipDialog::onVendorChanged);
    connect(m_chipList, &QListWidget::itemDoubleClicked, this,
            &ChipDialog::onChipActivated);
    connect(buttons, &QDialogButtonBox::accepted, this, &ChipDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &ChipDialog::reject);

    rebuildVendors();
    resize(620, 460);
}

void ChipDialog::rebuildVendors()
{
    const bool exact = m_exactRadio->isChecked();
    const QStringList vendors =
        m_db.vendorsFor(m_category, m_searchEdit->text(), exact);

    m_vendorList->blockSignals(true);
    m_vendorList->clear();
    for (const QString &vendor : vendors)
        m_vendorList->addItem(vendor);
    m_vendorList->blockSignals(false);

    m_manufactorLabel->setText(tr("Manufacturers: %1").arg(vendors.size()));
    m_totalLabel->setText(tr("IC total: %1").arg(m_db.matching(m_category, m_searchEdit->text(), exact).size()));

    if (!vendors.isEmpty())
        m_vendorList->setCurrentRow(0);
    else
        rebuildChips();
}

void ChipDialog::onVendorChanged(int row)
{
    Q_UNUSED(row);
    rebuildChips();
}

void ChipDialog::rebuildChips()
{
    const bool exact = m_exactRadio->isChecked();
    const QString vendor = m_vendorList->currentItem()
                               ? m_vendorList->currentItem()->text()
                               : QString();
    const QVector<ChipInfo> chips =
        m_db.matching(m_category, m_searchEdit->text(), exact);

    m_chipList->blockSignals(true);
    m_chipList->clear();
    for (const ChipInfo &chip : chips) {
        if (!vendor.isEmpty() && chip.vendor != vendor)
            continue;
        m_chipList->addItem(chip.name);
    }
    m_chipList->blockSignals(false);

    if (m_chipList->count() > 0)
        m_chipList->setCurrentRow(0);
}

void ChipDialog::onChipActivated(QListWidgetItem *item)
{
    if (!item)
        return;
    const bool exact = m_exactRadio->isChecked();
    const QVector<ChipInfo> chips =
        m_db.matching(m_category, m_searchEdit->text(), exact);
    for (const ChipInfo &chip : chips) {
        if (chip.name == item->text()) {
            m_selected = chip;
            m_hasSelection = true;
            accept();
            return;
        }
    }
}

ChipInfo ChipDialog::getChip(const ChipDatabase &db, QWidget *parent)
{
    ChipDialog dlg(db, parent);
    if (dlg.exec() == QDialog::Accepted && dlg.hasSelection())
        return dlg.selectedChip();
    return ChipInfo();
}
