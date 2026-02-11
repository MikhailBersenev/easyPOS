#include "checkdetailsdialog.h"
#include "ui_checkdetailsdialog.h"
#include "checkexporter.h"
#include "../easyposcore.h"
#include "../sales/salesmanager.h"
#include "../sales/structures.h"
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QLocale>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrinterInfo>
#include <QTextDocument>
#include <QPainter>
#include <QFont>
#include <QFontDatabase>
#include <QTextOption>
#include <QPageSize>
#include <QPageLayout>
#include <QStandardPaths>
#include <QDir>
#include <QAbstractPrintDialog>

CheckDetailsDialog::CheckDetailsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core, qint64 checkId)
    : QDialog(parent)
    , ui(new Ui::CheckDetailsDialog)
    , m_core(core)
    , m_checkId(checkId)
{
    ui->setupUi(this);
    QString appName = core ? core->getBrandingAppName() : QStringLiteral("easyPOS");
    setWindowTitle(tr("%1 — Детали чека №%2").arg(appName).arg(checkId));

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

    auto *btnPrint = new QPushButton(tr("Печать"));
    connect(btnPrint, &QPushButton::clicked, this, &CheckDetailsDialog::onPrint);
    ui->buttonBox->addButton(btnPrint, QDialogButtonBox::ActionRole);
    
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CheckDetailsDialog::~CheckDetailsDialog()
{
    delete ui;
}

void CheckDetailsDialog::onPrint()
{
    if (!m_core) return;
    auto *sm = m_core->createSalesManager(this);
    if (!sm) return;
    
    const Check ch = sm->getCheck(m_checkId);
    if (ch.id == 0) return;
    const QList<SaleRow> rows = sm->getSalesByCheck(m_checkId);
    const double toPay = (ch.totalAmount - ch.discountAmount) < 0 ? 0 : (ch.totalAmount - ch.discountAmount);

    // Генерируем текстовое представление чека
    QString checkText = CheckExporter::generateText(m_core, sm, m_checkId, ch, rows, toPay);

    // Автоматически сохраняем чек в txt файл перед печатью
    QString appName = m_core ? m_core->getBrandingAppName() : QStringLiteral("easyPOS");
    QString defaultTxtName = tr("%1_Чек_%2.txt").arg(appName).arg(m_checkId);
    
    // Используем стандартную директорию для сохранения чеков
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString checksDir = documentsPath + QLatin1String("/easyPOS/Checks");
    QDir dir;
    if (!dir.exists(checksDir)) {
        dir.mkpath(checksDir);
    }
    
    QString txtPath = checksDir + QLatin1String("/") + defaultTxtName;
    
    // Сохраняем чек в txt файл
    if (!CheckExporter::saveToTxt(m_core, sm, m_checkId, txtPath)) {
        QMessageBox::warning(this, tr("Предупреждение"), 
            tr("Не удалось автоматически сохранить чек в файл, но печать продолжится."));
    }

    // Всегда показываем диалог выбора принтера
    QPrinter printer(QPrinter::HighResolution);
    
    // Проверяем наличие принтеров
    const bool hasPrinters = !QPrinterInfo::availablePrinters().isEmpty() || !QPrinterInfo::defaultPrinter().isNull();
    if (!hasPrinters) {
        // Если принтеры не найдены автоматически, предлагаем сохранить в PDF
        printer.setOutputFormat(QPrinter::PdfFormat);
        QString defaultPdfName = tr("%1_Чек_%2.pdf").arg(appName).arg(m_checkId);
        QString pdfPath = checksDir + QLatin1String("/") + defaultPdfName;
        printer.setOutputFileName(pdfPath);
    }
    
    QPrintDialog dlg(&printer, this);
    dlg.setOption(QAbstractPrintDialog::PrintToFile, true); // Разрешаем печать в файл
    
    if (dlg.exec() != QDialog::Accepted) return;
    
    // Если выбран формат PDF или печать в файл, проверяем путь
    QString pdfOutputPath;
    if (printer.outputFormat() == QPrinter::PdfFormat) {
        pdfOutputPath = printer.outputFileName();
        if (pdfOutputPath.isEmpty()) {
            // Если путь не указан, используем стандартную директорию
            QString defaultPdfName = tr("%1_Чек_%2.pdf").arg(appName).arg(m_checkId);
            pdfOutputPath = checksDir + QLatin1String("/") + defaultPdfName;
            printer.setOutputFileName(pdfOutputPath);
        }
    }

    // Печатаем текст напрямую через QTextDocument
    QTextDocument document;
    
    // Настраиваем размер страницы для принтера ДО создания painter
    if (printer.outputFormat() == QPrinter::PdfFormat) {
        printer.setPageSize(QPageSize(QPageSize::A4));
        printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    }
    
    // Используем шрифт с поддержкой кириллицы
    QFont font;
    const QStringList tryFamilies = { QStringLiteral("DejaVu Sans Mono"), QStringLiteral("Liberation Mono"),
                                      QStringLiteral("Courier New"), QStringLiteral("Courier"),
                                      QStringLiteral("Monospace") };
    for (const QString &family : tryFamilies) {
        if (QFontDatabase().hasFamily(family)) {
            font.setFamily(family);
            break;
        }
    }
    font.setPointSize(9);
    document.setDefaultFont(font);
    
    // Устанавливаем черный цвет текста
    document.setDefaultStyleSheet("body { color: black; }");
    
    // Настраиваем форматирование для печати
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    document.setDefaultTextOption(textOption);
    
    // Устанавливаем текст как HTML для лучшего форматирования
    QString htmlText = QString("<pre style='font-family: %1; font-size: %2pt; color: black; white-space: pre;'>%3</pre>")
                          .arg(font.family())
                          .arg(font.pointSize())
                          .arg(checkText.toHtmlEscaped());
    document.setHtml(htmlText);
    
    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::critical(this, tr("Печать"), tr("Не удалось начать печать."));
        return;
    }
    
    // Устанавливаем черный цвет для painter
    painter.setPen(Qt::black);
    
    // Получаем размеры страницы в точках (1/72 дюйма)
    QRectF pageRect = printer.pageRect(QPrinter::Point);
    const double pageWidthPt = pageRect.width();
    const double pageHeightPt = pageRect.height();
    
    // Устанавливаем окно для painter
    painter.setWindow(QRect(0, 0, int(pageWidthPt), int(pageHeightPt)));
    
    // Настраиваем размер документа по размеру страницы
    document.setTextWidth(pageWidthPt);
    document.setPageSize(QSizeF(pageWidthPt, pageHeightPt));
    
    // Вычисляем высоту содержимого
    qreal documentHeight = document.size().height();
    
    // Рисуем документ с учетом отступов
    QRectF targetRect(0, 0, pageWidthPt, qMin(documentHeight, pageHeightPt));
    document.drawContents(&painter, targetRect);
    
    painter.end();
    
    // Показываем сообщение о сохранении PDF, если был выбран формат PDF
    if (printer.outputFormat() == QPrinter::PdfFormat && !pdfOutputPath.isEmpty()) {
        QMessageBox::information(this, tr("Печать"), 
            tr("Чек сохранён в PDF:\n%1").arg(pdfOutputPath));
    }
}
