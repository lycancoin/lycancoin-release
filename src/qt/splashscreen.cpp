#include "splashscreen.h"
#include "clientversion.h"
#include "util.h"
#include "chainparams.h"

#include <QApplication>
#include <QPainter>

SplashScreen::SplashScreen(const QPixmap &pixmap, Qt::WindowFlags f) :
    QSplashScreen(pixmap, f)
{
    // set reference point, paddings
    int paddingBottom           = 385;
    int paddingLeft             = 330;
    int titleVersionVSpace      = 17;
    int titleCopyrightVSpace    = 40;

    float fontFactor            = 1.0;

    // define text to place
    QString titleText       = QString(QApplication::applicationName()).replace(QString("-testnet"), QString(""), Qt::CaseSensitive); // cut of testnet, place it as single object further down
    QString versionText     = QString("Version %1").arg(QString::fromStdString(FormatFullVersion()));
    QString copyrightText   = QChar(0xA9)+QString(" 2014-%1 ").arg(COPYRIGHT_YEAR) + QString(tr("The Lycancoin Developers"));
    QString testnetAddText  = QString(tr("[testnet]")); // define text to place as single text object

    QString font            = "Arial";

    // load the bitmap for writing some text over it
    QPixmap newPixmap;
    if(TestNet()) {
        newPixmap     = QPixmap(":/images/splash");
    }
    else {
        newPixmap     = QPixmap(":/images/splash");
    }

    QPainter pixPaint(&newPixmap);
    pixPaint.setPen(QColor(190,190,190));

    // check font size and drawing with
    pixPaint.setFont(QFont(font, 20*fontFactor));
    QFontMetrics fm = pixPaint.fontMetrics();
    int titleTextWidth  = fm.width(titleText);
    if(titleTextWidth > 160) {
        // strange font rendering, Arial probably not found
        fontFactor = 0.75;
    }

    pixPaint.setFont(QFont(font, 33*fontFactor));
    fm = pixPaint.fontMetrics();
    titleTextWidth  = fm.width(titleText);
//    pixPaint.drawText(newPixmap.width()-titleTextWidth-paddingLeft,paddingBottom,titleText);

    pixPaint.setFont(QFont(font, 12*fontFactor));

    // if the version string is to long, reduce size
    fm = pixPaint.fontMetrics();
    int versionTextWidth  = fm.width(versionText);
    if(versionTextWidth > titleTextWidth+paddingLeft-10) {
        pixPaint.setFont(QFont(font, 5*fontFactor));
        titleVersionVSpace -= 5;
    }
    pixPaint.drawText(newPixmap.width()-titleTextWidth-paddingLeft+2,paddingBottom+titleVersionVSpace,versionText);

    // draw copyright stuff
    pixPaint.setFont(QFont(font, 8*fontFactor));
    pixPaint.drawText(newPixmap.width()-titleTextWidth-paddingLeft,paddingBottom+titleCopyrightVSpace,copyrightText);

    // draw testnet string if -testnet is on
    if(QApplication::applicationName().contains(QString("-testnet"))) {
        // draw copyright stuff
        QFont boldFont = QFont(font, 10*fontFactor);
        boldFont.setWeight(QFont::Bold);
        pixPaint.setFont(boldFont);
        fm = pixPaint.fontMetrics();
        int testnetAddTextWidth  = fm.width(testnetAddText);
        pixPaint.drawText(newPixmap.width()-testnetAddTextWidth-10,15,testnetAddText);
    }

    pixPaint.end();

    this->setPixmap(newPixmap);
}