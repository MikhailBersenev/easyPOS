#include "serviceeditdialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

ServiceEditDialog::ServiceEditDialog(QWidget *parent)
    : QDialog(parent)
    , m_nameEdit(new QLineEdit(this))
    , m_descriptionEdit(new QPlainTextEdit(this))
    , m_priceSpin(new QDoubleSpinBox(this))
    , m_categoryCombo(new QComboBox(this))
{
    setWindowTitle(tr("Услуга"));
    setMinimumWidth(400);
    m_nameEdit->setPlaceholderText(tr("Название услуги"));
    m_nameEdit->setMaxLength(200);
    m_descriptionEdit->setPlaceholderText(tr("Описание (необязательно)"));
    m_descriptionEdit->setMaximumHeight(80);
    m_priceSpin->setRange(0, 999999999.99);
    m_priceSpin->setDecimals(2);
    m_priceSpin->setSuffix(tr(" ₽"));

    auto *form = new QFormLayout();
    form->addRow(tr("Название:"), m_nameEdit);
    form->addRow(tr("Описание:"), m_descriptionEdit);
    form->addRow(tr("Цена:"), m_priceSpin);
    form->addRow(tr("Категория:"), m_categoryCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        if (name().isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Введите название."));
            m_nameEdit->setFocus();
            return;
        }
        if (m_categoryIds.isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Нет категорий. Сначала добавьте категории товаров."));
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    m_nameEdit->setFocus();
}

void ServiceEditDialog::setCategories(const QList<int> &ids, const QStringList &names)
{
    m_categoryIds = ids;
    m_categoryCombo->clear();
    m_categoryCombo->addItems(names);
}

void ServiceEditDialog::setData(const QString &name, const QString &description, double price, int categoryId)
{
    m_nameEdit->setText(name);
    m_descriptionEdit->setPlainText(description);
    m_priceSpin->setValue(price);
    int idx = m_categoryIds.indexOf(categoryId);
    if (idx >= 0)
        m_categoryCombo->setCurrentIndex(idx);
}

QString ServiceEditDialog::name() const
{
    return m_nameEdit->text().trimmed();
}

QString ServiceEditDialog::description() const
{
    return m_descriptionEdit->toPlainText().trimmed();
}

double ServiceEditDialog::price() const
{
    return m_priceSpin->value();
}

int ServiceEditDialog::categoryId() const
{
    int idx = m_categoryCombo->currentIndex();
    if (idx >= 0 && idx < m_categoryIds.size())
        return m_categoryIds[idx];
    return m_categoryIds.value(0, 0);
}
