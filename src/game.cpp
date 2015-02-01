#include <cmath>
#include "bml.h"

namespace game {

struct _GameParams {
    float movespeed, rotspeed, drag ;
} params = {   0.01,      0.1, 0.9 };

void init()
{
}

void update(GameState& state, const Input& input)
{
    // Movement
    float thrust = params.movespeed * input.axes.y1;
    state.player.rotation -= params.rotspeed * input.axes.x1;
    state.player.vel.x += thrust * cos(state.player.rotation);
    state.player.vel.y += thrust * sin(state.player.rotation);

    state.player.pos += state.player.vel;
    state.player.vel.x *= params.drag;
    state.player.vel.y *= params.drag;

    // Aiming
    state.player.reticle.x = input.axes.x2;
    state.player.reticle.y = input.axes.y2;

}

}
