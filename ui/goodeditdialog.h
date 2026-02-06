#ifndef GOODEDITDIALOG_H
#define GOODEDITDIALOG_H

#include <QDialog>

namespace Ui { class GoodEditDialog; }

class GoodEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GoodEditDialog(QWidget *parent = nullptr);
    ~GoodEditDialog();
    void setCategories(const QList<int> &ids, const QStringList &names);
    void setPromotions(const QList<int> &ids, const QStringList &names);
    void setData(const QString &name, const QString &description, int categoryId, bool isActive, const QString &imageUrl = QString(), int promotionId = 0);
    QString name() const;
    QString description() const;
    int categoryId() const;
    bool isActive() const;
    QString imageUrl() const;
    int promotionId() const;

private slots:
    void on_browseImage_clicked();

private:
    void updateIconPreview(const QString &path);
    Ui::GoodEditDialog *ui;
    QList<int> m_categoryIds;
    QList<int> m_promotionIds;
};

#endif // GOODEDITDIALOG_H
