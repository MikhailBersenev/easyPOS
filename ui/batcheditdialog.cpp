#include "batcheditdialog.h"
#include "ui_batcheditdialog.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QTableWidgetItem>

BatchEditDialog::BatchEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BatchEditDialog)
{
    ui->setupUi(this);
    ui->prodDateEdit->setDate(QDate::currentDate());
    ui->expDateEdit->setDate(QDate::currentDate().addYears(1));
    ui->barcodeTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    setupConnections();
}

BatchEditDialog::~BatchEditDialog()
{
    delete ui;
}

void BatchEditDialog::setupConnections()
{
    connect(ui->addBarcodeButton, &QPushButton::clicked, this, &BatchEditDialog::onAddBarcode);
    connect(ui->barcodeEdit, &QLineEdit::returnPressed, this, &BatchEditDialog::onAddBarcode);
    connect(ui->removeBarcodeButton, &QPushButton::clicked, this, &BatchEditDialog::onRemoveBarcode);
    connect(ui->qntSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
        ui->reservedSpin->setMaximum(v);
    });
    ui->reservedSpin->setMaximum(ui->qntSpin->value());
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        if (quantity() < 0 || reservedQuantity() > quantity()) {
            QMessageBox::warning(this, windowTitle(), tr("Резерв не может превышать количество."));
            return;
        }
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void BatchEditDialog::setBatch(const BatchDetail &batch)
{
    m_batchId = batch.id;
    ui->batchNumberEdit->setText(batch.batchNumber);
    ui->qntSpin->setValue(static_cast<int>(batch.qnt));
    ui->reservedSpin->setValue(static_cast<int>(batch.reservedQuantity));
    ui->reservedSpin->setMaximum(static_cast<int>(batch.qnt));
    ui->priceSpin->setValue(batch.price);
    ui->prodDateEdit->setDate(batch.prodDate.isValid() ? batch.prodDate : QDate::currentDate());
    ui->expDateEdit->setDate(batch.expDate.isValid() ? batch.expDate : QDate::currentDate().addYears(1));
    ui->writtenOffCheck->setChecked(batch.writtenOff);
    ui->goodCombo->setEnabled(false);
    for (int i = 0; i < ui->goodCombo->count(); ++i) {
        if (ui->goodCombo->itemData(i).toLongLong() == batch.goodId) {
            ui->goodCombo->setCurrentIndex(i);
            break;
        }
    }
    setWindowTitle(tr("Редактирование партии"));
}

void BatchEditDialog::setGoodsList(const QList<QPair<qint64, QString>> &goods)
{
    m_goodsList = goods;
    ui->goodCombo->clear();
    for (const auto &p : goods)
        ui->goodCombo->addItem(p.second, p.first);
}

void BatchEditDialog::setBarcodes(const QList<BarcodeEntry> &barcodes)
{
    m_barcodeEntries = barcodes;
    updateBarcodeTable();
}

void BatchEditDialog::updateBarcodeTable()
{
    ui->barcodeTable->setRowCount(0);
    for (const BarcodeEntry &e : m_barcodeEntries) {
        if (e.isDeleted) continue;
        const int row = ui->barcodeTable->rowCount();
        ui->barcodeTable->insertRow(row);
        ui->barcodeTable->setItem(row, 0, new QTableWidgetItem(e.barcode));
        ui->barcodeTable->item(row, 0)->setData(Qt::UserRole, e.id);
    }
}

void BatchEditDialog::onAddBarcode()
{
    const QString bc = ui->barcodeEdit->text().trimmed();
    if (bc.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Введите штрихкод."));
        ui->barcodeEdit->setFocus();
        return;
    }
    m_barcodeEntries.append(BarcodeEntry{ 0, bc, m_batchId, QDate::currentDate(), false });
    updateBarcodeTable();
    ui->barcodeEdit->clear();
    ui->barcodeEdit->setFocus();
}

void BatchEditDialog::onRemoveBarcode()
{
    const int row = ui->barcodeTable->currentRow();
    if (row < 0) return;
    const qint64 id = ui->barcodeTable->item(row, 0)->data(Qt::UserRole).toLongLong();
    const QString bc = ui->barcodeTable->item(row, 0)->text();
    for (int i = 0; i < m_barcodeEntries.size(); ++i) {
        if (id != 0 && m_barcodeEntries[i].id == id) {
            m_barcodeEntries[i].isDeleted = true;
            break;
        }
        if (id == 0 && m_barcodeEntries[i].barcode == bc) {
            m_barcodeEntries.removeAt(i);
            break;
        }
    }
    updateBarcodeTable();
}

qint64 BatchEditDialog::goodId() const
{
    const int idx = ui->goodCombo->currentIndex();
    if (idx < 0 || idx >= m_goodsList.size()) return 0;
    return m_goodsList.at(idx).first;
}

QString BatchEditDialog::batchNumber() const
{
    return ui->batchNumberEdit->text().trimmed();
}

qint64 BatchEditDialog::quantity() const
{
    return static_cast<qint64>(ui->qntSpin->value());
}

qint64 BatchEditDialog::reservedQuantity() const
{
    return static_cast<qint64>(ui->reservedSpin->value());
}

double BatchEditDialog::price() const
{
    return ui->priceSpin->value();
}

QDate BatchEditDialog::prodDate() const
{
    return ui->prodDateEdit->date();
}

QDate BatchEditDialog::expDate() const
{
    return ui->expDateEdit->date();
}

bool BatchEditDialog::writtenOff() const
{
    return ui->writtenOffCheck->isChecked();
}

QList<QString> BatchEditDialog::barcodes() const
{
    QList<QString> list;
    for (const BarcodeEntry &e : m_barcodeEntries) {
        if (!e.isDeleted && !e.barcode.isEmpty())
            list.append(e.barcode);
    }
    return list;
}

QList<QString> BatchEditDialog::barcodesToAdd() const
{
    QList<QString> list;
    for (const BarcodeEntry &e : m_barcodeEntries) {
        if (!e.isDeleted && e.id == 0 && !e.barcode.isEmpty())
            list.append(e.barcode);
    }
    return list;
}

QList<qint64> BatchEditDialog::barcodeIdsToRemove() const
{
    QList<qint64> list;
    for (const BarcodeEntry &e : m_barcodeEntries) {
        if (e.isDeleted && e.id != 0)
            list.append(e.id);
    }
    return list;
}
