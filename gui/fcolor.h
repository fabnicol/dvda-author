#ifndef FCOLOR_H
#define FCOLOR_H

#include <QtWidgets>
#include <QColorDialog>
#include <QPainter>


#define COLOR_LABEL_WIDTH 150
#define COLOR_LABEL_HEIGHT 20
#define DEFAULT_COLOR_0 "#FF0000" //red
#define DEFAULT_COLOR_1 "#00FF00" //green
#define DEFAULT_COLOR_2 "#0000FF" // blue

QString RGB2YCrCbStr(QColor& color);
QString RGBStr2YCrCbStr(const char* color);
QColor YCrCbStr2QColor(QString str);

class colorRect : public QWidget
{
private:

    QLabel *colorLabel;

public:
    colorRect(QColor color=Qt::red);
    void setWidth(int w);
    void setBrush(const QColor &b);
    virtual bool isAbstractEnabled() ;
};

#endif // FCOLOR_H
