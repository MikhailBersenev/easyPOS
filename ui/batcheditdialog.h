#ifndef BATCHEDITDIALOG_H
#define BATCHEDITDIALOG_H

#include <QDialog>
#include "../sales/structures.h"

namespace Ui {
class BatchEditDialog;
}

class BatchEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchEditDialog(QWidget *parent = nullptr);
    ~BatchEditDialog();

    void setBatch(const BatchDetail &batch);
    void setGoodsList(const QList<QPair<qint64, QString>> &goods);
    void setBarcodes(const QList<BarcodeEntry> &barcodes);

    bool isNewBatch() const { return m_batchId == 0; }
    qint64 batchId() const { return m_batchId; }

    qint64 goodId() const;
    QString batchNumber() const;
    qint64 quantity() const;
    qint64 reservedQuantity() const;
    double price() const;
    QDate prodDate() const;
    QDate expDate() const;
    bool writtenOff() const;
    QList<QString> barcodes() const;
    QList<QString> barcodesToAdd() const;
    QList<qint64> barcodeIdsToRemove() const;

private slots:
    void onAddBarcode();
    void onRemoveBarcode();

private:
    void setupConnections();
    void updateBarcodeTable();

    Ui::BatchEditDialog *ui;
    qint64 m_batchId = 0;
    QList<QPair<qint64, QString>> m_goodsList;
    QList<BarcodeEntry> m_barcodeEntries;
};

#endif // BATCHEDITDIALOG_H
