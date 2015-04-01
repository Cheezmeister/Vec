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


typedef struct _Input {
    bool quit;

    // Normalized (-1.0 <-> 1.0) axes
    struct _Axes {
        float x1;
        float x2;
        float y1;
        float y2;
    } axes;

    bool pause;
    bool shoot; // fire is held
    bool shot; // fire was pressed

} Input;


const int MAX_BULLETS = 500;
typedef struct _Bullet {
    float life;
    bml::Vec pos;
    bml::Vec vel;
} Bullet;

const int MAX_ENEMIES = 12;
typedef struct _Enemy {
    float life;
    bml::Vec pos;
    bml::Vec vel;
} Enemy;

typedef struct _GameState {

    Bullet bullets[MAX_BULLETS];
    int next_bullet;

    Enemy enemies[MAX_ENEMIES];
    int next_enemy;

    struct _Player {
        bml::Vec pos;
        bml::Vec vel;
        float rotation; // radians
        bml::Vec reticle;
    } player;
} GameState;

namespace gfx
{
void init();
void render(GameState& state, u32 ticks, bool debug);
}

namespace game
{
void init(GameState& state);
void update(GameState& state, const Input& input);
}
