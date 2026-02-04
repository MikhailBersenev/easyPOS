#include "discountdialog.h"
#include "ui_discountdialog.h"
#include <QPushButton>
#include <QButtonGroup>

DiscountDialog::DiscountDialog(double totalAmount, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DiscountDialog)
    , m_totalAmount(totalAmount)
{
    ui->setupUi(this);
    ui->sumSpin->setRange(0, totalAmount > 0 ? totalAmount : 1e9);
    auto *typeGroup = new QButtonGroup(this);
    typeGroup->addButton(ui->sumRadio);
    typeGroup->addButton(ui->percentRadio);
    int insertIdx = 1;
    for (int p : {5, 10, 15, 20}) {
        auto *btn = new QPushButton(tr("%1%").arg(p));
        connect(btn, &QPushButton::clicked, this, [this, p] {
            ui->percentRadio->setChecked(true);
            ui->percentSpin->setValue(p);
            updateFromPercent();
        });
        ui->presetLayout->insertWidget(insertIdx++, btn);
    }
    connect(ui->sumSpin, &QDoubleSpinBox::valueChanged, this, &DiscountDialog::updateFromSum);
    connect(ui->percentSpin, &QDoubleSpinBox::valueChanged, this, &DiscountDialog::updateFromPercent);
    connect(ui->sumRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) updateFromSum();
    });
    connect(ui->percentRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) updateFromPercent();
    });
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    ui->sumSpin->setFocus();
}

DiscountDialog::~DiscountDialog()
{
    delete ui;
}

void DiscountDialog::updateFromPercent()
{
    if (!ui->percentRadio->isChecked()) return;
    double p = ui->percentSpin->value();
    double sum = m_totalAmount > 0 ? (m_totalAmount * p / 100.0) : 0;
    ui->sumSpin->blockSignals(true);
    ui->sumSpin->setValue(sum);
    ui->sumSpin->blockSignals(false);
}

void DiscountDialog::updateFromSum()
{
    if (!ui->sumRadio->isChecked()) return;
    double sum = ui->sumSpin->value();
    double p = m_totalAmount > 0 ? (sum / m_totalAmount * 100.0) : 0;
    ui->percentSpin->blockSignals(true);
    ui->percentSpin->setValue(qMin(100.0, p));
    ui->percentSpin->blockSignals(false);
}

double DiscountDialog::discountAmount() const
{
    if (ui->sumRadio->isChecked())
        return ui->sumSpin->value();
    if (m_totalAmount > 0)
        return m_totalAmount * ui->percentSpin->value() / 100.0;
    return 0;
}
