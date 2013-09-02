#ifndef FCOLOR_H
#define FCOLOR_H

#include <QtWidgets>
#include <QColorDialog>
#include <QPainter>


#define COLOR_LABEL_WIDTH 150
#define COLOR_LABEL_HEIGHT 20
//#FF0000" red
//#00FF00" green
//#0000FF" blue
#define DEFAULT_COLOR_0 "red"
#define DEFAULT_COLOR_1 "#00FF00"
#define DEFAULT_COLOR_2 "turquoise"

QString RGB2YCrCbStr(QColor& color);
QString RGBStr2YCrCbStr(const QString  &color);
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


/*
    see files in doc/
*/


inline QString CONV(qreal X)
 { return QString::number(qFloor(X), 16).rightJustified(2, '0', true);}

inline qreal normalise(qreal X)
{
    if (X<0)   return 0;
    return (X>255)?255:X;
}

inline QString  RGB2YCrCbStr(QColor& color)
{
    qreal red=color.red();
    qreal green=color.green();
    qreal blue=color.blue();

    qreal Y = normalise(0.299 * red + 0.587 * green + 0.114 * blue) ;
    qreal Cb = normalise(-0.1687 * red  - 0.3313 * green + 0.5 * blue + 128);
    qreal Cr = normalise(0.5 * red - 0.4187 * green - 0.0813 * blue + 128) ;

    return CONV(Y) + CONV(Cr) + CONV(Cb);
}

inline QString  RGBStr2YCrCbStr(const QString&  s)
{
    QColor color;
    color.setNamedColor(QString(s));
    return RGB2YCrCbStr(color);
}

inline QColor YCrCbStr2QColor(QString str)
{

    if (str.length() < 6) return QColor(0,0,0);

    qreal Y =  str.mid(0,2).toInt(NULL, 16);
    qreal Cr = str.mid(2,2).toInt(NULL, 16);
    qreal Cb = str.mid(4,2).toInt(NULL, 16);

    qreal red = normalise(Y + 1.402 * (Cr-128));
    qreal green = normalise( Y - 0.34414 * (Cb-128) - 0.71414 * (Cr-128));
    qreal blue = normalise( Y + 1.772 * (Cb-128));

    return QColor(qFloor(red), qFloor(green),qFloor(blue));
}





#endif // FCOLOR_H
