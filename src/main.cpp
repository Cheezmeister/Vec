#include <iostream>
#include <ctime>
#include "crossgl.h"
#include "SDL.h"
#include "manymouse.h"
#include "bml.h"

////////////////////////////////////////////////////////////////////////////////
using namespace std;

typedef struct _Args {
    bool debug;
} Args;

typedef struct _Dimension2 {
    float x;
    float y;
} Dimension2;


// Ugly nasty globals
SDL_Window* win;
Args args;
Dimension2 viewport;
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
            if (event.key.keysym.sym == SDLK_SPACE) ret.shoot = true;
        }

        if (event.type == SDL_WINDOWEVENT)
        {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                int x = event.window.data1;
                int y = event.window.data2;
                viewport.x = x;
                viewport.y = y;
                int max = x > y ? x : y;

                glViewport(0, 0, max, max);
            }
        }

        if (event.type == SDL_CONTROLLERBUTTONDOWN)
        {
            if (event.cbutton.button & SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
            {
                ret.shoot = true;
            }
        }
    }

    // ManyMouse events
    ManyMouseEvent mme;
    while (ManyMouse_PollEvent(&mme))
    {
        // NOTE interestingly, it seems that having a fake mouse connected via Synergy
        //      will consume device slot 0, but won't produce events for ManyMouse.
        //      That's just a guess. And what happens if you have a gazillion mice!?
        const int leftmouse = 1;
        const int rightmouse = 2;

        if (mme.type == MANYMOUSE_EVENT_RELMOTION)
        {
            // TODO Determine 'left' vs. 'right' from device no.
            // NOTE 'item' indicates which axis, 'device' indicates which mouse
            float& axis = mme.item == 0 ?
                          (mme.device == leftmouse ? state.axes.x1 : state.axes.x2) :
                          (mme.device == leftmouse ? state.axes.y1 : state.axes.y2);
            float value = mme.value / (mme.item == 0 ? viewport.x : -viewport.y);
            axis += value;
        }
        else if (mme.type == MANYMOUSE_EVENT_BUTTON)
        {
            bool pressed = (mme.value == 1);
            if (mme.device == rightmouse)
            {
                if (mme.item == 0)
                    state.button.left2 = pressed;
                else if (mme.item == 1)
                    state.button.right2 = pressed;
            }
            else
            {
                if (mme.item == 0)
                    state.button.left1 = pressed;
                else if (mme.item == 1)
                    state.button.right1 = pressed;
            }
        }
        // TODO Do we need ABSMOTION for some platforms?
    }

    // Process MM state
    ret.axes.x1 += state.axes.x1;
    ret.axes.x2 += state.axes.x2;
    ret.axes.y1 += state.axes.y1;
    ret.axes.y2 += state.axes.y2;
    ret.shoot |= state.button.left2 || state.button.left1;
    ret.poop |= state.button.right2 || state.button.right1;

    // Poll keyboard
    const Uint8* keystate = SDL_GetKeyboardState(NULL);
    ret.axes.y1 += 1.0 * (keystate[SDL_SCANCODE_W]);
    ret.axes.y1 -= 1.0 * (keystate[SDL_SCANCODE_S]);
    ret.axes.x1 += 1.0 * (keystate[SDL_SCANCODE_D]);
    ret.axes.x1 -= 1.0 * (keystate[SDL_SCANCODE_A]);
    ret.poop |= (0 != keystate[SDL_SCANCODE_R]);

    // Poll sticks
    float deadzone = 0.15;
    ret.axes.x1 += get_axis(SDL_CONTROLLER_AXIS_LEFTX, deadzone);
    ret.axes.x2 += get_axis(SDL_CONTROLLER_AXIS_RIGHTX, deadzone);
    ret.axes.y1 -= get_axis(SDL_CONTROLLER_AXIS_LEFTY, deadzone);
    ret.axes.y2 -= get_axis(SDL_CONTROLLER_AXIS_RIGHTY, deadzone);
    ret.auxpoop |= (get_axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, deadzone) > 0.8);
    ret.poop    |= (get_axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, deadzone) > 0.8);

    return ret;
}

void loop()
{

    GameState state = {0};

    gfx::init();
    game::init(state);

    SDL_ShowCursor(SDL_DISABLE);

    while (!state.over)
    {
        // Timing
        u32 ticks = SDL_GetTicks();

        // Input
        Input input = handle_input();
        if (input.quit) break;

        // Process gameplay
        game::update(state, input);

        // Render graphics
        gfx::render(state, ticks, args.debug);

        // Commit
        SDL_GL_SwapWindow(win);

        // Finish frame
        SDL_Delay(20);
    }
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
    win = SDL_CreateWindow("Vec", 0, 0, viewport.x, viewport.y, flags);
    if (win == NULL)
    {
        cerr << "Couldn't set video mode";
        return 2;
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
