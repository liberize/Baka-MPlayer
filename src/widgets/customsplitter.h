#ifndef CUSTOMSPLITTER_H
#define CUSTOMSPLITTER_H

#include <QSplitter>

class CustomSplitter : public QSplitter {
    Q_OBJECT
public:
    explicit CustomSplitter(QWidget *parent = 0);

    int position() const;
    int normalPosition() const;
    int max() const;

public slots:
    void setPosition(int pos);
    void setNormalPosition(int pos);

signals:
    void positionChanged(int pos);
    void entered();

private:
    int normalPos;
};

#endif // CUSTOMSPLITTER_H
