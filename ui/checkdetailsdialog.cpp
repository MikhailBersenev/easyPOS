#include "checkdetailsdialog.h"
#include "ui_checkdetailsdialog.h"
#include "checkpdfexporter.h"
#include "../easyposcore.h"
#include "../sales/salesmanager.h"
#include "../sales/structures.h"
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QLocale>

CheckDetailsDialog::CheckDetailsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core, qint64 checkId)
    : QDialog(parent)
    , ui(new Ui::CheckDetailsDialog)
    , m_core(core)
    , m_checkId(checkId)
{
    ui->setupUi(this);
    setWindowTitle(tr("Детали чека №%1").arg(checkId));

    if (core) {
        auto *sm = core->createSalesManager(this);
        if (sm) {
            const Check ch = sm->getCheck(checkId);
            if (ch.id > 0) {
                const QList<SaleRow> rows = sm->getSalesByCheck(checkId);
                const double toPay = (ch.totalAmount - ch.discountAmount) < 0 ? 0 : (ch.totalAmount - ch.discountAmount);
                QString text = tr("Чек №%1\nДата: %2 %3\nСотрудник: %4\n\n")
                    .arg(ch.id).arg(QLocale::system().toString(ch.date, QLocale::ShortFormat))
                    .arg(ch.time.toString(Qt::ISODate)).arg(ch.employeeName);
                double totalVat = 0.0;
                for (const SaleRow &r : rows) {
                    const QString vatName = sm->getVatRateName(r.vatRateId);
                    const double rate = sm->getVatRatePercent(r.vatRateId);
                    const double vatAmt = rate > 0 ? r.sum * rate / (100.0 + rate) : 0.0;
                    totalVat += vatAmt;
                    text += QStringLiteral("%1 x %2 = %3 ₽")
                        .arg(r.itemName).arg(r.qnt).arg(QString::number(r.sum, 'f', 2));
                    if (!vatName.isEmpty())
                        text += QStringLiteral("  [%1]").arg(vatName);
                    if (vatAmt > 0)
                        text += QStringLiteral("  НДС: %1 ₽").arg(QString::number(vatAmt, 'f', 2));
                    text += QLatin1String("\n");
                }
                text += tr("\nИтого: %1 ₽\nСкидка: %2 ₽\nСумма НДС: %3 ₽\nК оплате: %4 ₽")
                    .arg(QString::number(ch.totalAmount, 'f', 2))
                    .arg(QString::number(ch.discountAmount, 'f', 2))
                    .arg(QString::number(totalVat, 'f', 2))
                    .arg(QString::number(toPay, 'f', 2));
                ui->textEdit->setPlainText(text);
            }
        }
    }

    auto *btnPdf = new QPushButton(tr("Сохранить в PDF"));
    connect(btnPdf, &QPushButton::clicked, this, &CheckDetailsDialog::onSaveToPdf);
    ui->buttonBox->addButton(btnPdf, QDialogButtonBox::ActionRole);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CheckDetailsDialog::~CheckDetailsDialog()
{
    delete ui;
}

void CheckDetailsDialog::onSaveToPdf()
{
    QString defaultName = tr("Чек_%1.pdf").arg(m_checkId);
    QString path = QFileDialog::getSaveFileName(this, tr("Сохранить чек в PDF"),
        defaultName, tr("PDF (*.pdf)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive))
        path += QLatin1String(".pdf");

    if (CheckPdfExporter::saveToPdf(m_core, m_checkId, path))
        QMessageBox::information(this, tr("PDF"), tr("Чек сохранён в PDF."));
    else
        QMessageBox::critical(this, tr("PDF"), tr("Не удалось сохранить PDF."));
}
