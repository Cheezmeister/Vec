#include <iostream>
#include <ctime>
#include "GL/glew.h"
#include "crossgl.h"
#include "SDL.h"
#include "vec.h"

#if USE_EMSCRIPTEN
#include "emscripten.h"
#endif

////////////////////////////////////////////////////////////////////////////////
using namespace std;

typedef struct _Args {
    bool debug;
    bool fullscreen;
    bool windowed;
    bool mute;
} Args;

// Commandline arguments
Args args;

// Window management
bool fullscreen = false;
SDL_Window* win;

// Gameplay
GameState state = {0};
SDL_GLContext context = {0};


// Forward
void _update();
int _setup();
void _cleanup();

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

int enter_fullscreen()
{
#if USE_EMSCRIPTEN
      cerr << "Fullscreen not possible on web" << endl;;
    return -1;
#else
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
#endif
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
    game::init(state);
    gfx::init();
    audio::init(SDL_GetTicks());
    input::init();

    SDL_ShowCursor(SDL_DISABLE);

    u32 FPS = 50;
#if __EMSCRIPTEN__
    emscripten_set_main_loop(_update, FPS, false);
    return;
#endif

    // TODO un-hard-code FPS and BPM
    while (!state.over)
    {

        u32 ticks = SDL_GetTicks();
        _update();

        // Finish frame
        u32 next = ticks + 1000 / FPS;
        u32 now = SDL_GetTicks();
        if (next > now)
            SDL_Delay(next - now);
    }
}

int _setup()
{
    // Fire up SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        cerr << "Couldn't init SDL";
        return 1;
    }

    // Open a window
    struct _Dimension2 {
        float x;
        float y;
    } viewport;
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
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    context = SDL_GL_CreateContext(win);
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
    return 0;
}

void _cleanup()
{
    input::cleanup();
    SDL_GL_DeleteContext(context);
    SDL_Quit();
}

void _update()
{
    // Timing
    u32 ticks = SDL_GetTicks();
    state.dticks = ticks - state.ticks;
    state.ticks = ticks;
    u32 before = ticks;
    u32 after;

    // Input
    Input input = input::handle_input();
    if (input.sys.quit) 
    {
        state.over = true;
        return;
    }
    if (input.sys.fullscreen)
    {
        if (enter_fullscreen())
            bml::logger << "Remaining in windowed mode\n";
    }

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

    RETURN_IF_NONZERO(_setup());

    print_info();

    loop();

    // Clean up and gtfo
    _cleanup();
    return 0;
}
