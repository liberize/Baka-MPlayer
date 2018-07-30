#include "pluginconfigdialog.h"
#include "ui_pluginconfigdialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>


PluginConfigDialog::PluginConfigDialog(QString name, QList<Pi::ConfigItem> &items, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PluginConfigDialog),
    inputWidgets(items.size(), nullptr),
    validatePass(items.size(), true)
{
    ui->setupUi(this);
    setWindowTitle(tr("Edit Config for Plugin: %0").arg(name));

    QSizePolicy titleSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    titleSizePolicy.setHorizontalStretch(0);
    titleSizePolicy.setVerticalStretch(0);
    QSizePolicy validSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    validSizePolicy.setHorizontalStretch(0);
    validSizePolicy.setVerticalStretch(0);

    int row = 0;
    for (auto it = items.begin(); it != items.end(); ++it, ++row) {
        auto titleLabel = new QLabel(it->title + ":", ui->gridWidget);
        titleLabel->setSizePolicy(titleSizePolicy);
        titleLabel->setMinimumSize(QSize(100, 0));
        ui->gridLayout->addWidget(titleLabel, row, 0);

        if (it->type == Pi::ConfigItem::BOOL) {
            auto checkBox = new QCheckBox(ui->gridWidget);
            checkBox->setChecked(it->value == "True");
            ui->gridLayout->addWidget(checkBox, row, 1, Qt::AlignLeft);
            inputWidgets[row] = checkBox;
        } else {
            auto inputLineEdit = new QLineEdit(it->value, ui->gridWidget);
            ui->gridLayout->addWidget(inputLineEdit, row, 1);
            inputWidgets[row] = inputLineEdit;

            auto validEntryLabel = new QLabel(ui->gridWidget);
            validEntryLabel->setSizePolicy(validSizePolicy);
            validEntryLabel->setPixmap(QPixmap(QString::fromUtf8(":/img/exists.svg")));
            validEntryLabel->setAlignment(Qt::AlignCenter);
            ui->gridLayout->addWidget(validEntryLabel, row, 2);

            if (auto validator = it->validator)
                connect(inputLineEdit, &QLineEdit::textChanged, [=] (QString text) {
                    validatePass[row] = text.isEmpty() || validator(text);
                    validEntryLabel->setPixmap(QPixmap(validatePass[row] ? ":/img/exists.svg" : ":/img/not_exists.svg"));
                    ui->okButton->setEnabled(validatePass[row] && std::all_of(validatePass.begin(), validatePass.end(), [] (bool b) { return b; }));
                });
        }
    }

    connect(ui->okButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    connect(this, &QDialog::accepted, [&items, this] {
        int row = 0;
        for (auto it = items.begin(); it != items.end(); ++it, ++row) {
            if (QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(inputWidgets[row])) {
                if (!lineEdit->text().isEmpty())
                    it->value = lineEdit->text();
            } else if (QCheckBox *checkBox = dynamic_cast<QCheckBox*>(inputWidgets[row]))
                it->value = checkBox->isChecked() ? "True" : "";
        }
    });
}

PluginConfigDialog::~PluginConfigDialog()
{
    delete ui;
}

bool PluginConfigDialog::open(QString name, QList<Pi::ConfigItem> &items, QWidget *parent)
{
    PluginConfigDialog dialog(name, items, parent);
    return (dialog.exec() == QDialog::Accepted);
}
