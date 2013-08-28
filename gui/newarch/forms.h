#ifndef FORMS_H
#define FORMS_H

#include <QLabel>
#include <QHBoxLayout>
#include "enums.h"

class FRichLabel :public QWidget
{

public:
  FRichLabel(const QString& title, const QString& path, int flag=flags::boldTitle);

};

#endif // FORMS_H
