#include "SDL.h"
#include "manymouse.h"
#include "vec.h"

using namespace std;

namespace input {

#ifndef USE_EMSCRIPTEN
SDL_GameController* controller = NULL;
float get_axis(SDL_GameControllerAxis which, float deadzone)
{
    float normalized = SDL_GameControllerGetAxis(controller, which) / (float)32767;
    if (fabs(normalized) < deadzone) return 0;
    return normalized;
}

void init_game_controller()
{
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
    {
        if (SDL_IsGameController(i))
        {
            cout << "Detected game controller in slot " << i << endl;
            controller = SDL_GameControllerOpen(i);
            break;
        }
    }
}
#endif

void init()
{
    // Fire up ManyMouse
    if (ManyMouse_Init() < 0)
    {
        cerr << "Couldn't init ManyMouse! Mouse input may be busted.";
    }

#ifndef USE_EMSCRIPTEN
    // Use the gamepad, if one is connected. First one we find.
    init_game_controller();
#endif

}
void cleanup()
{
    ManyMouse_Quit();
#ifndef USE_EMSCRIPTEN
    SDL_GameControllerClose(controller); // safe even if controller == NULL
#endif
}


Input handle_input()
{
    Input ret = {0};

    // ManyMouse gives us events, but not current mouse state. Record that here.
    static struct InputState {
        struct _Axes {
            float x1;
            float x2;
            float y1;
            float y2;
        } axes;
        struct _Buttons {
            bool left1; // left-click (technically, primary click) on left mouse
            bool right1; // right-click, left mouse
            bool left2; // left-click, right mouse
            bool right2; // right-click, right mouse
        } button;
    } state;

    // SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event) )
    {
        if (event.type == SDL_QUIT) ret.sys.quit = true;
        if (event.type == SDL_KEYDOWN)
        {
            SDL_Keycode sym = event.key.keysym.sym;
            if (sym == SDLK_ESCAPE) ret.sys.quit = true;
            if (sym == SDLK_f     ) ret.sys.fullscreen = true;
            if (sym == SDLK_LSHIFT) ret.auxpoop = true;
            if (sym == SDLK_r     ) ret.auxpoop = true;
        }

#ifndef USE_EMSCRIPTEN
        if (event.type == SDL_CONTROLLERBUTTONDOWN)
        {
            if (event.cbutton.button & SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
            {
                ret.auxshoot = true;
            }
            if (event.cbutton.button & SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
            {
                ret.auxpoop = true;
            }
        }
#endif
    }

    // ManyMouse events
    ManyMouseEvent mme;
    while (ManyMouse_PollEvent(&mme))
    {
        // NOTE interestingly, it seems that having a fake mouse connected via Synergy
        //      will consume device slot 0, but won't produce events for ManyMouse.
        //      That's just a guess.
        //
        //      A simple hack around, assign devices in the order they were used.
        static int leftmouse = -1;
        static int rightmouse = -1;
        if (rightmouse < 0)
            rightmouse = mme.device;
        else if (leftmouse < 0 && mme.device != rightmouse)
            leftmouse = mme.device;

        if (mme.type == MANYMOUSE_EVENT_RELMOTION)
        {
            // NOTE 'item' indicates which axis, 'device' indicates which mouse
            bool xaxis = ( mme.item == 0 );
            bool yaxis = !xaxis;
            float& axis = xaxis ?
                          (mme.device == leftmouse ? ret.axes.x3 : state.axes.x2) :
                          (mme.device == leftmouse ? ret.axes.y3 : state.axes.y2);
            float value = mme.value / 400.0f; //(float)bml::maximum(viewport.x, viewport.y);
            if (yaxis) value = -value;
            axis += value;
        }
        else if (mme.type == MANYMOUSE_EVENT_BUTTON)
        {
            bool pressed = (mme.value == 1);
            if (mme.device == leftmouse)
            {
                if (mme.item == 0)
                    ret.auxshoot = pressed;
                else if (mme.item == 1)
                    ret.auxpoop = pressed;
            }
            else
            {
                if (mme.item == 0)
                    state.button.left1 = pressed;
                else if (mme.item == 1)
                    state.button.right1 = pressed;
            }
        }
    }

    // Process MM state
    ret.axes.x2 += state.axes.x2;
    ret.axes.y2 += state.axes.y2;
    ret.shoot |= state.button.left1;
    ret.poop |= state.button.right1;

    // Poll keyboard
    const Uint8* keystate = SDL_GetKeyboardState(NULL);
    ret.axes.y1 += 1.0 * (keystate[SDL_SCANCODE_W]);
    ret.axes.y1 -= 1.0 * (keystate[SDL_SCANCODE_S]);
    ret.axes.x1 += 1.0 * (keystate[SDL_SCANCODE_D]);
    ret.axes.x1 -= 1.0 * (keystate[SDL_SCANCODE_A]);
    ret.axes.y4 += 1.0 * (keystate[SDL_SCANCODE_UP]);
    ret.axes.y4 -= 1.0 * (keystate[SDL_SCANCODE_DOWN]);
    ret.axes.x4 += 1.0 * (keystate[SDL_SCANCODE_RIGHT]);
    ret.axes.x4 -= 1.0 * (keystate[SDL_SCANCODE_LEFT]);
    ret.shoot |= (0 != keystate[SDL_SCANCODE_E]);
    ret.poop |= (0 != keystate[SDL_SCANCODE_Q]);
    ret.attract |= (0 != keystate[SDL_SCANCODE_SPACE]);

#ifndef USE_EMSCRIPTEN
    // Poll gamepad
    float deadzone = 0.15;
    ret.axes.x1 += get_axis(SDL_CONTROLLER_AXIS_LEFTX, deadzone);
    ret.axes.x2 += get_axis(SDL_CONTROLLER_AXIS_RIGHTX, deadzone);
    ret.axes.y1 -= get_axis(SDL_CONTROLLER_AXIS_LEFTY, deadzone);
    ret.axes.y2 -= get_axis(SDL_CONTROLLER_AXIS_RIGHTY, deadzone);
    ret.shoot      |= (get_axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, deadzone) > 0.8);
    ret.poop       |= (get_axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, deadzone) > 0.8);
#endif
    return ret;
}

} // namespace input
