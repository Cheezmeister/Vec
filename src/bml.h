#include <iostream>
#include <cstdint>
#include <string>

#ifndef NULL
#define NULL 0
#endif

#ifndef M_PI
#define M_PI 3.1415926536
#endif

#define DEBUGVAR(x) bml::debug << #x " is " << x << endl;
const char ESCAPE = '\e';

typedef uint32_t u32;
typedef int32_t i32;

namespace bml
{
static std::ostream& logger = std::cout;
typedef struct _Vec {
    float x;
    float y;

} Vec;

static Vec operator -(const Vec& lhs, const Vec& rhs)
{
    Vec ret = {lhs.x - rhs.x, lhs.y - rhs.y};
    return ret;
}
static Vec operator +(const Vec& lhs, const Vec& rhs)
{
    Vec ret = {lhs.x + rhs.x, lhs.y + rhs.y};
    return ret;
}
static void operator +=(Vec& lhs, const Vec& rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
}
static Vec operator *(float rhs, const Vec& lhs)
{
    Vec ret = {lhs.x * rhs, lhs.y * rhs};
    return ret;
}
static Vec operator *(const Vec& lhs, float rhs)
{
    Vec ret = {lhs.x * rhs, lhs.y * rhs};
    return ret;
}
}

