#ifndef FCOLOR_H
#define FCOLOR_H

#include <QWidget>
#include <QColorDialog>
#include <QWidget>
#include <QPainter>

//QString getYUVColors(u_int8_t& R, u_int8_t& G, u_int8_t& B);
//QString RGB2YCrCbStr(qreal R, qreal G, qreal B);
//QRgb YCrCb2QRGB(qreal Y, qreal Cr, qreal Cb);
//QRgb YCrCbStr2QRGB(QString yuvString);
//QString RGBStr2YUVStr(QString rgbString);

//void getRGBColors(int *red, int * green, int * blue);

class colorRect : public QWidget
{
private:
  QBrush brush;
  QRect rect;


public:
  colorRect(QColor color=Qt::red, int width=150, int x=0, int y=0)
  {
    rect.setRect(0, 0, width, 12);
    brush=QBrush(color);
  }
  void paintEvent(QPaintEvent* );
  QSize sizeHint() const;
  void setWidth(int w){rect.setWidth(w);}
  void setBrush(const QBrush &b) {brush=b;}
  virtual bool isAbstractEnabled() {return this->isEnabled();}
};

#endif // FCOLOR_H
