#include <QtWidgets/QApplication>
#include "widget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.Read_Arguments(argc, argv);
    w.show();
    return 0;
}
