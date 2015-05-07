#include <cmath>
#include "vec.h"

using namespace std;

namespace game {

// TODO cleanup
struct _GameParams {
    float movespeed, mousemovespeed, rotspeed, drag, bulletspeed, enemyspeed, hitbox, frequency ;
} params = {   0.005,      20.0,             6,  0.9,         0.03, 0.01,       0.005,  30.0 / 60.0 / 4.0 };

void collide(GameState& state);
void add_entity(GameState& state, Entity& e);
void destroy_entity(GameState& state, Entity& e);

float mag_squared(const bml::Vec& vec)
{
    return vec.x * vec.x + vec.y * vec.y;
}

void init(GameState& state)
{
    state.player.size = 1;
    for (int i = 0; i < MAX_ENEMIES; ++i)
    {
        Entity e = {0};
        e.type = E_ENEMY;
        e.pos.x = (rand() % MAX_ENEMIES) / (float)MAX_ENEMIES;
        e.pos.y = (rand() % MAX_ENEMIES) / (float)MAX_ENEMIES;
        e.vel.x = (rand() % MAX_ENEMIES) / (float)MAX_ENEMIES;
        e.vel.y = (rand() % MAX_ENEMIES) / (float)MAX_ENEMIES;
        e.life = 3;
        e.hue = (rand() % 360) / 360.0;
        add_entity(state, e);
    }
}

void update(GameState& state, u32 ticks, bool debug, const Input& input)
{
    // Aiming
    state.player.reticle.x = input.axes.x2;
    state.player.reticle.y = input.axes.y2;

    // Movement
    float thrust = params.movespeed * input.axes.y1;
    float sidethrust = params.movespeed * input.axes.x1;
    if (thrust == 0 && sidethrust == 0) // If dual-mouse, move differently
    {
        // TODO pid control
        thrust = params.movespeed * params.mousemovespeed * input.axes.y3;
        sidethrust = params.movespeed * params.mousemovespeed * input.axes.x3;

    }

    // Shooting
    if (input.shoot)
    {
        Entity b = {0};
        b.life = 1.0;
        b.type = E_BULLET;
        b.vel = state.player.reticle - state.player.pos;
        b.pos = state.player.pos;
        add_entity(state, b);
    }
    if (input.auxshoot)
    {
        Entity b = {0};
        b.life = 1.0;
        b.type = E_ROCKET;
        b.vel = state.player.reticle - state.player.pos;
        b.pos = state.player.pos;
        add_entity(state, b);
    }

    // Pooping
    if (input.poop)
    {
        Entity t = {0};
        t.type = E_TURD;
        t.pos = state.player.pos;
        t.rotation = state.player.rotation;
        t.life = 1;
        add_entity(state, t);
    }
    if (input.auxpoop)
    {
        Entity t = {0};
        t.type = E_NOVA;
        t.pos = state.player.pos;
        t.rotation = state.player.rotation;
        t.life = 1;
        add_entity(state, t);
    }

    // Update player (TODO: implement real polar movement)
    float r = state.player.rotation;
    bml::Vec t = {
        thrust * cos(r) + sidethrust * sin(r),
        thrust * sin(r) - sidethrust * cos(r)
    };
    state.player.rotation = atan2(state.player.reticle.y - state.player.pos.y, state.player.reticle.x - state.player.pos.x);
    state.player.vel += t;
    state.player.pos += state.player.vel;
    state.player.vel = state.player.vel * params.drag;
    float phase = ticks / 1000.0 * params.frequency;
    phase -= floor(phase);
    state.player.phase = phase;

    // Process entities
    for (int i = 0; i < MAX_ENTITIES; ++i)
    {
        Entity& e = state.entities[i];
        switch (e.type)
        {
        // Bullets/Rockets
        case E_BULLET:
        case E_ROCKET:
            e.life -= 0.01;
            e.pos += e.vel * params.bulletspeed;
            continue;

        // Turds & Novae
        case E_TURD:
        case E_NOVA:
            e.life -= 0.01;
            continue;

        // Enemies/XP Chunks
        case E_ENEMY:
            e.hue += 0.01;
        case E_XPCHUNK:
            e.pos += e.vel * params.enemyspeed;
            if (e.pos.x < -1.0) e.pos.x += 2.0;
            if (e.pos.y < -1.0) e.pos.y += 2.0;
            if (e.pos.x > 1.0) e.pos.x -= 2.0;
            if (e.pos.y > 1.0) e.pos.y -= 2.0;
            continue;

        default:
            ;
        }
    }

    // Collisions
    collide(state);

    // Game Over
    if (state.player.size <= 0)
        state.over = true;

}

bool check_collision(Entity& a, Entity& b)
{
    // skip dead ents
    if (a.life <= 0 || b.life <= 0) return false;

    // TODO variable hitbxes
    if (mag_squared(a.pos - b.pos) < params.hitbox)
    {
        return true;
    }
    return false;
}

void add_entity(GameState& state, Entity& e)
{
    // find next fungible (not-enemy) ent
    do ++state.next %= MAX_ENTITIES;
    while (state.entities[state.next].type == E_ENEMY);
    state.entities[state.next] = e;
}

void destroy_entity(GameState& state, Entity& e)
{
    e.life = 0;

    // Propogate event for gfx/audio
    Event& evt = state.events[state.next_event++];
    evt.type = Event::T_ENT_DESTROYED;
    evt.entity = e.type;

    if (e.type == E_ENEMY)
    {
        for (int i = 0; i < 4; ++i)
        {
            Entity xp = {0};
            xp.type = E_XPCHUNK;
            xp.pos = e.pos;
            xp.vel.x = e.vel.x + bml::normrand() * 0.5;
            xp.vel.y = e.vel.y + bml::normrand() * 0.5;
            xp.life = 1;
            xp.hue = e.hue;
            add_entity(state, xp);
        }
    }
}

void collide(GameState& state)
{
    // Check ent-ent collisions
    for (int i =   0; i < MAX_ENTITIES; ++i)
        for (int j = i+1; j < MAX_ENTITIES; ++j)
        {
            Entity& a = state.entities[i],
                    b = state.entities[j];
            if (check_collision(a, b))
            {
// XXX Lord this is hackish. Hao fix?
#define WHEN_COLLIDE(type1, type2) if ((a.type == type1 && b.type == type2 )|| (a.type == type2 && b.type == type1))
#define THEN_THE(type1) (a.type == type1 ? a : b)
                WHEN_COLLIDE(E_TURD, E_ENEMY)
                {
                    THEN_THE(E_TURD).life = 0;
                    Entity& enemy = THEN_THE(E_ENEMY);
                    enemy.life -= 0.1;
                    if (enemy.life <= 0)
                    {
                        destroy_entity(state, enemy);
                    }
                }
                WHEN_COLLIDE(E_BULLET, E_ENEMY)
                {
                    THEN_THE(E_BULLET).life = 0;
                    Entity& enemy = THEN_THE(E_ENEMY);
                    enemy.life -= 0.1;
                    if (enemy.life <= 0)
                    {
                        destroy_entity(state, enemy);
                    }
                }
#undef WHEN_COLLIDE
#undef THEN_THE
            }
        }

    // Handle player collisions
    for (int i = 0; i < MAX_ENTITIES; ++i)
    {
        Entity& e = state.entities[i];

        // skip dead ents
        if (e.life <= 0) continue;


        if (mag_squared(e.pos - state.player.pos) < params.hitbox)
        {
            // Collect bullets to grow (TODO this is a stupid mechanic)
            if (e.type == E_XPCHUNK)
            {
                state.player.size *= 1.1;
                destroy_entity(state, e);
            }

            // Enemies cause shrinkage
            else if (e.type == E_ENEMY)
            {
                if (state.player.size <= 1)
                    state.player.size -= 0.1;
                else
                    state.player.size *= 0.5;
            }
        }
    }
}

}
