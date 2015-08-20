#include <iostream>
#include <ctime>
#include "GL/glew.h"
#include "crossgl.h"
#include "SDL.h"
#include "manymouse.h"
#include "vec.h"

////////////////////////////////////////////////////////////////////////////////
using namespace std;

typedef struct _Args {
    bool debug;
    bool fullscreen;
    bool windowed;
    bool mute;
} Args;

typedef struct _Dimension2 {
    float x;
    float y;
} Dimension2;

// Commandline arguments
Args args;

// Window management
bool fullscreen = false;
SDL_Window* win;
Dimension2 viewport;

// Input state
SDL_GameController* controller = NULL;

int parse_args(int argc, char** argv, Args* outArgs)
{
    for (int i = 0; i < argc; ++i)
    {
        char* arg = argv[i];
        char first = arg[0];
        if (first == '-')
        {
            if (arg[1] == 'd')
                outArgs->debug = true;
            if (arg[1] == 'f')
                outArgs->fullscreen = true;
            if (arg[1] == 'w')
                outArgs->windowed = true;
            if (arg[1] == 'm')
                outArgs->mute = true;
        }
    }

    return 0;
}

float get_axis(SDL_GameControllerAxis which, float deadzone)
{
    float normalized = SDL_GameControllerGetAxis(controller, which) / (float)32767;
    if (fabs(normalized) < deadzone) return 0;
    return normalized;
}

int enter_fullscreen()
{
    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
    {
        fprintf(stderr, "SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
        return 1;
    }
    if (SDL_SetWindowDisplayMode(win, &dm))
    {
        fprintf(stderr, "SDL_SetWindowDisplayMode failed: %s", SDL_GetError());
        return 2;
    }
    if (SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN))
    {
        cerr << "Error going fullscreen: " << SDL_GetError() << endl;;
        return 3;
    }
    SDL_GetCurrentDisplayMode(0, &dm);
    cout << "Entered fullscreen at " << dm.w << "x" << dm.h << "@" << dm.refresh_rate << "Hz\n";
    gfx::set_viewport(dm.w, dm.h);
    SDL_ShowCursor(SDL_DISABLE);
    return 0;
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
        if (event.type == SDL_QUIT) ret.quit = true;
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_ESCAPE) ret.quit = true;
            if (event.key.keysym.sym == SDLK_f)
            {
                if (enter_fullscreen()) continue;
            }
            if (event.key.keysym.sym == SDLK_SPACE) ret.auxshoot = true;
            if (event.key.keysym.sym == SDLK_LSHIFT) ret.auxpoop = true;
        }

        if (event.type == SDL_WINDOWEVENT)
        {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                int x = viewport.x = event.window.data1;
                int y = viewport.y = event.window.data2;
                gfx::set_viewport(x, y);

            }
        }

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
            float value = mme.value / (float)bml::maximum(viewport.x, viewport.y);
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

    // Poll gamepad
    float deadzone = 0.15;
    ret.axes.x1 += get_axis(SDL_CONTROLLER_AXIS_LEFTX, deadzone);
    ret.axes.x2 += get_axis(SDL_CONTROLLER_AXIS_RIGHTX, deadzone);
    ret.axes.y1 -= get_axis(SDL_CONTROLLER_AXIS_LEFTY, deadzone);
    ret.axes.y2 -= get_axis(SDL_CONTROLLER_AXIS_RIGHTY, deadzone);
    ret.shoot      |= (get_axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, deadzone) > 0.8);
    ret.poop       |= (get_axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, deadzone) > 0.8);
    return ret;
}

void print_info()
{
    SDL_version version;
    SDL_VERSION(&version);
    cout << "SDL version: " << (int)version.major << "." << (int)version.minor << (int)version.patch << endl;
    SDL_GetVersion(&version);
    cout << "runtime version: " << (int)version.major << "." << (int)version.minor << (int)version.patch << endl;
    printf("OpenGL vendor: '%s'\n" , glGetString(GL_VENDOR));
    printf("OpenGL renderer: '%s'\n" , glGetString(GL_RENDERER));
    printf("OpenGL version: '%s'\n" , glGetString(GL_VERSION));
    printf("GLSL version: '%s'\n" , glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("GLEW version: %s\n", glewGetString(GLEW_VERSION));
}

void loop()
{

    GameState state = {0};

    game::init(state);
    gfx::init();
    audio::init(SDL_GetTicks());

    SDL_ShowCursor(SDL_DISABLE);

    // TODO un-hard-code FPS and BPM
    while (!state.over)
    {
        // Timing
        u32 ticks = SDL_GetTicks();
        state.dticks = ticks - state.ticks;
        state.ticks = ticks;
        u32 before = ticks;
        u32 after;

        // Input
        Input input = handle_input();
        if (input.quit) break;

        // Zero out events. Sophisticated, I know.
        memset(&state.events, 0, sizeof(state.events));
        state.next_event = 0;

        // Process gameplay
        game::update(state, ticks, args.debug, input);
        after = SDL_GetTicks();
        /* cerr << "Gameplay took " << (after - before) << endl; */

        // Render graphics
        before = SDL_GetTicks();
        gfx::render(state, ticks, args.debug);
        after = SDL_GetTicks();
        /* cerr << "Render took " << (after - before) << endl; */

        // Update audio
        audio::update(state, ticks);

        // Commit
        SDL_GL_SwapWindow(win);

        // Finish frame
        u32 next = ticks + 20;
        u32 now = SDL_GetTicks();
        if (next > now)
            SDL_Delay(next - now);
    }
}

int scratch()
{
    return 0;
}

extern "C"
int main(int argc, char** argv)
{
    int code = parse_args(argc, argv, &args);
    if (code) return code;

    srand(time(NULL));

    // Fire up SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        cerr << "Couldn't init SDL";
        return 1;
    }

    // Open a window
    viewport.x = 400;
    viewport.y = 400;
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#ifdef DEBUG
    if (args.fullscreen)
#else
    if (!args.windowed)
#endif
    {
        SDL_DisplayMode dm;
        if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
        {
            fprintf(stderr, "SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
        }
        else
        {
            viewport.x = dm.w;
            viewport.y = dm.h;
            flags |= SDL_WINDOW_FULLSCREEN;
        }
    }
    win = SDL_CreateWindow("Vec", 0, 0, viewport.x, viewport.y, flags);
    if (win == NULL)
    {
        cerr << "Couldn't set video mode";
        return 2;
    }

    if (args.mute)
    {
        SDL_PauseAudio(1); // HACK TODO volume control
    }

    SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    // Massage OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GLContext context = SDL_GL_CreateContext(win);
    if (context == NULL)
    {
        cerr << "Couldn't get a gl context: " << SDL_GetError();
        return 3;
    }

    // Massage GL some more
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        return 4;
    }

    // Fire up ManyMouse
    if (ManyMouse_Init() < 0)
    {
        cerr << "Couldn't init ManyMouse! Mouse input may be busted.";
    }

    print_info();



    // Use the gamepad, if one is connected. First one we find.
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
    {
        if (SDL_IsGameController(i))
        {
            cout << "Detected game controller in slot " << i << endl;
            controller = SDL_GameControllerOpen(i);
            break;
        }
    }

    loop();

    // Clean up and gtfo
    ManyMouse_Quit();
    SDL_GameControllerClose(controller); // safe even if controller == NULL
    SDL_GL_DeleteContext(context);
    SDL_Quit();
    return 0;
}
