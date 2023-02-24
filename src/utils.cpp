
#include "utils.h"

const int level_emergent = 0;
const int level_alert = 1;

const int level_critical = 2;
const int level_error = 3;
const int level_warning = 4;
const int level_notice = 5;

const int level_info = 6;
const int level_debug = 7;

// https://stackoverflow.com/questions/48803363/bitwise-casting-uint32-t-to-float-in-c-c
float ieee_float(uint32_t f)
{
    static_assert(sizeof(float) == sizeof f, "`float` has a weird size.");
    float ret;
    std::memcpy(&ret, &f, sizeof(float));
    return ret;
}

double ieee_double(uint64_t f)
{
    static_assert(sizeof(double) == sizeof f, "`float` has a weird size.");
    double ret;
    std::memcpy(&ret, &f, sizeof(double));
    return ret;
}
