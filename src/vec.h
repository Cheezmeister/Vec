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
    E_LAST,
};

const int MAX_ENEMIES = 12;
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

typedef struct _Event {
	enum Type {
		T_ENT_DESTROYED,
		T_LAST
	} type;

	EType entity;
} Event;

typedef struct _GameState {

    Entity entities[MAX_ENTITIES];
    int next;

	Event events[MAX_EVENTS];
	int next_event;

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

namespace audio
{
	void init();
	void update(const GameState& state, u32 ticks);
}
