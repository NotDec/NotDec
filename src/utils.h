#ifndef _NOTDEC_UTILS_H_
#define _NOTDEC_UTILS_H_

#include <cstdint>
#include <cstring>

float ieee_float(uint32_t f);
double ieee_double(uint64_t f);

extern const int level_emergent;
extern const int level_alert;

extern const int level_critical;
extern const int level_error;
extern const int level_warning;
extern const int level_notice;

extern const int level_info;
extern const int level_debug;

#endif
