#include "goodeditdialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

GoodEditDialog::GoodEditDialog(QWidget *parent)
    : QDialog(parent)
    , m_nameEdit(new QLineEdit(this))
    , m_descriptionEdit(new QPlainTextEdit(this))
    , m_categoryCombo(new QComboBox(this))
    , m_activeCheck(new QCheckBox(tr("Активен"), this))
{
    setWindowTitle(tr("Товар"));
    setMinimumWidth(420);
    m_nameEdit->setPlaceholderText(tr("Название товара"));
    m_nameEdit->setMaxLength(200);
    m_descriptionEdit->setPlaceholderText(tr("Описание (необязательно)"));
    m_descriptionEdit->setMaximumHeight(80);
    m_activeCheck->setChecked(true);

    auto *form = new QFormLayout();
    form->addRow(tr("Название:"), m_nameEdit);
    form->addRow(tr("Описание:"), m_descriptionEdit);
    form->addRow(tr("Категория:"), m_categoryCombo);
    form->addRow(QString(), m_activeCheck);

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

void GoodEditDialog::setCategories(const QList<int> &ids, const QStringList &names)
{
    m_categoryIds = ids;
    m_categoryCombo->clear();
    m_categoryCombo->addItems(names);
}

void GoodEditDialog::setData(const QString &name, const QString &description, int categoryId, bool isActive)
{
    m_nameEdit->setText(name);
    m_descriptionEdit->setPlainText(description);
    m_activeCheck->setChecked(isActive);
    int idx = m_categoryIds.indexOf(categoryId);
    if (idx >= 0)
        m_categoryCombo->setCurrentIndex(idx);
}

QString GoodEditDialog::name() const
{
    return m_nameEdit->text().trimmed();
}

QString GoodEditDialog::description() const
{
    return m_descriptionEdit->toPlainText().trimmed();
}

int GoodEditDialog::categoryId() const
{
    int idx = m_categoryCombo->currentIndex();
    if (idx >= 0 && idx < m_categoryIds.size())
        return m_categoryIds[idx];
    return m_categoryIds.value(0, 0);
}

bool GoodEditDialog::isActive() const
{
    return m_activeCheck->isChecked();
}
