#include "vatrateeditdialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

VatRateEditDialog::VatRateEditDialog(QWidget *parent)
    : QDialog(parent)
    , m_nameEdit(new QLineEdit(this))
    , m_rateSpin(new QDoubleSpinBox(this))
{
    setWindowTitle(tr("Ставка НДС"));
    setMinimumWidth(320);
    m_nameEdit->setPlaceholderText(tr("Например: НДС 20%"));
    m_nameEdit->setMaxLength(100);
    m_rateSpin->setRange(0, 100);
    m_rateSpin->setDecimals(2);
    m_rateSpin->setSuffix(tr(" %"));

    auto *form = new QFormLayout();
    form->addRow(tr("Название:"), m_nameEdit);
    form->addRow(tr("Ставка:"), m_rateSpin);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        if (name().isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Введите название."));
            m_nameEdit->setFocus();
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    m_nameEdit->setFocus();
}

void VatRateEditDialog::setData(const QString &name, double rate)
{
    m_nameEdit->setText(name);
    m_rateSpin->setValue(rate);
}

QString VatRateEditDialog::name() const
{
    return m_nameEdit->text().trimmed();
}

double VatRateEditDialog::rate() const
{
    return m_rateSpin->value();
}
