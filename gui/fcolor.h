#ifndef FCOLOR_H
#define FCOLOR_H

#include <QtWidgets>
#include <QColorDialog>
#include <QPainter>


QString RGB2YCrCbStr(QColor& color);
QString RGBStr2YCrCbStr(const char* color);

class colorRect : public QWidget
{
private:
  QColor brush;
  QLabel *colorLabel;

public:
  colorRect(QColor color=Qt::red);
          //, int width=150, int x=0, int y=0);
  //void paintEvent(QPaintEvent* );
  //QSize sizeHint() const;
  void setWidth(int w);
  void setBrush(const QColor &b);
  virtual bool isAbstractEnabled() ;
};

#endif // FCOLOR_H
