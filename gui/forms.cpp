#include "forms.h"



FRichLabel::FRichLabel(const QString &title, const QString &path, flags::font flag) : QWidget()
{
  QHBoxLayout* mainLayout=new QHBoxLayout;
  QLabel *label=new QLabel;
  QLabel *label2=new QLabel;
  switch (flag)
    {
      case flags::font::regularTitle:
        label->setText(title);break;

      case flags::font::boldTitle:
        label->setText("<b>"+title+"</b>"); break;

      case flags::font::italicTitle:
        label->setText("<i>"+title+"</i>"); break;
    }

  label->setFixedHeight(20);
  label2->setPixmap(QPixmap(path).scaledToWidth(48, Qt::SmoothTransformation));

  mainLayout->addStretch(100);
  mainLayout->addWidget(label, Qt::AlignRight);
  mainLayout->addWidget(label2, Qt::AlignRight);
  setLayout(mainLayout);
}
