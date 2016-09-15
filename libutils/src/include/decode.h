#ifndef DECODE_H
#define DECODE_H

#include "c_utils.h"


#ifndef CHECK_FIELD_
#define CHECK_FIELD_(X, fp, log) test_field(X##__, X, sizeof(X), #X, fp, log, true, true);
#endif
#ifndef CHECK_FIELD_NOWRITE_
#define CHECK_FIELD_NOWRITE_(X, fp, log) test_field(X##__, X, sizeof(X), #X, fp, log, false, true);
#endif
#ifndef RW_FIELD_
#define RW_FIELD_(X, fp, log) rw_field(X, sizeof(X), #X, fp, log);
#endif

#ifdef CHECK_FIELD
#undef CHECK_FIELD
#endif

#define CHECK_FIELD(X) { uint8_t X##__[sizeof(X)];\
                         memset(X##__, 0, sizeof(X)); \
                         CHECK_FIELD_(X, fp, aob_log) }

#define CHECK_FIELD_AT(X, Y) { fseek(fp, Y, SEEK_SET); CHECK_FIELD(X) };

#ifdef CHECK_FIELD_NOWRITE
#undef CHECK_FIELD_NOWRITE
#endif

#define CHECK_FIELD_NOWRITE(X) { uint8_t X##__[sizeof(X)];\
                                 memset(X##__, 0, sizeof(X)); \
                                 CHECK_FIELD_NOWRITE_(X, fp, aob_log) }


#ifndef RW_FIELD
#undef RW_FIELD
#endif

#ifdef CHECK_FIELD_AT
#undef CHECK_FIELD_AT
#endif

#define CHECK_FIELD_AT(X, Y) { fseek(fp, Y, SEEK_SET); RW_FIELD(X) };

#ifdef RW_FIELD
#undef RW_FIELD
#endif

#define RW_FIELD(X)  { uint8_t X##__[sizeof(X)];\
                       memset(X##__, 0, sizeof(X)); \
                       RW_FIELD_(X, fp, aob_log) }


#endif // DECODE_H
