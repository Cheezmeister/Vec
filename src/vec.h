#include "bml.h"

typedef struct _Input {
    bool quit;

    // Normalized (-1.0 <-> 1.0) axes
    struct _Axes {
        float x1;
        float x2;
        float x3;
        float y1;
        float y2;
        float y3;
    } axes;

    bool pause;

    bool shoot; // primary fire is held
    bool poop; // primary poop is held
    bool auxshoot; // aux fire was pressed
    bool auxpoop; // aux poop was pressed

} Input;

// Entity types
enum {
    E_FIRST = 1,
    E_BULLET = 1,
    E_ROCKET,
    E_TURD,
    E_NOVA,
    E_ENEMY,
    E_XPCHUNK,
    E_LAST,
};
typedef int EType;

const int MAX_ENEMIES = 12;
const int MAX_ENTITIES = 500;

typedef struct Entity {
    EType type;
    float life;
    bml::Vec pos;
    bml::Vec vel;
    float rotation;
    float hue;
} Entity;

typedef struct _GameState {

    Entity entities[MAX_ENTITIES];
    int next;

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
