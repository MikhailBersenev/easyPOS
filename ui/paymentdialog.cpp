#include "paymentdialog.h"
#include "ui_paymentdialog.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QHeaderView>

PaymentDialog::PaymentDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PaymentDialog)
{
    ui->setupUi(this);
    ui->toPayFrame->setStyleSheet(QStringLiteral("QFrame { background-color: palette(base); padding: 8px; }"));
    QFont toPayFont = ui->toPayLabel->font();
    toPayFont.setPointSize(toPayFont.pointSize() + 1);
    toPayFont.setBold(true);
    ui->toPayLabel->setFont(toPayFont);
    ui->paymentsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->paymentsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->paymentsTable->setColumnWidth(1, 120);
    connect(ui->paymentsTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &PaymentDialog::onSelectionChanged);
    setupConnections();
    onSelectionChanged();
}

PaymentDialog::~PaymentDialog()
{
    delete ui;
}

void PaymentDialog::setupConnections()
{
    connect(ui->addBtn, &QPushButton::clicked, this, &PaymentDialog::onAddRow);
    connect(ui->removeBtn, &QPushButton::clicked, this, &PaymentDialog::onRemoveRow);
    connect(ui->fullAmountBtn, &QPushButton::clicked, this, &PaymentDialog::onFullAmount);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        const double diff = qAbs(sumPayments() - m_amountToPay);
        if (diff > 0.01) {
            QMessageBox::warning(this, windowTitle(),
                tr("Сумма оплат (%1 ₽) должна совпадать с суммой к оплате (%2 ₽).")
                    .arg(QString::number(sumPayments(), 'f', 2))
                    .arg(QString::number(m_amountToPay, 'f', 2)));
            return;
        }
        if (sumPayments() <= 0) {
            QMessageBox::warning(this, windowTitle(), tr("Укажите хотя бы одну оплату."));
            return;
        }
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Оплатить"));
}

void PaymentDialog::setAmountToPay(double amount)
{
    m_amountToPay = amount;
    ui->toPayLabel->setText(tr("К оплате: %1 ₽").arg(QString::number(m_amountToPay, 'f', 2)));
    updateTotal();
}

void PaymentDialog::setPaymentMethods(const QList<PaymentMethodInfo> &methods)
{
    m_methods = methods;
    ui->paymentsTable->setRowCount(0);
    if (m_methods.isEmpty())
        return;
    onAddRow();
    if (ui->paymentsTable->rowCount() > 0) {
        if (auto *spin = qobject_cast<QDoubleSpinBox *>(ui->paymentsTable->cellWidget(0, 1)))
            spin->setValue(m_amountToPay);
    }
    updateTotal();
}

void PaymentDialog::onAddRow()
{
    if (m_methods.isEmpty()) return;
    const int row = ui->paymentsTable->rowCount();
    ui->paymentsTable->insertRow(row);
    auto *combo = new QComboBox(this);
    for (const PaymentMethodInfo &m : m_methods)
        combo->addItem(m.name, m.id);
    ui->paymentsTable->setCellWidget(row, 0, combo);
    auto *amountEdit = new QDoubleSpinBox(this);
    amountEdit->setMinimum(0.0);
    amountEdit->setMaximum(99999999.99);
    amountEdit->setDecimals(2);
    amountEdit->setValue(0.0);
    amountEdit->setSuffix(tr(" ₽"));
    connect(amountEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PaymentDialog::updateTotal);
    ui->paymentsTable->setCellWidget(row, 1, amountEdit);
    updateTotal();
}

void PaymentDialog::onRemoveRow()
{
    const int row = ui->paymentsTable->currentRow();
    if (row >= 0)
        ui->paymentsTable->removeRow(row);
    updateTotal();
    onSelectionChanged();
}

void PaymentDialog::onFullAmount()
{
    if (m_methods.isEmpty()) return;
    if (ui->paymentsTable->rowCount() == 0)
        onAddRow();
    if (ui->paymentsTable->rowCount() > 0) {
        if (auto *spin = qobject_cast<QDoubleSpinBox *>(ui->paymentsTable->cellWidget(0, 1)))
            spin->setValue(m_amountToPay);
    }
    updateTotal();
}

void PaymentDialog::onSelectionChanged()
{
    const bool hasSelection = ui->paymentsTable->currentRow() >= 0;
    const bool canRemove = hasSelection && ui->paymentsTable->rowCount() > 1;
    ui->removeBtn->setEnabled(canRemove);
}

void PaymentDialog::updateTotal()
{
    const double total = sumPayments();
    ui->totalLabel->setText(tr("Итого оплат: %1 ₽").arg(QString::number(total, 'f', 2)));
    const double diff = total - m_amountToPay;
    if (qAbs(diff) <= 0.01) {
        ui->diffLabel->setText(tr("<span style='color: green; font-weight: bold;'>✓ Сумма совпадает</span>"));
    } else if (diff > 0) {
        ui->diffLabel->setText(tr("<span style='color: red;'>Переплата: %1 ₽</span>").arg(QString::number(diff, 'f', 2)));
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
    double sum = 0;
    for (int r = 0; r < ui->paymentsTable->rowCount(); ++r) {
        QWidget *w = ui->paymentsTable->cellWidget(r, 1);
        if (auto *spin = qobject_cast<QDoubleSpinBox *>(w))
            sum += spin->value();
    }
    return sum;
}

QList<CheckPaymentRow> PaymentDialog::getPayments() const
{
    QList<CheckPaymentRow> list;
    for (int r = 0; r < ui->paymentsTable->rowCount(); ++r) {
        QWidget *comboW = ui->paymentsTable->cellWidget(r, 0);
        QWidget *amountW = ui->paymentsTable->cellWidget(r, 1);
        auto *combo = qobject_cast<QComboBox *>(comboW);
        auto *spin = qobject_cast<QDoubleSpinBox *>(amountW);
        if (!combo || !spin || spin->value() <= 0)
            continue;
        CheckPaymentRow row;
        row.paymentMethodId = combo->currentData().toLongLong();
        row.paymentMethodName = combo->currentText();
        row.amount = spin->value();
        list.append(row);
    }
    return list;
}
