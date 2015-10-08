#include <iostream>
#include <cstdint>
#include <string>

#ifndef NULL
#define NULL 0
#endif

#ifndef M_PI
#define M_PI 3.1415926536
#endif

#define RETURN_IF_NONZERO(do_something) \
  { int _code = do_something; if (code) return code; }


const float π = M_PI;
const float ROOT_2   = 1.4142135623731;
const float ROOT_2_2 = 0.707106781186548;

#ifndef FORCE_DEBUG 
#ifdef DEBUG
const int FORCE_DEBUG = true;
#else
const int FORCE_DEBUG = false;
#endif
#endif

#define DEBUGVAR(x) bml::logger << #x " is " << x << std::endl;
const char ESCAPE = '\e';

typedef uint32_t u32;
typedef int32_t i32;

namespace bml
{
static std::ostream& logger = std::cerr;
typedef struct _Vec {
    float x;
    float y;
} Vec;

const Vec UNIT_X = { 1, 0 };
const Vec UNIT_Y = { 0, 1 };

// http://stackoverflow.com/q/563198
static float cross(const Vec& lhs, const Vec& rhs)
{
    return lhs.x * rhs.y - rhs.x * lhs.y;
}

static void negate(Vec& vec)
{
    vec.x = -vec.x;
    vec.y = -vec.y;
}

static Vec operator -(const Vec& unary)
{
    Vec ret = {-unary.x, -unary.y};
    return ret;
}
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
static void operator *=(Vec& lhs, float rhs)
{
    lhs.x *= rhs;
    lhs.y *= rhs;
}
static void operator -=(Vec& lhs, const Vec& rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
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

static std::ostream& operator<<(std::ostream& lhs, const Vec& rhs)
{
    return lhs << '[' << rhs.x << ',' << rhs.y << ']';
}

static void warn(const char* message)
{
    logger << "WARNING: " << message << std::endl;
}

// random float between -1 and 1
static float normrand()
{
    return (float)(rand() % 32767) * 2.0 / (float)32767 - 1.0;
}

static int maximum(int a, int b)
{
    return a > b ? a : b;
}

static int minimum(int a, int b)
{
    return a < b ? a : b;
}

} // namespace bml

