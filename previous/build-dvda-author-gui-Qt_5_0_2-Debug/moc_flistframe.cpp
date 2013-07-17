/****************************************************************************
** Meta object code from reading C++ file 'flistframe.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.0.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../gui/flistframe.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'flistframe.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.0.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_FListFrame_t {
    QByteArrayData data[14];
    char stringdata[263];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_FListFrame_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_FListFrame_t qt_meta_stringdata_FListFrame = {
    {
QT_MOC_LITERAL(0, 0, 10),
QT_MOC_LITERAL(1, 11, 21),
QT_MOC_LITERAL(2, 33, 0),
QT_MOC_LITERAL(3, 34, 8),
QT_MOC_LITERAL(4, 43, 11),
QT_MOC_LITERAL(5, 55, 21),
QT_MOC_LITERAL(6, 77, 20),
QT_MOC_LITERAL(7, 98, 12),
QT_MOC_LITERAL(8, 111, 29),
QT_MOC_LITERAL(9, 141, 29),
QT_MOC_LITERAL(10, 171, 27),
QT_MOC_LITERAL(11, 199, 9),
QT_MOC_LITERAL(12, 209, 23),
QT_MOC_LITERAL(13, 233, 28)
    },
    "FListFrame\0is_signalList_changed\0\0"
    "addGroup\0deleteGroup\0on_deleteItem_clicked\0"
    "on_clearList_clicked\0currentIndex\0"
    "on_importFromMainTree_clicked\0"
    "on_moveDownItemButton_clicked\0"
    "on_moveUpItemButton_clicked\0addGroups\0"
    "on_mainTabIndex_changed\0"
    "on_embeddingTabIndex_changed\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FListFrame[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   84,    2, 0x05,

 // slots: name, argc, parameters, tag, flags
       3,    0,   87,    2, 0x0a,
       4,    0,   88,    2, 0x0a,
       5,    0,   89,    2, 0x0a,
       6,    1,   90,    2, 0x0a,
       6,    0,   93,    2, 0x2a,
       8,    0,   94,    2, 0x09,
       9,    0,   95,    2, 0x09,
      10,    0,   96,    2, 0x09,
      11,    1,   97,    2, 0x09,
      12,    1,  100,    2, 0x09,
      12,    0,  103,    2, 0x29,
      13,    1,  104,    2, 0x09,
      13,    0,  107,    2, 0x29,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    2,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void,

       0        // eod
};

void FListFrame::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        FListFrame *_t = static_cast<FListFrame *>(_o);
        switch (_id) {
        case 0: _t->is_signalList_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->addGroup(); break;
        case 2: _t->deleteGroup(); break;
        case 3: _t->on_deleteItem_clicked(); break;
        case 4: _t->on_clearList_clicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->on_clearList_clicked(); break;
        case 6: _t->on_importFromMainTree_clicked(); break;
        case 7: _t->on_moveDownItemButton_clicked(); break;
        case 8: _t->on_moveUpItemButton_clicked(); break;
        case 9: _t->addGroups((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->on_mainTabIndex_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 11: _t->on_mainTabIndex_changed(); break;
        case 12: _t->on_embeddingTabIndex_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 13: _t->on_embeddingTabIndex_changed(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (FListFrame::*_t)(int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&FListFrame::is_signalList_changed)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject FListFrame::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_FListFrame.data,
      qt_meta_data_FListFrame,  qt_static_metacall, 0, 0}
};


const QMetaObject *FListFrame::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FListFrame::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_FListFrame.stringdata))
        return static_cast<void*>(const_cast< FListFrame*>(this));
    return QWidget::qt_metacast(_clname);
}

int FListFrame::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void FListFrame::is_signalList_changed(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
