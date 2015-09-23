#include <cmath>
#include "vec.h"

using namespace std;

namespace game {

// TODO cleanup this initialization
struct _GameParams {
    float movespeed,  squarespeed, mousemovespeed, rotspeed, drag, bulletdrag, bulletspeed, enemyspeed, hitbox, squaregrowth, squaregravity, squaredecay;
} params = {   0.005,        0.01,       20.0,            6,  0.9,      0.97,          0.03, 0.01,       0.005, 1.01,        0.02, 0.995 };

void collide(GameState& state, const GameState& previousState);
void add_entity(GameState& state, Entity& e);
void attract_entity(GameState& state, Entity& e);
void destroy_entity(GameState& state, Entity& e);
void spawn_enemies(GameState& state);

float mag_squared(const bml::Vec& vec)
{
    return vec.x * vec.x + vec.y * vec.y;
}

int entity_count(const GameState& state)
{
    int ents = 0;
    for (int i = 0; i < MAX_ENTITIES; ++i)
        if (state.entities[i].life > 0)
            ++ents;
    return ents;
}

void init(GameState& state)
{
    state.player.size = 1;
    state.player.type = E_TRIANGLE;
    state.square.size = 1;
    spawn_enemies(state);
}

void update(GameState& state, u32 ticks, bool debug, const Input& input)
{
    GameState previousState = state;

    // HACK TODO
    if (input.respawn && entity_count(state) == 0)
        spawn_enemies(state);

    // Aiming
    state.player.reticle.x = input.axes.x2;
    state.player.reticle.y = input.axes.y2;

    // Movement
    float thrust = params.movespeed * input.axes.y1;
    float sidethrust = params.movespeed * input.axes.x1;
    if (thrust == 0 && sidethrust == 0) // If dual-mouse, move differently
    {
        thrust = params.movespeed * params.mousemovespeed * input.axes.y3;
        sidethrust = params.movespeed * params.mousemovespeed * input.axes.x3;
    }

    // Move square
    state.square.pos.x += params.squarespeed * input.axes.x4;
    state.square.pos.y += params.squarespeed * input.axes.y4;

    // Shooting
    state.player.cooldown -= (float)state.dticks;
    if (input.shoot && state.player.cooldown <= 0)
    {
        Entity b = {0};
        b.life = 1.0;
        b.type = E_BULLET;
        b.vel = state.player.reticle - state.player.pos;
        b.pos = state.player.pos;
        add_entity(state, b);
        state.player.cooldown += 60000.0 / (4 * beats_per_minute(state));
    }
    if (input.auxshoot)
    {
        Entity b = {0};
        b.life = 1.0;
        b.type = E_ROCKET;
        b.vel = state.player.reticle - state.player.pos;
        b.pos = state.player.pos;
        b.rotation = state.player.rotation;
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
    state.square.attract = input.attract;

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

    float phase = ticks / 1000.0 * beats_per_minute(state) / pow(2, 10);
    phase -= floor(phase); // keep phase between 0-1
    state.player.phase = phase;

    // Update square
    if (state.square.size > 1)
    {
        state.square.size *= params.squaredecay;
    }

    // Process entities
    for (int i = 0; i < MAX_ENTITIES; ++i)
    {
        Entity& e = state.entities[i];
        if (e.life <= 0) continue;

        switch (e.type)
        {
        // Bullets/Rockets
        case E_BULLET:
            attract_entity(state, e);
        case E_ROCKET:
            e.life -= 0.002;
            if (e.life < 0.95)
                e.vel *= params.bulletdrag;
            e.pos += e.vel * params.bulletspeed;
            if (e.pos.x > 1 || e.pos.x < -1 || e.pos.y > 1 || e.pos.y < -1)
                destroy_entity(state, e);
            continue;

        // Turds & Novae
        case E_TURD:
        case E_NOVA:
            e.life -= 0.01;
            continue;

        // Enemies/XP Chunks
        case E_ENEMY:
            /* e.hue += 0.01; */
        case E_XPCHUNK:
            // Move
            e.pos += e.vel * params.enemyspeed;

            // Wrap
            if (e.pos.x < -1.0) e.pos.x += 2.0;
            if (e.pos.y < -1.0) e.pos.y += 2.0;
            if (e.pos.x > 1.0) e.pos.x -= 2.0;
            if (e.pos.y > 1.0) e.pos.y -= 2.0;

            // Attract
            attract_entity(state, e);
            continue;

        default:
            ;
        }
    }

    // Collisions
    collide(state, previousState);

    // Game Over
    if (state.player.size <= 0)
        state.over = true;

}

// (Re)spawn enemies
void spawn_enemies(GameState& state)
{
    int count = min(state.player.killcount + 1, MAX_ENEMIES);
    for (int i = 0; i < count; ++i)
    {
        Entity e = {0};
        e.type = E_ENEMY;
        float mag = bml::normrand() / 4.0 + 0.75;
        float angle = bml::normrand() * M_PI * 2;
        e.pos.x = mag * cos(angle);
        e.pos.y = mag * sin(angle);
        e.vel = e.pos * 0.5;
        e.life = 1.0;
        e.hue = (rand() % 360) / 360.0;
        add_entity(state, e);
    }
}


bool check_square_collision(GameState::_Square& square, Entity& e)
{
    bml::Vec offset = e.pos - square.pos;
    float sz = square.size * 0.2;
    if (mag_squared(offset) < sz)
    if (abs(offset.x) < sz / sqrt(2))
    if (abs(offset.y) < sz / sqrt(2))
    return true;

    return false;
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

void record_event(GameState& state, Event& evt)
{
    int i = state.next_event++;
    if (i >= MAX_EVENTS) return; // TODO

    state.events[i] = evt;
}

void add_entity(GameState& state, Entity& e)
{
    // find next fungible (not-enemy) ent
    do ++state.next %= MAX_ENTITIES;
    while (state.entities[state.next].type == E_ENEMY);
    state.entities[state.next] = e;

    // Propogate event for gfx/audio
    Event evt;
    evt.type = Event::T_ENT_CREATED;
    evt.entity = e.type;
    record_event(state, evt);

}

void destroy_entity(GameState& state, Entity& e)
{
    if (e.life == 0) bml::warn("Killing dead ent\n");

    e.life = 0;

    // Propogate event for gfx/audio
    Event evt;
    evt.type = Event::T_ENT_DESTROYED;
    evt.entity = e.type;
    record_event(state, evt);

    if (e.type == E_ENEMY)
    {
        ++state.player.killcount;
        if (state.ticks - state.player.lastkill < beats_per_minute(state))
            ++state.player.combo;
        else
            state.player.combo = 1;
        state.player.lastkill = state.ticks;

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

    if (entity_count(state) == 0)
    {
        spawn_enemies(state);
        state.player.killcount = 0;
        state.player.size = 1;
    }
}

void hurt_entity(GameState& state, Entity& e, float damage)
{
    e.life -= damage;
    if (e.life <= 0)
    {
        destroy_entity(state, e);
    }
}

void collide(GameState& state, const GameState& previousState)
{
    // Check ent-ent collisions
    for (int i =   0; i < MAX_ENTITIES; ++i)
        for (int j = i+1; j < MAX_ENTITIES; ++j)
        {
            Entity& a = state.entities[i];
            Entity& b = state.entities[j];
            if (check_collision(a, b))
            {
// XXX Lord this is hackish. Hao fix?
#define WHEN_COLLIDE(type1, type2) if ((a.type == type1 && b.type == type2 )|| (a.type == type2 && b.type == type1))
#define THEN_THE(type1) (a.type == type1 ? a : b)
                WHEN_COLLIDE(E_TURD, E_ENEMY)
                {
                    destroy_entity(state, THEN_THE(E_TURD));
                    hurt_entity(state, THEN_THE(E_ENEMY), 0.1);
                    Event evt;
                    evt.type = Event::T_ENT_HIT;
                    evt.entity = E_ENEMY;
                    record_event(state, evt);
                }
                WHEN_COLLIDE(E_BULLET, E_ENEMY)
                {
                    destroy_entity(state, THEN_THE(E_BULLET));
                    hurt_entity(state, THEN_THE(E_ENEMY), 0.1);
                    Event evt;
                    evt.type = Event::T_ENT_HIT;
                    evt.entity = E_ENEMY;
                    record_event(state, evt);
                }
                WHEN_COLLIDE(E_ROCKET, E_ENEMY)
                {
                    destroy_entity(state, THEN_THE(E_ROCKET));
                    hurt_entity(state, THEN_THE(E_ENEMY), 0.5);
                    Event evt;
                    evt.type = Event::T_ENT_HIT;
                    evt.entity = E_ENEMY;
                    record_event(state, evt);
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

        // collide ents with square TODO refactor and fix 0.2 scale wackness
        if (check_square_collision(state.square, e))
        {
            if (e.type == E_BULLET) // bullets are absorbed
            {
                state.square.size *= params.squaregrowth;
                destroy_entity(state, e);
            }
            if (e.type == E_ROCKET) // rockets bounce
            {
                e.vel.x = -e.vel.x;
                e.vel.y = -e.vel.y;
            }
            if (e.type == E_TURD) // turds block square
            {
                state.square.pos = previousState.square.pos;
            }
            if (e.type == E_ENEMY) // enemies block square
            {
                e.vel = -e.vel;
                bml::negate(e.vel);
            }

        }

        if (mag_squared(e.pos - state.player.pos) < params.hitbox)
        {
            // Collect xp to grow
            if (e.type == E_XPCHUNK)
            {
                state.player.size *= 1.05;
                destroy_entity(state, e);
            }

            // Enemies cause shrinkage
            else if (e.type == E_ENEMY)
            {
                if (state.player.size <= 1)
                    ;//state.player.size -= 0.1;
                else
                    state.player.size *= 0.5;
            }
        }
    }
}

void attract_entity(GameState& state, Entity& e)
{
    if (state.square.attract)
    {
        bml::Vec offset = e.pos - state.square.pos;
        e.pos -= offset * params.squaregravity;
        e.vel -= 0.01 * offset * params.squaregravity;
    }
}

}
