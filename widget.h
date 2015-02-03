#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "wrp.h"

namespace Ui {
    class Widget;
}

class Widget : public QWidget {
    Q_OBJECT
public:
    Widget(QWidget *parent = 0);
    ~Widget();

    void Create_Image(char *ofpsource, int satres, char *satbmp);
    int ReadFile(char *wrpname);
    void Prepare_TextStrings();
    void Write_Texture_Names(QString fileName);
    void Read_Arguments(int argc, char *argv[]);

    // snake global variables ;)

    // 32 chars in array of 512.
    char TexStrings[512][32];

    // texture indexes
    short TexIndex;

    // wrp size
    int MapSize;

protected:
    void changeEvent(QEvent *e);

private:
    Ui::Widget *ui;
};

#endif // WIDGET_H
