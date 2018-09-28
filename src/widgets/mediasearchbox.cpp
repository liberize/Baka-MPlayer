#include "mediasearchbox.h"
#include "mediaprovider.h"


MediaSearchBox::MediaSearchBox(QWidget *parent):
    CustomLineEdit(parent)
{
    setIcon(QIcon(":/img/search_dropdown.svg"), QSize(16, 16));
    menuProviders = new QMenu(this);
    menuProviders->setStyleSheet(
        "QMenu {\n"
        "  padding: 0;\n"
        "}\n"
        "QMenu::item {\n"
        "  height: 24px;\n"
        "  padding-left: 28px;\n"
        "  padding-right: 8px;\n"
        "  background: #5b5b5b;\n"
        "}\n"
        "QMenu::item:selected {\n"
        "  border: none;\n"
        "  background: #0770DD;\n"
        "}\n"
        "QMenu::icon {\n"
        "  margin-left: 8px;\n"
        "}\n");

    connect(this, &CustomLineEdit::submitted, [=] (QString s) {
        word = s;
    });
}

MediaSearchBox::~MediaSearchBox()
{
    delete menuProviders;
}

void MediaSearchBox::addProvider(MediaProvider *provider)
{
    QAction *action = menuProviders->addAction(provider->getIcon(), provider->getName());
    action->setCheckable(true);
    connect(action, &QAction::triggered, [=] {
        if (currentProvider != provider) {
            setIcon(provider->getIcon(), QSize(16, 16));
            currentProvider = provider;
            emit providerChanged(provider);
        } else
            action->setChecked(true);
    });
}

void MediaSearchBox::removeProvider(MediaProvider *provider)
{
    QList<QAction*> actions = menuProviders->actions();
    for (auto &action : actions) {
        if (action->text() == provider->getName()) {
            menuProviders->removeAction(action);
            break;
        }
    }
    if (currentProvider == provider) {
        setIcon(QIcon(":/img/search_dropdown.svg"), QSize(16, 16));
        currentProvider = nullptr;
        emit providerChanged(nullptr);
    }
}

void MediaSearchBox::mouseMoveEvent(QMouseEvent *event)
{
    QRect rect(0, 0, iconWidth(), height());
    setCursor(rect.contains(event->pos()) ? Qt::PointingHandCursor : Qt::IBeamCursor);
    QLineEdit::mouseMoveEvent(event);
}

void MediaSearchBox::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QRect rect(0, 0, iconWidth(), height());
        if (rect.contains(event->pos()) && !menuProviders->actions().isEmpty()) {
            QPoint pos(0, height());
            menuProviders->exec(mapToGlobal(pos));
        }
    }
}
