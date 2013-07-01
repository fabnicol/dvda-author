/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "highlighter.h"
#include "fwidgets.h"

inline void Highlighter::createRulePattern(QColor color,  QFont::Weight weight,  QStringList list)
{
    HighlightingRule rule;
    QTextCharFormat classFormat;

    classFormat.setForeground(color);
    classFormat.setFontWeight(weight);
    rule.format = classFormat;

    for (QString a : list)
    {
        rule.pattern = QRegExp(a);
        highlightingRules.append(rule);
    }
}


Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::darkMagenta);
    keywordFormat.setFontWeight(QFont::Bold);

    for (FAbstractWidget* a : Abstract::abstractWidgetList)
    {
        rule.pattern = QRegExp("\\b"+a->getHashKey()+"\\b");
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    createRulePattern(Qt::darkBlue, QFont::Bold, {"\\bwidgetDepth\\b"});

    createRulePattern(QColor("maroon"), QFont::Bold, {"\\bgroup\\b", "\\btitleset\\b", "\\btrack\\b", "\\bmenu\\b", "\\bYCrCb\\b"});

    createRulePattern(QColor("orange"), QFont::Bold, {"\\bfile\\b", "\\bslide\\b"});

    createRulePattern(Qt::darkGreen, QFont::Black, {"\".*\""});

    createRulePattern(Qt::red, QFont::Black, {"<[/]?", ">", "data>", "system>", "recent>", "project>", "<\\?xml"});
}

void Highlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules)
    {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }

}

