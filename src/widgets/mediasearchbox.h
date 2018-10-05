#ifndef MEDIASEARCHBOX_H
#define MEDIASEARCHBOX_H

#include <QMenu>
#include <QMouseEvent>

#include "customlineedit.h"

class MediaProvider;

class MediaSearchBox : public CustomLineEdit {
    Q_OBJECT
public:
    explicit MediaSearchBox(QWidget *parent = 0);
    ~MediaSearchBox();

    void addProvider(MediaProvider *provider);
    void removeProvider(MediaProvider *provider);
    MediaProvider *getCurrentProvider() const { return currentProvider; }
    QString getWord() const { return word; }

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

signals:
    void providerChanged(MediaProvider *provider);
    void menuVisibilityChanging(bool visible);

private:
    MediaProvider *currentProvider = nullptr;
    QMenu *menuProviders = nullptr;
    QString word;
};

#endif // MEDIASEARCHBOX_H
