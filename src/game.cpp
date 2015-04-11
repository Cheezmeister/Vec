#include <cmath>
#include "vec.h"

using namespace std;

namespace game {

struct _GameParams {
    float movespeed, rotspeed, drag, bulletspeed, enemyspeed, hitbox, frequency ;
} params = {   0.005,      6,  0.9,         0.03, 0.01,       0.005,  30.0 / 60.0 / 4.0 };

float mag_squared(const bml::Vec& vec)
{
    return vec.x * vec.x + vec.y * vec.y;
}

void init(GameState& state)
{
    state.player.size = 1;
    for (int i = 0; i < MAX_ENEMIES; ++i)
    {
        state.enemies[i].pos.x = (rand() % MAX_ENEMIES) / (float)MAX_ENEMIES;
        state.enemies[i].pos.y = (rand() % MAX_ENEMIES) / (float)MAX_ENEMIES;
        state.enemies[i].vel.x = (rand() % MAX_ENEMIES) / (float)MAX_ENEMIES;
        state.enemies[i].vel.y = (rand() % MAX_ENEMIES) / (float)MAX_ENEMIES;
        state.enemies[i].life = 1;
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
    float r = state.player.rotation;

    // Shooting
    if (input.shoot)
    {
        int i = state.next_bullet;
        ++state.next_bullet %= MAX_BULLETS;
        state.bullets[i].life = 1000;
        state.bullets[i].vel = state.player.reticle - state.player.pos;
        state.bullets[i].pos = state.player.pos;
    }

    // Pooping
    if (input.poop)
    {
        int i = state.next_turd;
        ++state.next_turd %= MAX_TURDS;
        state.turds[i].pos = state.player.pos;
        state.turds[i].rotation = state.player.rotation;
        state.turds[i].life = 1;
    }

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


    // Bullets
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        Bullet& b = state.bullets[i];
        b.life--;
        b.pos += b.vel * params.bulletspeed;
    }

    // Turds
    for (int i = 0; i < MAX_TURDS; ++i)
    {
        Turd& t = state.turds[i];
        t.life -= 0.001;
    }

    // Enemies
    for (int i = 0; i < MAX_ENEMIES; ++i)
    {
        Enemy& e = state.enemies[i];
        e.pos += e.vel * params.enemyspeed;
        if (e.pos.x < -1.0) e.pos.x += 2.0;
        if (e.pos.y < -1.0) e.pos.y += 2.0;
        if (e.pos.x > 1.0) e.pos.x -= 2.0;
        if (e.pos.y > 1.0) e.pos.y -= 2.0;
    }

    // Collisions
    for (int j = 0; j < MAX_ENEMIES; ++j)
    {
        Enemy& e = state.enemies[j];

        // skip dead ents
        if (e.life <= 0) continue;

        if (mag_squared(state.player.pos - e.pos) < params.hitbox)
        {
            if (state.player.size <= 1)
                state.player.size -= 0.1;
            else
                state.player.size *= 0.5;
        }

        for (int i = 0; i < MAX_BULLETS; ++i)
        {
            Bullet& b = state.bullets[i];

            // Skip dead bullets
            if (b.life <= 0) continue;

            if (mag_squared(b.pos - e.pos) < params.hitbox) // TODO track size/hitbox
            {
                e.life -= 0.1;
                b.life = 0;
            }
        }
        for (int i = 0; i < MAX_TURDS; ++i)
        {
            Turd& b = state.turds[i];

            // Skip dead turds
            if (b.life <= 0) continue;

            if (mag_squared(b.pos - e.pos) < params.hitbox) // TODO track size/hitbox
            {
                e.life -= 0.1;
                b.life = 0;
            }
        }
    }
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        Bullet& b = state.bullets[i];

        // Skip dead bullets
        if (b.life <= 0) continue;

        if (mag_squared(b.pos - state.player.pos) < params.hitbox && b.life < 800)
        {
            state.player.size *= 1.1;
            b.life = 0;
        }
    }

    // Game Over
    if (state.player.size <= 0)
        state.over = true;

}

}
