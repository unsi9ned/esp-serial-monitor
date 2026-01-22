#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "version.h"
#include <QDate>
#include <QTime>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->labelProgName->setText(APP_NAME);
    ui->labelVersion->setText(APP_VERSION);
    ui->labelBuildDate->setText(makeBuildTimestamp());
    ui->labelAuthor->setText(APP_AUTHOR);
    ui->labelWebSite->setText(APP_WEBSITE);
    ui->labelGitHub->setText(APP_GITHUB);

    ui->labelWebSite->setText(QString("<a href='%1'>%1</a>").arg(ui->labelWebSite->text()));
    ui->labelWebSite->setOpenExternalLinks(true);
    ui->labelGitHub->setText(QString("<a href='%1'>%1</a>").arg(ui->labelGitHub->text()));
    ui->labelGitHub->setOpenExternalLinks(true);

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

QString AboutDialog::makeBuildTimestamp()
{
    QDate date = QLocale("en_US").toDate(QString(APP_BUILD_DATE).simplified(), "MMM d yyyy");
    QTime time = QTime::fromString(QString(APP_BUILD_TIME), "hh:mm:ss");
    QString timestamp = QString("%1%2%3.%4%5").
                            arg(date.year(), 4, 10, QChar('0')).
                            arg(date.month(), 2, 10, QChar('0')).
                            arg(date.day(), 2, 10, QChar('0')).
                            arg(time.hour(), 2, 10, QChar('0')).
                            arg(time.minute(), 2, 10, QChar('0'));
    return timestamp;
}
