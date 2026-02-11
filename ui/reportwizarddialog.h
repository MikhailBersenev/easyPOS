#ifndef REPORTWIZARDDIALOG_H
#define REPORTWIZARDDIALOG_H

#include <QWizard>
#include <memory>

class EasyPOSCore;

#include <QDate>

#include "../reports/reportdata.h"

class ReportWizardDialog;

/** Типы отчётов для setPreset() */
enum ReportWizardType {
    ReportSalesByPeriod = 0,
    ReportStockReceipt = 1,
    ReportStockWriteOff = 2,
    ReportSalesByShift = 3,
    ReportReconciliation = 4,
    ReportStockBalance = 5,
    ReportStockBalanceWithBarcodes = 6,
    ReportChecks = 7,
    ReportProduction = 8
};

class ReportWizardDialog : public QWizard
{
    Q_OBJECT
public:
    explicit ReportWizardDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~ReportWizardDialog();

    EasyPOSCore *core() const;

    /** Предзаполнить тип отчёта и параметры, открыть на нужной странице (0=тип, 1=параметры, 2=экспорт) */
    void setPreset(ReportWizardType type, const QDate &dateFrom, const QDate &dateTo, qint64 shiftId = 0, int startPage = 0);

private slots:
    void onCurrentIdChanged(int id);

private:
    void setupPages();
    void setupTypePage();
    void setupParamsPage();
    void setupExportPage();
    ReportData generateReport() const;

    class TypePage *m_typePage = nullptr;
    class ParamsPage *m_paramsPage = nullptr;
    class ExportPage *m_exportPage = nullptr;

    std::shared_ptr<EasyPOSCore> m_core;

    friend class ExportPage;
};

// Страница выбора типа отчёта
class TypePage : public QWizardPage
{
    Q_OBJECT
public:
    explicit TypePage(QWidget *parent = nullptr);
    int reportType() const;
    void setReportType(int type);

private:
    class QListWidget *m_list = nullptr;
};

// Страница параметров (период / смена)
class ParamsPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit ParamsPage(ReportWizardDialog *wizard, QWidget *parent = nullptr);
    void updateForReportType(int type);
    void refreshShiftList(class EasyPOSCore *core);
    void setDates(const QDate &from, const QDate &to);
    void setShiftId(qint64 shiftId);
    bool validatePage() override;
    QDate dateFrom() const;
    QDate dateTo() const;
    qint64 shiftId() const;

private:
    ReportWizardDialog *m_wizard = nullptr;
    class QDateEdit *m_dateFrom = nullptr;
    class QDateEdit *m_dateTo = nullptr;
    class QComboBox *m_shiftCombo = nullptr;
    class QWidget *m_periodWidget = nullptr;
    class QWidget *m_shiftWidget = nullptr;
};

// Страница превью и экспорта
class ExportPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit ExportPage(ReportWizardDialog *wizard, QWidget *parent = nullptr);
    void fillFromReportData(const ReportData &data);

private slots:
    void onExportCsv();
    void onExportOdt();

private:
    ReportWizardDialog *m_wizard = nullptr;
    class QTableWidget *m_previewTable = nullptr;
};

#endif // REPORTWIZARDDIALOG_H
