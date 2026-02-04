#include "serviceeditdialog.h"
#include "ui_serviceeditdialog.h"
#include <QMessageBox>

ServiceEditDialog::ServiceEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ServiceEditDialog)
{
    ui->setupUi(this);
    ui->priceSpin->setRange(0, 999999999.99);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        if (name().isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Введите название."));
            ui->nameEdit->setFocus();
            return;
        }
        if (m_categoryIds.isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Нет категорий. Сначала добавьте категории товаров."));
            return;
        }
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    ui->nameEdit->setFocus();
}

ServiceEditDialog::~ServiceEditDialog()
{
    delete ui;
}

void ServiceEditDialog::setCategories(const QList<int> &ids, const QStringList &names)
{
    m_categoryIds = ids;
    ui->categoryCombo->clear();
    ui->categoryCombo->addItems(names);
}

void ServiceEditDialog::setData(const QString &name, const QString &description, double price, int categoryId)
{
    ui->nameEdit->setText(name);
    ui->descriptionEdit->setPlainText(description);
    ui->priceSpin->setValue(price);
    int idx = m_categoryIds.indexOf(categoryId);
    if (idx >= 0)
        ui->categoryCombo->setCurrentIndex(idx);
}

QString ServiceEditDialog::name() const
{
    return ui->nameEdit->text().trimmed();
}

QString ServiceEditDialog::description() const
{
    return ui->descriptionEdit->toPlainText().trimmed();
}

double ServiceEditDialog::price() const
{
    return ui->priceSpin->value();
}

int ServiceEditDialog::categoryId() const
{
    int idx = ui->categoryCombo->currentIndex();
    if (idx >= 0 && idx < m_categoryIds.size())
        return m_categoryIds[idx];
    return m_categoryIds.value(0, 0);
}
