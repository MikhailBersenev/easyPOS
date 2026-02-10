#include "shiftsdialog.h"
#include "ui_shiftsdialog.h"
#include "../easyposcore.h"
#include "../shifts/shiftmanager.h"
#include "../shifts/structures.h"
#include "../alerts/alertkeys.h"
#include "../alerts/alertsmanager.h"
#include "../RBAC/structures.h"
#include "../logging/logmanager.h"
#include "windowcreator.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDateTime>

ShiftsDialog::ShiftsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , ui(new Ui::ShiftsDialog)
    , m_core(core)
{
    ui->setupUi(this);
    if (m_core && m_core->hasActiveSession()) {
        m_employeeId = m_core->getEmployeeIdByUserId(m_core->getCurrentSession().userId);
    }
    LOG_INFO() << "ShiftsDialog: открыт, employeeId=" << m_employeeId;
    QDate today = QDate::currentDate();
    ui->dateFromEdit->setDate(today.addDays(-7));
    ui->dateToEdit->setDate(today);
    ui->shiftsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->shiftsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->shiftsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->shiftsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->refreshButton, &QPushButton::clicked, this, &ShiftsDialog::on_refreshButton_clicked);
    connect(ui->startShiftButton, &QPushButton::clicked, this, &ShiftsDialog::on_startShiftButton_clicked);
    connect(ui->endShiftButton, &QPushButton::clicked, this, &ShiftsDialog::on_endShiftButton_clicked);
    updateCurrentShiftLabel();
    loadShifts();
}

ShiftsDialog::~ShiftsDialog()
{
    delete ui;
}

void ShiftsDialog::updateCurrentShiftLabel()
{
    if (m_employeeId <= 0) {
        ui->currentShiftLabel->setText(tr("Текущая смена: нет привязки к сотруднику"));
        ui->startShiftButton->setEnabled(false);
        ui->endShiftButton->setEnabled(false);
        return;
    }
    ShiftManager *sm = m_core ? m_core->getShiftManager() : nullptr;
    if (!sm) {
        ui->currentShiftLabel->setText(tr("Текущая смена: —"));
        ui->startShiftButton->setEnabled(false);
        ui->endShiftButton->setEnabled(false);
        return;
    }
    WorkShift current = sm->getCurrentShift(m_employeeId);
    if (current.id == 0) {
        LOG_DEBUG() << "ShiftsDialog::updateCurrentShiftLabel: смена не начата";
        ui->currentShiftLabel->setText(tr("Текущая смена: не начата"));
        ui->startShiftButton->setEnabled(true);
        ui->endShiftButton->setEnabled(false);
    } else {
        LOG_DEBUG() << "ShiftsDialog::updateCurrentShiftLabel: открытая смена id=" << current.id;
        ui->currentShiftLabel->setText(tr("Текущая смена: с %1 %2").arg(current.startedDate.toString(tr("dd.MM.yyyy"))).arg(current.startedTime.toString(tr("HH:mm"))));
        ui->startShiftButton->setEnabled(false);
        ui->endShiftButton->setEnabled(true);
    }
}

void ShiftsDialog::loadShifts()
{
    ui->shiftsTable->setRowCount(0);
    ShiftManager *sm = m_core ? m_core->getShiftManager() : nullptr;
    if (!sm) return;
    QDate from = ui->dateFromEdit->date();
    QDate to = ui->dateToEdit->date();
    if (from > to) from = to;
    QList<WorkShift> list = sm->getShifts(m_employeeId, from, to);
    LOG_DEBUG() << "ShiftsDialog::loadShifts: загружено смен:" << list.size() << "за период" << from << "-" << to;
    for (const WorkShift &s : list) {
        int row = ui->shiftsTable->rowCount();
        ui->shiftsTable->insertRow(row);
        ui->shiftsTable->setItem(row, 0, new QTableWidgetItem(s.employeeName.isEmpty() ? QString::number(s.employeeId) : s.employeeName));
        ui->shiftsTable->setItem(row, 1, new QTableWidgetItem(tr("%1 %2").arg(s.startedDate.toString(tr("dd.MM.yyyy"))).arg(s.startedTime.toString(tr("HH:mm")))));
        if (s.endedDate.isValid()) {
            ui->shiftsTable->setItem(row, 2, new QTableWidgetItem(tr("%1 %2").arg(s.endedDate.toString(tr("dd.MM.yyyy"))).arg(s.endedTime.toString(tr("HH:mm")))));
            QDateTime startDt(s.startedDate, s.startedTime);
            QDateTime endDt(s.endedDate, s.endedTime);
            qint64 secs = startDt.secsTo(endDt);
            int h = int(secs / 3600), m = int((secs % 3600) / 60);
            ui->shiftsTable->setItem(row, 3, new QTableWidgetItem(tr("%1 ч %2 мин").arg(h).arg(m)));
        } else {
            ui->shiftsTable->setItem(row, 2, new QTableWidgetItem(tr("—")));
            ui->shiftsTable->setItem(row, 3, new QTableWidgetItem(tr("идёт")));
        }
    }
}

void ShiftsDialog::on_refreshButton_clicked()
{
    updateCurrentShiftLabel();
    loadShifts();
}

void ShiftsDialog::on_startShiftButton_clicked()
{
    if (m_employeeId <= 0 || !m_core) return;
    ShiftManager *sm = m_core->getShiftManager();
    if (!sm) return;
    LOG_INFO() << "ShiftsDialog: нажато «Начать смену», employeeId=" << m_employeeId;
    WorkShift openShift = sm->getCurrentShift(m_employeeId);
    if (openShift.id != 0) {
        LOG_INFO() << "ShiftsDialog: начало смены отклонено — уже есть открытая смена id=" << openShift.id;
        QString msg = tr("У вас уже есть открытая смена (начата %1 в %2).\n\n"
                         "Сначала нажмите «Завершить смену», затем можно начать новую.")
            .arg(openShift.startedDate.toString(tr("dd.MM.yyyy")))
            .arg(openShift.startedTime.toString(tr("HH:mm")));
        QMessageBox::warning(this, windowTitle(), msg);
        updateCurrentShiftLabel();
        return;
    }
    if (!sm->startShift(m_employeeId)) {
        LOG_WARNING() << "ShiftsDialog: не удалось начать смену (БД?)";
        QMessageBox::warning(this, windowTitle(), tr("Не удалось начать смену. Проверьте подключение к базе данных."));
        return;
    }
    if (auto *alerts = m_core->createAlertsManager()) {
        qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
        alerts->log(AlertCategory::Shift, AlertSignature::ShiftStarted,
                    tr("Смена начата, employeeId=%1").arg(m_employeeId), uid, m_employeeId);
    }
    LOG_INFO() << "ShiftsDialog: смена успешно начата";
    QMessageBox::information(this, windowTitle(), tr("Смена начата."));
    updateCurrentShiftLabel();
    loadShifts();
}

void ShiftsDialog::on_endShiftButton_clicked()
{
    if (m_employeeId <= 0 || !m_core) return;
    ShiftManager *sm = m_core->getShiftManager();
    if (!sm) return;
    LOG_INFO() << "ShiftsDialog: нажато «Завершить смену», employeeId=" << m_employeeId;
    if (!sm->endCurrentShift(m_employeeId)) {
        LOG_INFO() << "ShiftsDialog: завершение смены — нет открытой смены";
        QMessageBox::warning(this, windowTitle(), tr("Нет открытой смены для завершения."));
        return;
    }
    if (auto *alerts = m_core->createAlertsManager()) {
        qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
        alerts->log(AlertCategory::Shift, AlertSignature::ShiftEnded,
                    tr("Смена завершена, employeeId=%1").arg(m_employeeId), uid, m_employeeId);
    }
    LOG_INFO() << "ShiftsDialog: смена успешно завершена";
    QMessageBox::information(this, windowTitle(), tr("Смена завершена."));
    updateCurrentShiftLabel();
    loadShifts();
}
