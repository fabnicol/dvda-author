/****************************************************************************
** Meta object code from reading C++ file 'dvda-author-gui.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.0.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../gui/dvda-author-gui.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dvda-author-gui.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.0.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[12];
    char stringdata[223];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_MainWindow_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10),
QT_MOC_LITERAL(1, 11, 21),
QT_MOC_LITERAL(2, 33, 0),
QT_MOC_LITERAL(3, 34, 30),
QT_MOC_LITERAL(4, 65, 36),
QT_MOC_LITERAL(5, 102, 28),
QT_MOC_LITERAL(6, 131, 24),
QT_MOC_LITERAL(7, 156, 14),
QT_MOC_LITERAL(8, 171, 9),
QT_MOC_LITERAL(9, 181, 16),
QT_MOC_LITERAL(10, 198, 5),
QT_MOC_LITERAL(11, 204, 17)
    },
    "MainWindow\0on_exitButton_clicked\0\0"
    "on_displayOutputButton_clicked\0"
    "on_displayFileTreeViewButton_clicked\0"
    "on_editProjectButton_clicked\0"
    "on_optionsButton_clicked\0showMainWidget\0"
    "configure\0configureOptions\0about\0"
    "on_activate_lplex\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   74,    2, 0x08,
       3,    0,   75,    2, 0x08,
       4,    1,   76,    2, 0x08,
       4,    0,   79,    2, 0x08,
       5,    0,   80,    2, 0x08,
       6,    0,   81,    2, 0x08,
       7,    0,   82,    2, 0x08,
       7,    1,   83,    2, 0x08,
       8,    0,   86,    2, 0x08,
       9,    0,   87,    2, 0x08,
      10,    0,   88,    2, 0x08,
      11,    1,   89,    2, 0x08,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    2,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        MainWindow *_t = static_cast<MainWindow *>(_o);
        switch (_id) {
        case 0: _t->on_exitButton_clicked(); break;
        case 1: _t->on_displayOutputButton_clicked(); break;
        case 2: _t->on_displayFileTreeViewButton_clicked((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->on_displayFileTreeViewButton_clicked(); break;
        case 4: _t->on_editProjectButton_clicked(); break;
        case 5: _t->on_optionsButton_clicked(); break;
        case 6: _t->showMainWidget(); break;
        case 7: _t->showMainWidget((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->configure(); break;
        case 9: _t->configureOptions(); break;
        case 10: _t->about(); break;
        case 11: _t->on_activate_lplex((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject MainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_MainWindow.data,
      qt_meta_data_MainWindow,  qt_static_metacall, 0, 0}
};


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata))
        return static_cast<void*>(const_cast< MainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 12;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
