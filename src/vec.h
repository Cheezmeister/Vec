#include "bml.h"

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

    bool shoot; // primary fire is held
    bool poop; // primary poop is held
    bool auxshoot; // aux fire was pressed
    bool auxpoop; // aux poop was pressed

} Input;

const int MAX_BULLETS = 500;
typedef struct _Bullet {
    float life;
    bml::Vec pos;
    bml::Vec vel;
} Bullet;

const int MAX_TURDS = 50;
typedef struct _Turd {
    float rotation;
    float life;
    bml::Vec pos;
} Turd;

const int MAX_ENEMIES = 12;
typedef struct _Enemy {
    float life;
    bml::Vec pos;
    bml::Vec vel;
} Enemy;

typedef struct _GameState {

    Bullet bullets[MAX_BULLETS];
    int next_bullet;

    Turd turds[MAX_TURDS];
    int next_turd;

    Enemy enemies[MAX_ENEMIES];
    int next_enemy;

    struct _Player {
        bml::Vec pos;
        bml::Vec vel;
        float size;
        float rotation; // radians
        float phase; // norm
        bml::Vec reticle;
    } player;

    bool over; // game over?
} GameState;



namespace gfx
{
void init();
void render(GameState& state, u32 ticks, bool debug);
}

namespace game
{
void init(GameState& state);
void update(GameState& state, u32 ticks, bool debug, const Input& input);
}
