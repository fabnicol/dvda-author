#include "fcolor.h"
#include "fstring.h"


/* Society of Motion Picture and Television Engineers,
"Television - Signal Parameters - 1125-Line High-Definition Production", SMPTE 240M-1999.
see file doc/rgb2yuv.pdf
*/

qreal Kry =0.212;
qreal Kby =0.087;
qreal Kgy= 0.701;

#define CONV(X)  QString::number(qFloor(X), 16)

QString  RGB2YCrCbStr(QColor& color)
{
    qreal red=color.red();
    qreal green=color.green();
    qreal blue=color.blue();

    qreal Y = Kry * red + Kgy * green + Kby * blue ;
    qreal Cb = blue - Y;
    qreal Cr = red - Y ;

    return CONV(Y) + CONV(Cr) + CONV(Cb);

}

QString  RGBStr2YCrCbStr(const char* s)
{
    QColor color;
    color.setNamedColor(QString(s));
    return RGB2YCrCbStr(color);
}

QColor YCrCbStr2QColor(QString str)
{
    if (str.length() < 6) return QColor(0,0,0);

    qreal Y =  str.mid(0,2).toFloat();
    qreal Cr = str.mid(2,2).toFloat();
    qreal Cb = str.mid(4,2).toFloat();

    qreal red = Y + Cr;
    qreal green = Y - (Kby / Kgy) *Cb - (Kry / Kgy) *Cr;
    qreal blue = Y + Cb;

    return QColor(qFloor(red), qFloor(green),qFloor(blue));
}




