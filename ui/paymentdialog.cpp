#include "paymentdialog.h"
#include "ui_paymentdialog.h"
#include <QMessageBox>

PaymentDialog::PaymentDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PaymentDialog)
{
    ui->setupUi(this);
    ui->gridLayout->setColumnStretch(1, 1);
    ui->gridLayout->setColumnMinimumWidth(0, 120);
    setupConnections();
}

PaymentDialog::~PaymentDialog()
{
    delete ui;
}

void PaymentDialog::setupConnections()
{
    connect(ui->fullAmountBtn, &QPushButton::clicked, this, &PaymentDialog::onFullAmount);
    connect(ui->amountSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PaymentDialog::updateTotal);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        const double total = sumPayments();
        const double diff = qAbs(total - m_amountToPay);
        if (diff > 0.01) {
            QMessageBox::warning(this, windowTitle(),
                tr("Сумма оплат (%1 ₽) должна совпадать с суммой к оплате (%2 ₽).")
                    .arg(QString::number(total, 'f', 2))
                    .arg(QString::number(m_amountToPay, 'f', 2)));
            return;
        }
        if (total <= 0) {
            QMessageBox::warning(this, windowTitle(), tr("Укажите сумму оплаты."));
            return;
        }
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Оплатить"));
    if (QPushButton *cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel))
        cancelBtn->setObjectName(QStringLiteral("cancelButton"));
    if (QLayout *lay = layout())
        lay->setAlignment(ui->buttonBox, Qt::AlignRight);
}

void PaymentDialog::setAmountToPay(double amount)
{
    m_amountToPay = amount;
    ui->toPayLabel->setText(tr("К оплате: %1 ₽").arg(QString::number(m_amountToPay, 'f', 2)));
    ui->amountSpinBox->setValue(m_amountToPay);
    updateTotal();
}

void PaymentDialog::setPaymentMethods(const QList<PaymentMethodInfo> &methods)
{
    m_methods = methods;
    ui->paymentMethodCombo->clear();
    for (const PaymentMethodInfo &m : m_methods)
        ui->paymentMethodCombo->addItem(m.name, m.id);
    if (!m_methods.isEmpty()) {
        ui->paymentMethodCombo->setCurrentIndex(0);
        ui->amountSpinBox->setValue(m_amountToPay);
    }
    updateTotal();
}

void PaymentDialog::onFullAmount()
{
    ui->amountSpinBox->setValue(m_amountToPay);
    updateTotal();
}

void PaymentDialog::updateTotal()
{
    const double total = sumPayments();
    ui->totalLabel->setText(tr("К оплате: %1 ₽").arg(QString::number(total, 'f', 2)));
    const double diff = total - m_amountToPay;
    if (qAbs(diff) <= 0.01) {
        ui->diffLabel->setText(tr("<span style='color: green; font-weight: bold;'>✓ Сумма совпадает</span>"));
    } else if (diff > 0) {
        ui->diffLabel->setText(tr("<span style='color: green; font-weight: bold;'>Сдача: %1 ₽</span>").arg(QString::number(diff, 'f', 2)));
    } else {
        ui->diffLabel->setText(tr("<span style='color: red;'>Недоплата: %1 ₽</span>").arg(QString::number(-diff, 'f', 2)));
    }
    updateOkButton();
}

void PaymentDialog::updateOkButton()
{
    const double total = sumPayments();
    const bool valid = total > 0 && qAbs(total - m_amountToPay) <= 0.01;
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

double PaymentDialog::sumPayments() const
{
    return ui->amountSpinBox->value();
}

QList<CheckPaymentRow> PaymentDialog::getPayments() const
{
    QList<CheckPaymentRow> list;
    const double amount = ui->amountSpinBox->value();
    if (amount <= 0)
        return list;
    CheckPaymentRow row;
    row.paymentMethodId = ui->paymentMethodCombo->currentData().toLongLong();
    row.paymentMethodName = ui->paymentMethodCombo->currentText();
    row.amount = amount;
    list.append(row);
    return list;
}
