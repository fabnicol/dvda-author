#ifndef LPLEX_H
#define LPLEX_H


#include "common.h"

class lplexPage : public common
{
    Q_OBJECT

public:
    lplexPage();

private slots:
   void on_lplexDirButton_clicked();
   void on_lplexInfoDirButton_clicked();
   void on_lplexBackgroundButton_clicked();

private:
    FComboBox *lplexVideoType, *lplexSpliceType, *lplexShiftType, *lplexCreateType, *lplexMediaType;
    FLineEdit *lplexBackgroundLineEdit, *lplexInfoDirLineEdit, *lplexDirLineEdit;
    FCheckBox *lplexScreenParameterBox, *lplexMd5AwareBox, *lplexInfofilesBox, *lplexRescaleBox, *lplexInfoDirBox;
};


#endif // LPLEX_H
