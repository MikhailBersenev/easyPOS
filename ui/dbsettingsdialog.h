#ifndef DBSETTINGSDIALOG_H
#define DBSETTINGSDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui { class DbSettingsDialog; }
class EasyPOSCore;

class DbSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DbSettingsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~DbSettingsDialog();

private slots:
    void on_testButton_clicked();
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    void loadFromSettings();
    void saveToSettings();
    void setResult(const QString &text, bool success);

    Ui::DbSettingsDialog *ui;
    std::shared_ptr<EasyPOSCore> m_core;
};

#endif // DBSETTINGSDIALOG_H
