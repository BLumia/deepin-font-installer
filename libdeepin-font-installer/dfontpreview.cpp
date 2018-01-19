/*
 * Copyright (C) 2017 ~ 2017 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dfontpreview.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QTextStream>
#include <QPainter>
#include <QDebug>
#include <QFile>

static const QString lowerTextStock = "abcdefghijklmnopqrstuvwxyz";
static const QString upperTextStock = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const QString punctuationTextStock = "0123456789.:,;(*!?')";
static QString sampleString = nullptr;
static QString styleName = nullptr;
static QHash<QString, QString> contents = {};

DFontPreview::DFontPreview(QWidget *parent)
    : QWidget(parent),
      m_library(0),
      m_face(0),
      m_fontDatabase(new QFontDatabase)
{
    initContents();

    setFixedSize(QApplication::desktop()->width() / 1.5,
                 QApplication::desktop()->height() / 1.5);
}

DFontPreview::~DFontPreview()
{
}

void DFontPreview::setFileUrl(const QString &url)
{
    m_fontDatabase->removeAllApplicationFonts();
    m_fontDatabase->addApplicationFont(url);

    FT_Init_FreeType(&m_library);
    FT_New_Face(m_library, url.toUtf8().constData(), 0, &m_face);

    sampleString = getSampleString().simplified();
    styleName = (char *) m_face->style_name;

    FT_Done_Face(m_face);
    FT_Done_FreeType(m_library);

    repaint();
}

void DFontPreview::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QFont font(m_fontDatabase->applicationFontFamilies(0).first());
    painter.setPen(Qt::black);

    if (styleName.contains("Italic")) {
        font.setItalic(true);
    }

    if (styleName.contains("Regular")) {
        font.setWeight(QFont::Normal);
    } else if (styleName.contains("Bold")) {
        font.setWeight(QFont::Bold);
    } else if (styleName.contains("Light")) {
        font.setWeight(QFont::Light);
    } else if (styleName.contains("Thin")) {
        font.setWeight(QFont::Thin);
    } else if (styleName.contains("ExtraLight")) {
        font.setWeight(QFont::ExtraLight);
    } else if (styleName.contains("ExtraBold")) {
        font.setWeight(QFont::ExtraBold);
    } else if (styleName.contains("Medium")) {
        font.setWeight(QFont::Medium);
    } else if (styleName.contains("DemiBold")) {
        font.setWeight(QFont::DemiBold);
    } else if (styleName.contains("Black")) {
        font.setWeight(QFont::Black);
    }

    const int padding = 20;
    const int x = 35;
    int y = 10;
    int fontSize = 25;

    font.setPointSize(fontSize);
    painter.setFont(font);

    const QFontMetrics metrics(font);
    const int lowerWidth = metrics.width(lowerTextStock);
    const int lowerHeight = metrics.height();
    painter.drawText(QRect(x, y + padding, lowerWidth, lowerHeight), Qt::AlignLeft, lowerTextStock);
    y += lowerHeight;

    const int upperWidth = metrics.width(upperTextStock);
    const int upperHeight = metrics.height();
    painter.drawText(QRect(x, y + padding, upperWidth, upperHeight), Qt::AlignLeft, upperTextStock);
    y += upperHeight;

    const int punWidth = metrics.width(punctuationTextStock);
    int punHeight = metrics.height();
    painter.drawText(QRect(x, y + padding, punWidth, punHeight), Qt::AlignLeft, punctuationTextStock);
    y += punHeight;

    for (int i = 0; i < 10; ++i) {
        fontSize += 3;
        font.setPointSize(fontSize);
        painter.setFont(font);

        QFontMetrics met(font);
        int sampleWidth = met.width(sampleString);
        int sampleHeight = met.height();

        if (y + sampleHeight >= rect().height() - padding * 2)
            break;

        painter.drawText(QRect(x, y + padding * 2, sampleWidth , sampleHeight), Qt::AlignLeft, sampleString);
        y += sampleHeight + padding;
    }

    QWidget::paintEvent(e);
}

void DFontPreview::initContents()
{
    QFile file(":/CONTENTS.txt");

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray content = file.readAll();
    QTextStream stream(&content, QIODevice::ReadOnly);

    file.close();

    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        const QStringList items = line.split(QChar(':'));

        contents.insert(items.at(0), items.at(1));
    }
}

QString DFontPreview::getSampleString()
{
    QString sampleString = nullptr;
    bool isAvailable = false;

    // check the current system language sample string.
    sampleString = getLanguageSampleString(QLocale::system().name());
    if (checkFontContainText(sampleString) && !sampleString.isEmpty()) {
        isAvailable = true;
    }

    // check english sample string.
    if (!isAvailable) {
        sampleString = getLanguageSampleString("en");
        if (checkFontContainText(sampleString)) {
            isAvailable = true;
        }
    }

    // random string from available chars.
    if (!isAvailable) {
        sampleString = buildCharlistForFace(36);
    }

    return sampleString;
}

QString DFontPreview::getLanguageSampleString(const QString &language)
{
    QString result = nullptr;
    QString key = nullptr;

    if (contents.contains(language)) {
        key = language;
    } else {
        const QStringList parseList = language.split("_", QString::SkipEmptyParts);
        if (parseList.length() > 0 &&
            contents.contains(parseList.first())) {
            key = parseList.first();
        }
    }

    auto findResult = contents.find(key);
    result.append(findResult.value());

    return result;
}

bool DFontPreview::checkFontContainText(const QString &text)
{
    bool retval = true;

    FT_Select_Charmap(m_face, FT_ENCODING_UNICODE);

    for (auto ch : text) {
        if (!FT_Get_Char_Index(m_face, ch.unicode())) {
            retval = false;
            break;
        }
    }

    return retval;
}

QString DFontPreview::buildCharlistForFace(int length)
{
    unsigned int glyph;
    unsigned long ch;
    int totalChars = 0;
    QString retval;

    ch = FT_Get_First_Char(m_face, &glyph);

    while (glyph != 0) {
        retval.append(QChar((int) ch));
        ch = FT_Get_Next_Char(m_face, ch, &glyph);
        totalChars++;

        if (retval.count() == length)
            break;
    }

    return retval;
}