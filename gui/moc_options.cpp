/****************************************************************************
** Meta object code from reading C++ file 'options.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.0.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "options.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'options.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.0.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_standardPage_t {
    QByteArrayData data[5];
    char stringdata[61];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_standardPage_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_standardPage_t qt_meta_stringdata_standardPage = {
    {
QT_MOC_LITERAL(0, 0, 12),
QT_MOC_LITERAL(1, 13, 17),
QT_MOC_LITERAL(2, 31, 0),
QT_MOC_LITERAL(3, 32, 16),
QT_MOC_LITERAL(4, 49, 10)
    },
    "standardPage\0changeAspectRatio\0\0"
    "QListWidgetItem*\0changeNorm\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_standardPage[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    2,   24,    2, 0x0a,
       4,    2,   29,    2, 0x0a,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3,    2,    2,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3,    2,    2,

       0        // eod
};

void standardPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        standardPage *_t = static_cast<standardPage *>(_o);
        switch (_id) {
        case 0: _t->changeAspectRatio((*reinterpret_cast< QListWidgetItem*(*)>(_a[1])),(*reinterpret_cast< QListWidgetItem*(*)>(_a[2]))); break;
        case 1: _t->changeNorm((*reinterpret_cast< QListWidgetItem*(*)>(_a[1])),(*reinterpret_cast< QListWidgetItem*(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObject standardPage::staticMetaObject = {
    { &common::staticMetaObject, qt_meta_stringdata_standardPage.data,
      qt_meta_data_standardPage,  qt_static_metacall, 0, 0}
};


const QMetaObject *standardPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *standardPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_standardPage.stringdata))
        return static_cast<void*>(const_cast< standardPage*>(this));
    return common::qt_metacast(_clname);
}

int standardPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = common::qt_metacall(_c, _id, _a);
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
struct qt_meta_stringdata_videolinkPage_t {
    QByteArrayData data[3];
    char stringdata[43];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_videolinkPage_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_videolinkPage_t qt_meta_stringdata_videolinkPage = {
    {
QT_MOC_LITERAL(0, 0, 13),
QT_MOC_LITERAL(1, 14, 26),
QT_MOC_LITERAL(2, 41, 0)
    },
    "videolinkPage\0on_videolinkButton_clicked\0"
    "\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_videolinkPage[] = {

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

void videolinkPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        videolinkPage *_t = static_cast<videolinkPage *>(_o);
        switch (_id) {
        case 0: _t->on_videolinkButton_clicked(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject videolinkPage::staticMetaObject = {
    { &common::staticMetaObject, qt_meta_stringdata_videolinkPage.data,
      qt_meta_data_videolinkPage,  qt_static_metacall, 0, 0}
};


const QMetaObject *videolinkPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *videolinkPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_videolinkPage.stringdata))
        return static_cast<void*>(const_cast< videolinkPage*>(this));
    return common::qt_metacast(_clname);
}

int videolinkPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = common::qt_metacall(_c, _id, _a);
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
struct qt_meta_stringdata_optionsPage_t {
    QByteArrayData data[5];
    char stringdata[72];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_optionsPage_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_optionsPage_t qt_meta_stringdata_optionsPage = {
    {
QT_MOC_LITERAL(0, 0, 11),
QT_MOC_LITERAL(1, 12, 24),
QT_MOC_LITERAL(2, 37, 0),
QT_MOC_LITERAL(3, 38, 24),
QT_MOC_LITERAL(4, 63, 7)
    },
    "optionsPage\0on_mkisofsButton_clicked\0"
    "\0dvdwriterCheckEditStatus\0checked\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_optionsPage[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   24,    2, 0x08,
       3,    1,   25,    2, 0x08,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    4,

       0        // eod
};

void optionsPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        optionsPage *_t = static_cast<optionsPage *>(_o);
        switch (_id) {
        case 0: _t->on_mkisofsButton_clicked(); break;
        case 1: _t->dvdwriterCheckEditStatus((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject optionsPage::staticMetaObject = {
    { &common::staticMetaObject, qt_meta_stringdata_optionsPage.data,
      qt_meta_data_optionsPage,  qt_static_metacall, 0, 0}
};


const QMetaObject *optionsPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *optionsPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_optionsPage.stringdata))
        return static_cast<void*>(const_cast< optionsPage*>(this));
    return common::qt_metacast(_clname);
}

int optionsPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = common::qt_metacall(_c, _id, _a);
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
struct qt_meta_stringdata_advancedPage_t {
    QByteArrayData data[3];
    char stringdata[44];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_advancedPage_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_advancedPage_t qt_meta_stringdata_advancedPage = {
    {
QT_MOC_LITERAL(0, 0, 12),
QT_MOC_LITERAL(1, 13, 28),
QT_MOC_LITERAL(2, 42, 0)
    },
    "advancedPage\0on_extraAudioFilters_changed\0"
    "\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_advancedPage[] = {

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
       1,    1,   19,    2, 0x08,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    2,

       0        // eod
};

void advancedPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        advancedPage *_t = static_cast<advancedPage *>(_o);
        switch (_id) {
        case 0: _t->on_extraAudioFilters_changed((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject advancedPage::staticMetaObject = {
    { &common::staticMetaObject, qt_meta_stringdata_advancedPage.data,
      qt_meta_data_advancedPage,  qt_static_metacall, 0, 0}
};


const QMetaObject *advancedPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *advancedPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_advancedPage.stringdata))
        return static_cast<void*>(const_cast< advancedPage*>(this));
    return common::qt_metacast(_clname);
}

int advancedPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = common::qt_metacall(_c, _id, _a);
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
struct qt_meta_stringdata_outputPage_t {
    QByteArrayData data[10];
    char stringdata[218];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_outputPage_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_outputPage_t qt_meta_stringdata_outputPage = {
    {
QT_MOC_LITERAL(0, 0, 10),
QT_MOC_LITERAL(1, 11, 20),
QT_MOC_LITERAL(2, 32, 0),
QT_MOC_LITERAL(3, 33, 24),
QT_MOC_LITERAL(4, 58, 28),
QT_MOC_LITERAL(5, 87, 28),
QT_MOC_LITERAL(6, 116, 28),
QT_MOC_LITERAL(7, 145, 27),
QT_MOC_LITERAL(8, 173, 30),
QT_MOC_LITERAL(9, 204, 12)
    },
    "outputPage\0on_logButton_clicked\0\0"
    "on_openLogButton_clicked\0"
    "on_openHtmlLogButton_clicked\0"
    "on_openWorkDirButton_clicked\0"
    "on_openTempDirButton_clicked\0"
    "on_openBinDirButton_clicked\0"
    "on_openTargetDirButton_clicked\0"
    "selectOutput\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_outputPage[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x08,
       3,    0,   55,    2, 0x08,
       4,    0,   56,    2, 0x08,
       5,    0,   57,    2, 0x08,
       6,    0,   58,    2, 0x08,
       7,    0,   59,    2, 0x08,
       8,    0,   60,    2, 0x08,
       9,    0,   61,    2, 0x08,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void outputPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        outputPage *_t = static_cast<outputPage *>(_o);
        switch (_id) {
        case 0: _t->on_logButton_clicked(); break;
        case 1: _t->on_openLogButton_clicked(); break;
        case 2: _t->on_openHtmlLogButton_clicked(); break;
        case 3: _t->on_openWorkDirButton_clicked(); break;
        case 4: _t->on_openTempDirButton_clicked(); break;
        case 5: _t->on_openBinDirButton_clicked(); break;
        case 6: _t->on_openTargetDirButton_clicked(); break;
        case 7: _t->selectOutput(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject outputPage::staticMetaObject = {
    { &common::staticMetaObject, qt_meta_stringdata_outputPage.data,
      qt_meta_data_outputPage,  qt_static_metacall, 0, 0}
};


const QMetaObject *outputPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *outputPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_outputPage.stringdata))
        return static_cast<void*>(const_cast< outputPage*>(this));
    return common::qt_metacast(_clname);
}

int outputPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = common::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}
struct qt_meta_stringdata_audioMenuPage_t {
    QByteArrayData data[10];
    char stringdata[146];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_audioMenuPage_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_audioMenuPage_t qt_meta_stringdata_audioMenuPage = {
    {
QT_MOC_LITERAL(0, 0, 13),
QT_MOC_LITERAL(1, 14, 17),
QT_MOC_LITERAL(2, 32, 0),
QT_MOC_LITERAL(3, 33, 26),
QT_MOC_LITERAL(4, 60, 19),
QT_MOC_LITERAL(5, 80, 23),
QT_MOC_LITERAL(6, 104, 15),
QT_MOC_LITERAL(7, 120, 5),
QT_MOC_LITERAL(8, 126, 13),
QT_MOC_LITERAL(9, 140, 4)
    },
    "audioMenuPage\0launchImageViewer\0\0"
    "on_audioMenuButton_clicked\0"
    "on_frameTab_changed\0on_slidesButton_clicked\0"
    "setMinimumNMenu\0value\0readFontSizes\0"
    "rank\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_audioMenuPage[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   44,    2, 0x08,
       3,    0,   45,    2, 0x08,
       4,    1,   46,    2, 0x08,
       5,    0,   49,    2, 0x08,
       6,    1,   50,    2, 0x08,
       8,    1,   53,    2, 0x08,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Int,    9,

       0        // eod
};

void audioMenuPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        audioMenuPage *_t = static_cast<audioMenuPage *>(_o);
        switch (_id) {
        case 0: _t->launchImageViewer(); break;
        case 1: _t->on_audioMenuButton_clicked(); break;
        case 2: _t->on_frameTab_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->on_slidesButton_clicked(); break;
        case 4: _t->setMinimumNMenu((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->readFontSizes((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject audioMenuPage::staticMetaObject = {
    { &common::staticMetaObject, qt_meta_stringdata_audioMenuPage.data,
      qt_meta_data_audioMenuPage,  qt_static_metacall, 0, 0}
};


const QMetaObject *audioMenuPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *audioMenuPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_audioMenuPage.stringdata))
        return static_cast<void*>(const_cast< audioMenuPage*>(this));
    return common::qt_metacast(_clname);
}

int audioMenuPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = common::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}
struct qt_meta_stringdata_videoMenuPage_t {
    QByteArrayData data[5];
    char stringdata[111];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_videoMenuPage_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_videoMenuPage_t qt_meta_stringdata_videoMenuPage = {
    {
QT_MOC_LITERAL(0, 0, 13),
QT_MOC_LITERAL(1, 14, 32),
QT_MOC_LITERAL(2, 47, 0),
QT_MOC_LITERAL(3, 48, 28),
QT_MOC_LITERAL(4, 77, 32)
    },
    "videoMenuPage\0on_openVideoImportButton_clicked\0"
    "\0on_videoImportButton_clicked\0"
    "on_videoMenuImportButton_clicked\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_videoMenuPage[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x08,
       3,    0,   30,    2, 0x08,
       4,    0,   31,    2, 0x08,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void videoMenuPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        videoMenuPage *_t = static_cast<videoMenuPage *>(_o);
        switch (_id) {
        case 0: _t->on_openVideoImportButton_clicked(); break;
        case 1: _t->on_videoImportButton_clicked(); break;
        case 2: _t->on_videoMenuImportButton_clicked(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject videoMenuPage::staticMetaObject = {
    { &common::staticMetaObject, qt_meta_stringdata_videoMenuPage.data,
      qt_meta_data_videoMenuPage,  qt_static_metacall, 0, 0}
};


const QMetaObject *videoMenuPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *videoMenuPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_videoMenuPage.stringdata))
        return static_cast<void*>(const_cast< videoMenuPage*>(this));
    return common::qt_metacast(_clname);
}

int videoMenuPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = common::qt_metacall(_c, _id, _a);
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
struct qt_meta_stringdata_stillPage_t {
    QByteArrayData data[8];
    char stringdata[150];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_stillPage_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_stillPage_t qt_meta_stringdata_stillPage = {
    {
QT_MOC_LITERAL(0, 0, 9),
QT_MOC_LITERAL(1, 10, 19),
QT_MOC_LITERAL(2, 30, 0),
QT_MOC_LITERAL(3, 31, 20),
QT_MOC_LITERAL(4, 52, 35),
QT_MOC_LITERAL(5, 88, 26),
QT_MOC_LITERAL(6, 115, 17),
QT_MOC_LITERAL(7, 133, 15)
    },
    "stillPage\0on_nextStep_clicked\0\0"
    "on_clearList_clicked\0"
    "on_applyAllEffectsToOneFile_clicked\0"
    "on_applyAllEffects_clicked\0launchVideoPlayer\0"
    "importSlideshow\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_stillPage[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   44,    2, 0x08,
       3,    0,   45,    2, 0x08,
       4,    0,   46,    2, 0x08,
       5,    0,   47,    2, 0x08,
       6,    0,   48,    2, 0x08,
       7,    0,   49,    2, 0x08,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void stillPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        stillPage *_t = static_cast<stillPage *>(_o);
        switch (_id) {
        case 0: _t->on_nextStep_clicked(); break;
        case 1: _t->on_clearList_clicked(); break;
        case 2: _t->on_applyAllEffectsToOneFile_clicked(); break;
        case 3: _t->on_applyAllEffects_clicked(); break;
        case 4: _t->launchVideoPlayer(); break;
        case 5: _t->importSlideshow(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject stillPage::staticMetaObject = {
    { &common::staticMetaObject, qt_meta_stringdata_stillPage.data,
      qt_meta_data_stillPage,  qt_static_metacall, 0, 0}
};


const QMetaObject *stillPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *stillPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_stillPage.stringdata))
        return static_cast<void*>(const_cast< stillPage*>(this));
    return common::qt_metacast(_clname);
}

int stillPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = common::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}
struct qt_meta_stringdata_options_t {
    QByteArrayData data[11];
    char stringdata[128];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_options_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_options_t qt_meta_stringdata_options = {
    {
QT_MOC_LITERAL(0, 0, 7),
QT_MOC_LITERAL(1, 8, 12),
QT_MOC_LITERAL(2, 21, 0),
QT_MOC_LITERAL(3, 22, 10),
QT_MOC_LITERAL(4, 33, 19),
QT_MOC_LITERAL(5, 53, 12),
QT_MOC_LITERAL(6, 66, 10),
QT_MOC_LITERAL(7, 77, 16),
QT_MOC_LITERAL(8, 94, 7),
QT_MOC_LITERAL(9, 102, 8),
QT_MOC_LITERAL(10, 111, 15)
    },
    "options\0defaultClick\0\0registered\0"
    "refreshOptionFields\0closeOptions\0"
    "changePage\0QListWidgetItem*\0current\0"
    "previous\0clearOptionData\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_options[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   44,    2, 0x05,
       3,    0,   47,    2, 0x05,

 // slots: name, argc, parameters, tag, flags
       4,    0,   48,    2, 0x0a,
       5,    0,   49,    2, 0x08,
       6,    2,   50,    2, 0x08,
      10,    0,   55,    2, 0x08,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7, 0x80000000 | 7,    8,    9,
    QMetaType::Void,

       0        // eod
};

void options::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        options *_t = static_cast<options *>(_o);
        switch (_id) {
        case 0: _t->defaultClick((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->registered(); break;
        case 2: _t->refreshOptionFields(); break;
        case 3: _t->closeOptions(); break;
        case 4: _t->changePage((*reinterpret_cast< QListWidgetItem*(*)>(_a[1])),(*reinterpret_cast< QListWidgetItem*(*)>(_a[2]))); break;
        case 5: _t->clearOptionData(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (options::*_t)(bool );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&options::defaultClick)) {
                *result = 0;
            }
        }
        {
            typedef void (options::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&options::registered)) {
                *result = 1;
            }
        }
    }
}

const QMetaObject options::staticMetaObject = {
    { &common::staticMetaObject, qt_meta_stringdata_options.data,
      qt_meta_data_options,  qt_static_metacall, 0, 0}
};


const QMetaObject *options::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *options::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_options.stringdata))
        return static_cast<void*>(const_cast< options*>(this));
    return common::qt_metacast(_clname);
}

int options::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = common::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void options::defaultClick(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void options::registered()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
