#ifndef _NOTDEC_UTILS_H_
#define _NOTDEC_UTILS_H_

#include <cstdint>
#include <cstring>

float ieee_float(uint32_t f);
double ieee_double(uint64_t f);


enum log_level {
    level_emergent = 0,
    level_alert = 1,
    level_critical = 2,
    level_error = 3,
    level_warning = 4,
    level_notice = 5,
    level_info = 6,
    level_debug = 7
};

extern const char* MEM_NAME;

#endif
