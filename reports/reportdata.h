#ifndef REPORTDATA_H
#define REPORTDATA_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QDateTime>

/**
 * Универсальная структура данных отчёта (заголовок + строки + аналитика).
 * Используется абстрактными экспортёрами для вывода в CSV, PDF, RTF и др.
 */
struct ReportData {
    QString title;                   ///< Название отчёта
    QString suggestedFilename;       ///< Рекомендуемое имя файла (без расширения)
    QStringList header;              ///< Заголовки колонок
    QList<QStringList> rows;        ///< Строки данных
    QStringList summaryLines;        ///< Строки аналитики (итоги, показатели)
    QDateTime generatedAt;          ///< Дата и время формирования отчёта
    QString preparedByName;         ///< ФИО составившего отчёт (для блока подписи)
    QString preparedByPosition;     ///< Должность составившего отчёт

    ReportData() = default;
    ReportData(const QString &t, const QStringList &h, const QList<QStringList> &r = {})
        : title(t), header(h), rows(r) {}
};

#endif // REPORTDATA_H
