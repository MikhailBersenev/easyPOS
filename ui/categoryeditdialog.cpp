#include "categoryeditdialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>

CategoryEditDialog::CategoryEditDialog(QWidget *parent)
    : QDialog(parent)
    , m_nameEdit(new QLineEdit(this))
    , m_descriptionEdit(new QPlainTextEdit(this))
{
    setWindowTitle(tr("Категория"));
    setMinimumWidth(380);
    m_nameEdit->setPlaceholderText(tr("Название категории"));
    m_nameEdit->setMaxLength(200);
    m_descriptionEdit->setPlaceholderText(tr("Описание (необязательно)"));
    m_descriptionEdit->setMaximumHeight(100);

    auto *form = new QFormLayout();
    form->addRow(tr("Название:"), m_nameEdit);
    form->addRow(tr("Описание:"), m_descriptionEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        if (name().isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Введите название."));
            m_nameEdit->setFocus();
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

void CategoryEditDialog::setData(const QString &name, const QString &description)
{
    m_nameEdit->setText(name);
    m_descriptionEdit->setPlainText(description);
}

QString CategoryEditDialog::name() const
{
    return m_nameEdit->text().trimmed();
}

QString CategoryEditDialog::description() const
{
    return m_descriptionEdit->toPlainText().trimmed();
}
