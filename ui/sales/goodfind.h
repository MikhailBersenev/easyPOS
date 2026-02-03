#ifndef GOODFIND_H
#define GOODFIND_H

#include <QDialog>
#include <QTableWidgetItem>
#include <memory>

class EasyPOSCore;

namespace Ui {
class GoodFind;
}

class GoodFind : public QDialog
{
    Q_OBJECT

public:
    explicit GoodFind(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~GoodFind();

    // Установить начальный фильтр поиска
    void setInitialSearch(const QString &text);

    // Результат выбора: true = товар (batch), false = услуга (service)
    bool isBatchSelected() const { return m_isBatch; }
    qint64 selectedBatchId() const { return m_batchId; }
    qint64 selectedServiceId() const { return m_serviceId; }

private slots:
    void on_searchEdit_textChanged(const QString &text);
    void on_selectButton_clicked();
    void on_cancelButton_clicked();
    void on_tableWidget_itemDoubleClicked(QTableWidgetItem *item);

private:
    void fillTable();
    void applyFilter(const QString &filter);
    void selectCurrentRow();

    Ui::GoodFind *ui;
    std::shared_ptr<EasyPOSCore> m_core;
    bool m_isBatch = false;
    qint64 m_batchId = 0;
    qint64 m_serviceId = 0;
};

#endif // GOODFIND_H
