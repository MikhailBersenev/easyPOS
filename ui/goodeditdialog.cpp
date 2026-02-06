#include "goodeditdialog.h"
#include "ui_goodeditdialog.h"
#include <QMessageBox>
#include <QApplication>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>

constexpr int kPreviewSize = 64;

GoodEditDialog::GoodEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GoodEditDialog)
{
    ui->setupUi(this);
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
    connect(ui->browseImageButton, &QPushButton::clicked, this, &GoodEditDialog::on_browseImage_clicked);
    connect(ui->imageUrlEdit, &QLineEdit::textChanged, this, [this](const QString &text) { updateIconPreview(text.trimmed()); });
    ui->nameEdit->setFocus();
}

GoodEditDialog::~GoodEditDialog()
{
    delete ui;
}

void GoodEditDialog::setCategories(const QList<int> &ids, const QStringList &names)
{
    m_categoryIds = ids;
    ui->categoryCombo->clear();
    ui->categoryCombo->addItems(names);
}

void GoodEditDialog::setData(const QString &name, const QString &description, int categoryId, bool isActive, const QString &imageUrl)
{
    ui->nameEdit->setText(name);
    ui->descriptionEdit->setPlainText(description);
    ui->activeCheck->setChecked(isActive);
    int idx = m_categoryIds.indexOf(categoryId);
    if (idx >= 0)
        ui->categoryCombo->setCurrentIndex(idx);
    ui->imageUrlEdit->setText(imageUrl);
    updateIconPreview(imageUrl);
}

QString GoodEditDialog::name() const
{
    return ui->nameEdit->text().trimmed();
}

QString GoodEditDialog::description() const
{
    return ui->descriptionEdit->toPlainText().trimmed();
}

int GoodEditDialog::categoryId() const
{
    int idx = ui->categoryCombo->currentIndex();
    if (idx >= 0 && idx < m_categoryIds.size())
        return m_categoryIds[idx];
    return m_categoryIds.value(0, 0);
}

bool GoodEditDialog::isActive() const
{
    return ui->activeCheck->isChecked();
}

QString GoodEditDialog::imageUrl() const
{
    return ui->imageUrlEdit->text().trimmed();
}

void GoodEditDialog::updateIconPreview(const QString &path)
{
    if (path.isEmpty()) {
        ui->iconPreviewLabel->clear();
        ui->iconPreviewLabel->setText(QLatin1String("—"));
        ui->iconPreviewLabel->setPixmap(QPixmap());
        return;
    }
    QString absPath = path;
    if (!QFileInfo(path).isAbsolute())
        absPath = QDir(QApplication::applicationDirPath()).absoluteFilePath(path);
    QPixmap pm(absPath);
    if (pm.isNull()) {
        ui->iconPreviewLabel->clear();
        ui->iconPreviewLabel->setText(tr("?"));
        ui->iconPreviewLabel->setPixmap(QPixmap());
        return;
    }
    ui->iconPreviewLabel->setText(QString());
    ui->iconPreviewLabel->setPixmap(pm.scaled(kPreviewSize, kPreviewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void GoodEditDialog::on_browseImage_clicked()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Выбрать пиктограмму товара"),
        ui->imageUrlEdit->text().trimmed().isEmpty() ? QDir::homePath() : QFileInfo(ui->imageUrlEdit->text().trimmed()).absolutePath(),
        tr("Изображения (*.png *.jpg *.jpeg *.bmp *.gif);;Все файлы (*)"));
    if (!path.isEmpty()) {
        ui->imageUrlEdit->setText(path);
        updateIconPreview(path);
    }
}
