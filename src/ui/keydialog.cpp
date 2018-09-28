#include "keydialog.h"
#include "ui_keydialog.h"

KeyDialog::KeyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KeyDialog)
{
    ui->setupUi(this);
    connect(ui->keySequenceEdit, &QKeySequenceEdit::keySequenceChanged, [=](const QKeySequence&) {
        setButtons();
    });

    connect(ui->commandLineEdit, &QLineEdit::textChanged, [=](const QString&) {
        setButtons();
    });

    connect(ui->clearButton, &QPushButton::clicked, [=] {
        ui->keySequenceEdit->clear();
    });

    connect(ui->addButton, SIGNAL(clicked()),
            this, SLOT(accept()));
    connect(ui->changeButton, SIGNAL(clicked()),
            this, SLOT(accept()));
    connect(ui->cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
}

KeyDialog::~KeyDialog()
{
    delete ui;
}

QPair<QString, QPair<QString, QString>> KeyDialog::selectKey(bool add, QPair<QString, QPair<QString, QString>> init)
{
    this->add = add;
    ui->keySequenceEdit->setKeySequence(QKeySequence(init.first));
    ui->commandLineEdit->setText(init.second.first);
    ui->labelLineEdit->setText(init.second.second);
    setButtons();
    if (exec() == QDialog::Rejected)
        return QPair<QString, QPair<QString, QString>>();
    return QPair<QString, QPair<QString, QString>>({
        ui->keySequenceEdit->keySequence().toString(),
        {ui->commandLineEdit->text(),
         ui->labelLineEdit->text()}
    });
}

void KeyDialog::setButtons()
{
    bool enabled =
        (!ui->keySequenceEdit->keySequence().isEmpty() &&
         !ui->commandLineEdit->text().isEmpty());

    if (add) {
        ui->changeButton->setVisible(false);
        ui->addButton->setVisible(true);
        ui->addButton->setEnabled(enabled);
    } else {
        ui->addButton->setVisible(false);
        ui->changeButton->setVisible(true);
        ui->changeButton->setEnabled(enabled);
    }
}
