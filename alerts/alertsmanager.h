#ifndef ALERTSMANAGER_H
#define ALERTSMANAGER_H

#include <QObject>
#include <QList>
#include <QDate>
#include <QSqlError>
#include "../db/databaseconnection.h"
#include "structures.h"

class AlertsManager : public QObject
{
    Q_OBJECT
public:
    explicit AlertsManager(QObject *parent = nullptr);
    void setDatabaseConnection(DatabaseConnection *conn);

    /**
     * Записать событие в таблицу alerts.
     * @param category Категория события
     * @param signature Название/сигнатура события
     * @param fullLog Полный текст лога (может быть пустым)
     * @param userId ID пользователя (0 — не задан)
     * @param employeeId ID сотрудника (0 — не задан)
     * @return true при успешной записи
     */
    bool log(const QString &category, const QString &signature, const QString &fullLog = QString(),
             qint64 userId = 0, qint64 employeeId = 0);

    /**
     * Записать событие с явной датой/временем (для импорта или отложенной записи).
     */
    bool logAt(const QString &category, const QString &signature, const QString &fullLog,
               const QDate &eventDate, const QTime &eventTime,
               qint64 userId = 0, qint64 employeeId = 0);

    /**
     * Выборка событий за период с опциональными фильтрами.
     * @param dateFrom Начало периода (включительно)
     * @param dateTo Конец периода (включительно)
     * @param category Фильтр по категории (пустая — все категории)
     * @param userId Фильтр по пользователю (0 — все)
     * @param employeeId Фильтр по сотруднику (0 — все)
     * @param limit Максимум записей (0 — без ограничения)
     */
    QList<Alert> getAlerts(const QDate &dateFrom, const QDate &dateTo,
                           const QString &category = QString(),
                           qint64 userId = 0, qint64 employeeId = 0,
                           int limit = 0) const;

    /** Одна запись по id */
    Alert getAlert(qint64 alertId) const;

    QSqlError lastError() const { return m_lastError; }

signals:
    void alertLogged(const Alert &alert);

private:
    DatabaseConnection *m_db = nullptr;
    mutable QSqlError m_lastError;
};

#endif // ALERTSMANAGER_H
