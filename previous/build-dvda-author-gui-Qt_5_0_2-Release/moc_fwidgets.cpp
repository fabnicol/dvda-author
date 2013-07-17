/****************************************************************************
** Meta object code from reading C++ file 'fwidgets.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.0.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../gui/fwidgets.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'fwidgets.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.0.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_FAbstractConnection_t {
    QByteArrayData data[1];
    char stringdata[21];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_FAbstractConnection_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_FAbstractConnection_t qt_meta_stringdata_FAbstractConnection = {
    {
QT_MOC_LITERAL(0, 0, 19)
    },
    "FAbstractConnection\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FAbstractConnection[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void FAbstractConnection::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject FAbstractConnection::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_FAbstractConnection.data,
      qt_meta_data_FAbstractConnection,  qt_static_metacall, 0, 0}
};


const QMetaObject *FAbstractConnection::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FAbstractConnection::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_FAbstractConnection.stringdata))
        return static_cast<void*>(const_cast< FAbstractConnection*>(this));
    return QObject::qt_metacast(_clname);
}

int FAbstractConnection::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
struct qt_meta_stringdata_FListWidget_t {
    QByteArrayData data[4];
    char stringdata[53];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_FListWidget_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_FListWidget_t qt_meta_stringdata_FListWidget = {
    {
QT_MOC_LITERAL(0, 0, 11),
QT_MOC_LITERAL(1, 12, 16),
QT_MOC_LITERAL(2, 29, 0),
QT_MOC_LITERAL(3, 30, 21)
    },
    "FListWidget\0open_tabs_signal\0\0"
    "is_signalList_changed\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FListWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   24,    2, 0x05,
       3,    1,   27,    2, 0x05,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::Int,    2,

       0        // eod
};

void FListWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        FListWidget *_t = static_cast<FListWidget *>(_o);
        switch (_id) {
        case 0: _t->open_tabs_signal((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->is_signalList_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (FListWidget::*_t)(int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&FListWidget::open_tabs_signal)) {
                *result = 0;
            }
        }
        {
            typedef void (FListWidget::*_t)(int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&FListWidget::is_signalList_changed)) {
                *result = 1;
            }
        }
    }
}

const QMetaObject FListWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_FListWidget.data,
      qt_meta_data_FListWidget,  qt_static_metacall, 0, 0}
};


const QMetaObject *FListWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FListWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_FListWidget.stringdata))
        return static_cast<void*>(const_cast< FListWidget*>(this));
    if (!strcmp(_clname, "FAbstractWidget"))
        return static_cast< FAbstractWidget*>(const_cast< FListWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int FListWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void FListWidget::open_tabs_signal(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void FListWidget::is_signalList_changed(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
struct qt_meta_stringdata_FCheckBox_t {
    QByteArrayData data[3];
    char stringdata[31];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_FCheckBox_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_FCheckBox_t qt_meta_stringdata_FCheckBox = {
    {
QT_MOC_LITERAL(0, 0, 9),
QT_MOC_LITERAL(1, 10, 18),
QT_MOC_LITERAL(2, 29, 0)
    },
    "FCheckBox\0uncheckDisabledBox\0\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FCheckBox[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x08,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void FCheckBox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        FCheckBox *_t = static_cast<FCheckBox *>(_o);
        switch (_id) {
        case 0: _t->uncheckDisabledBox(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject FCheckBox::staticMetaObject = {
    { &QCheckBox::staticMetaObject, qt_meta_stringdata_FCheckBox.data,
      qt_meta_data_FCheckBox,  qt_static_metacall, 0, 0}
};


const QMetaObject *FCheckBox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FCheckBox::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_FCheckBox.stringdata))
        return static_cast<void*>(const_cast< FCheckBox*>(this));
    if (!strcmp(_clname, "FAbstractWidget"))
        return static_cast< FAbstractWidget*>(const_cast< FCheckBox*>(this));
    return QCheckBox::qt_metacast(_clname);
}

int FCheckBox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QCheckBox::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_FRadioBox_t {
    QByteArrayData data[6];
    char stringdata[50];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_FRadioBox_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_FRadioBox_t qt_meta_stringdata_FRadioBox = {
    {
QT_MOC_LITERAL(0, 0, 9),
QT_MOC_LITERAL(1, 10, 7),
QT_MOC_LITERAL(2, 18, 0),
QT_MOC_LITERAL(3, 19, 5),
QT_MOC_LITERAL(4, 25, 9),
QT_MOC_LITERAL(5, 35, 13)
    },
    "FRadioBox\0toggled\0\0value\0toggledTo\0"
    "resetRadioBox\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FRadioBox[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x05,

 // slots: name, argc, parameters, tag, flags
       4,    1,   32,    2, 0x08,
       5,    1,   35,    2, 0x08,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,    3,

       0        // eod
};

void FRadioBox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        FRadioBox *_t = static_cast<FRadioBox *>(_o);
        switch (_id) {
        case 0: _t->toggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->toggledTo((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->resetRadioBox((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (FRadioBox::*_t)(bool );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&FRadioBox::toggled)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject FRadioBox::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_FRadioBox.data,
      qt_meta_data_FRadioBox,  qt_static_metacall, 0, 0}
};


const QMetaObject *FRadioBox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FRadioBox::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_FRadioBox.stringdata))
        return static_cast<void*>(const_cast< FRadioBox*>(this));
    if (!strcmp(_clname, "FAbstractWidget"))
        return static_cast< FAbstractWidget*>(const_cast< FRadioBox*>(this));
    return QWidget::qt_metacast(_clname);
}

int FRadioBox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void FRadioBox::toggled(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_FComboBox_t {
    QByteArrayData data[4];
    char stringdata[51];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_FComboBox_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_FComboBox_t qt_meta_stringdata_FComboBox = {
    {
QT_MOC_LITERAL(0, 0, 9),
QT_MOC_LITERAL(1, 10, 21),
QT_MOC_LITERAL(2, 32, 0),
QT_MOC_LITERAL(3, 33, 16)
    },
    "FComboBox\0is_signalList_changed\0\0"
    "fromCurrentIndex\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FComboBox[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   24,    2, 0x05,

 // slots: name, argc, parameters, tag, flags
       3,    1,   27,    2, 0x08,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    2,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    2,

       0        // eod
};

void FComboBox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        FComboBox *_t = static_cast<FComboBox *>(_o);
        switch (_id) {
        case 0: _t->is_signalList_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->fromCurrentIndex((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (FComboBox::*_t)(int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&FComboBox::is_signalList_changed)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject FComboBox::staticMetaObject = {
    { &QComboBox::staticMetaObject, qt_meta_stringdata_FComboBox.data,
      qt_meta_data_FComboBox,  qt_static_metacall, 0, 0}
};


const QMetaObject *FComboBox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FComboBox::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_FComboBox.stringdata))
        return static_cast<void*>(const_cast< FComboBox*>(this));
    if (!strcmp(_clname, "FAbstractWidget"))
        return static_cast< FAbstractWidget*>(const_cast< FComboBox*>(this));
    return QComboBox::qt_metacast(_clname);
}

int FComboBox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QComboBox::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void FComboBox::is_signalList_changed(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_FLineEdit_t {
    QByteArrayData data[1];
    char stringdata[11];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_FLineEdit_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_FLineEdit_t qt_meta_stringdata_FLineEdit = {
    {
QT_MOC_LITERAL(0, 0, 9)
    },
    "FLineEdit\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FLineEdit[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void FLineEdit::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject FLineEdit::staticMetaObject = {
    { &QLineEdit::staticMetaObject, qt_meta_stringdata_FLineEdit.data,
      qt_meta_data_FLineEdit,  qt_static_metacall, 0, 0}
};


const QMetaObject *FLineEdit::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FLineEdit::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_FLineEdit.stringdata))
        return static_cast<void*>(const_cast< FLineEdit*>(this));
    if (!strcmp(_clname, "FAbstractWidget"))
        return static_cast< FAbstractWidget*>(const_cast< FLineEdit*>(this));
    return QLineEdit::qt_metacast(_clname);
}

int FLineEdit::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLineEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
struct qt_meta_stringdata_FColorButton_t {
    QByteArrayData data[3];
    char stringdata[28];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_FColorButton_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_FColorButton_t qt_meta_stringdata_FColorButton = {
    {
QT_MOC_LITERAL(0, 0, 12),
QT_MOC_LITERAL(1, 13, 12),
QT_MOC_LITERAL(2, 26, 0)
    },
    "FColorButton\0changeColors\0\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FColorButton[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x0a,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void FColorButton::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        FColorButton *_t = static_cast<FColorButton *>(_o);
        switch (_id) {
        case 0: _t->changeColors(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject FColorButton::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_FColorButton.data,
      qt_meta_data_FColorButton,  qt_static_metacall, 0, 0}
};


const QMetaObject *FColorButton::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FColorButton::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_FColorButton.stringdata))
        return static_cast<void*>(const_cast< FColorButton*>(this));
    if (!strcmp(_clname, "FAbstractWidget"))
        return static_cast< FAbstractWidget*>(const_cast< FColorButton*>(this));
    return QWidget::qt_metacast(_clname);
}

int FColorButton::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_FPalette_t {
    QByteArrayData data[1];
    char stringdata[10];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_FPalette_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_FPalette_t qt_meta_stringdata_FPalette = {
    {
QT_MOC_LITERAL(0, 0, 8)
    },
    "FPalette\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FPalette[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void FPalette::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject FPalette::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_FPalette.data,
      qt_meta_data_FPalette,  qt_static_metacall, 0, 0}
};


const QMetaObject *FPalette::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FPalette::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_FPalette.stringdata))
        return static_cast<void*>(const_cast< FPalette*>(this));
    if (!strcmp(_clname, "FAbstractWidget"))
        return static_cast< FAbstractWidget*>(const_cast< FPalette*>(this));
    return QWidget::qt_metacast(_clname);
}

int FPalette::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
