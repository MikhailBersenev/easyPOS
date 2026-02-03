#include "discountdialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>

DiscountDialog::DiscountDialog(double totalAmount, QWidget *parent)
    : QDialog(parent)
    , m_totalAmount(totalAmount)
{
    setWindowTitle(tr("Скидка"));
    setMinimumWidth(320);

    m_sumRadio = new QRadioButton(tr("Сумма (₽)"));
    m_percentRadio = new QRadioButton(tr("Процент (%)"));
    m_sumRadio->setChecked(true);
    auto *typeGroup = new QButtonGroup(this);
    typeGroup->addButton(m_sumRadio);
    typeGroup->addButton(m_percentRadio);

    m_sumSpin = new QDoubleSpinBox(this);
    m_sumSpin->setRange(0, totalAmount > 0 ? totalAmount : 1e9);
    m_sumSpin->setDecimals(2);
    m_sumSpin->setSuffix(tr(" ₽"));

    m_percentSpin = new QDoubleSpinBox(this);
    m_percentSpin->setRange(0, 100);
    m_percentSpin->setDecimals(1);
    m_percentSpin->setSuffix(tr(" %"));
    m_percentSpin->setValue(10);

    auto *presetLayout = new QHBoxLayout();
    presetLayout->addWidget(new QLabel(tr("Быстро:")));
    for (int p : {5, 10, 15, 20}) {
        auto *btn = new QPushButton(tr("%1%").arg(p));
        connect(btn, &QPushButton::clicked, this, [this, p] {
            m_percentRadio->setChecked(true);
            m_percentSpin->setValue(p);
            updateFromPercent();
        });
        presetLayout->addWidget(btn);
    }
    presetLayout->addStretch();

    auto *form = new QFormLayout();
    form->addRow(m_sumRadio);
    form->addRow(tr("Сумма:"), m_sumSpin);
    form->addRow(m_percentRadio);
    form->addRow(tr("Процент:"), m_percentSpin);
    form->addRow(presetLayout);

    connect(m_sumSpin, &QDoubleSpinBox::valueChanged, this, &DiscountDialog::updateFromSum);
    connect(m_percentSpin, &QDoubleSpinBox::valueChanged, this, &DiscountDialog::updateFromPercent);
    connect(m_sumRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) updateFromSum();
    });
    connect(m_percentRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) updateFromPercent();
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    m_sumSpin->setFocus();
}

void DiscountDialog::updateFromPercent()
{
    if (!m_percentRadio->isChecked()) return;
    double p = m_percentSpin->value();
    double sum = m_totalAmount > 0 ? (m_totalAmount * p / 100.0) : 0;
    m_sumSpin->blockSignals(true);
    m_sumSpin->setValue(sum);
    m_sumSpin->blockSignals(false);
}

void DiscountDialog::updateFromSum()
{
    if (!m_sumRadio->isChecked()) return;
    double sum = m_sumSpin->value();
    double p = m_totalAmount > 0 ? (sum / m_totalAmount * 100.0) : 0;
    m_percentSpin->blockSignals(true);
    m_percentSpin->setValue(qMin(100.0, p));
    m_percentSpin->blockSignals(false);
}

double DiscountDialog::discountAmount() const
{
    if (m_sumRadio->isChecked())
        return m_sumSpin->value();
    if (m_totalAmount > 0)
        return m_totalAmount * m_percentSpin->value() / 100.0;
    return 0;
}
