#include "bml.h"

typedef int EType;

// Entity types
enum {
    E_FIRST = 1,
    E_BULLET = 1,
    E_ROCKET,
    E_TURD,
    E_NOVA,
    E_ENEMY,
    E_XPCHUNK,
    E_TRIANGLE,
    E_LAST,
};

const int MAX_ENEMIES = 15;
const int MAX_ENTITIES = 500;
const int MAX_EVENTS = 20;

typedef struct _Entity {
    EType type;
    float life;
    bml::Vec pos;
    bml::Vec vel;
    float rotation;
    float hue;
} Entity;

typedef struct _Player : public Entity {
    float size;
    float phase; // norm
    bml::Vec reticle;
    float cooldown;
    int killcount;
    u32 lastkill;
    int combo;
} Player;

typedef struct _Event {
    enum Type {
        T_ENT_DESTROYED,
        T_ENT_CREATED,
        T_ENT_HIT,
        T_LAST
    } type;

    EType entity;
} Event;

typedef struct _GameState {

    u32 ticks; // Millis since start
    u32 dticks; // Millis since last frame

    // Enemies, bullets, and stuff
    Entity entities[MAX_ENTITIES];
    int next;

    // Events that happened this frame
    Event events[MAX_EVENTS];
    int next_event;

    // Player
    Player player;

    struct _Square {
        bml::Vec pos;
        float size;
        bool attract;
    } square;

    bool over; // game over?
} GameState;

typedef struct _Input {
    bool quit;

    // Normalized (-1.0 <-> 1.0) axes
    struct _Axes {
        float x1, x2, x3, x4;
        float y1, y2, y3, y4;
    } axes;

    bool pause;

    bool shoot; // primary fire is held
    bool poop; // primary poop is held
    bool auxshoot; // aux fire was pressed
    bool auxpoop; // aux poop was pressed

    bool attract; // square attract button is held
    bool respawn; // force respawn ents

} Input;

static float beats_per_minute(const GameState& state)
{
    return 120 - 2 * state.player.killcount * bml::minimum(state.player.size, 1);
}

namespace gfx
{
void init();
void render(GameState& state, u32 ticks, bool debug);
void set_viewport(int x, int y);
}

namespace game
{
void init(GameState& state);
void update(GameState& state, u32 ticks, bool debug, const Input& input);
}

namespace audio
{
void init(u32 ticks);
void update(const GameState& state, u32 ticks);
}
