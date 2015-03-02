#include <cmath>
#include "bml.h"

namespace game {

struct _GameParams {
    float movespeed, rotspeed, drag ;
} params = {   0.005,      6, 0.9 };

void init()
{
}

void update(GameState& state, const Input& input)
{
    // Movement
    float thrust = params.movespeed * input.axes.y1;
    float sidethrust = params.movespeed * input.axes.x1;
    float r = state.player.rotation;

    // Aiming
    state.player.reticle.x = input.axes.x2;
    state.player.reticle.y = input.axes.y2;

    bml::Vec t = {
      thrust * cos(r) + sidethrust * sin(r),
      thrust * sin(r) - sidethrust * cos(r)
    };

    state.player.rotation = atan2(state.player.reticle.y - state.player.pos.y, state.player.reticle.x - state.player.pos.x);
    state.player.vel += t;

    state.player.pos += state.player.vel;
    state.player.vel = state.player.vel * params.drag;

}

}
