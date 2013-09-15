#ifndef FORMS_H
#define FORMS_H

#include <QLabel>
#include <QHBoxLayout>
#include "enums.h"

class FRichLabel :public QWidget
{

public:
  FRichLabel(const QString& title, const QString& path, flags::font flag=flags::font::boldTitle);

};

#endif // FORMS_H
