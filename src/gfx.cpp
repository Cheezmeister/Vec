#include <cmath>
#include "GL/glew.h"
#include "crossgl.h"
#include "vec.h"

// http://stackoverflow.com/a/13874526
#define MAKE_SHADER(version, shader)  "#version " #version "\n" #shader

#define GLSL_VERSION "#version 120\n"

using namespace std;
using namespace bml;

// Adapted from arsynthesis.org/gltut
namespace arcsynthesis {

GLuint CreateShader(GLenum eShaderType, const char* strFileData)
{
    GLuint shader = glCreateShader(eShaderType);
    glShaderSource(shader, 1, &strFileData, NULL);

    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

        const char *strShaderType = eShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment";
        fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
        delete[] strInfoLog;
    }

    return shader;
}


GLuint CreateProgram(GLuint vertex, GLuint fragment)
{
    GLuint program = glCreateProgram();

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    glLinkProgram(program);

    GLint status;
    glGetProgramiv (program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
        fprintf(stderr, "Linker failure: %s\n", strInfoLog);
        delete[] strInfoLog;
    }

    glDetachShader(program, vertex);
    glDetachShader(program, fragment);

    return program;
}

}

namespace gfx
{

typedef union Vertex {
    float a[4];
    struct {
        float x;
        float y;
        float z;
        float w;
    };
} Vertex;

template<int N>
union VertexBuffer {
    float flat[4 * N];
    Vertex v[N];
};

typedef struct _VBO {
    GLuint handle;
    GLsizei size;
} VBO;

typedef struct _FBO {
    GLuint handle;
    GLuint texture;
    float width;
    float height;
} FBO;

typedef struct _RenderState {
    struct _Shaders {
        GLuint player;
        GLuint square;
        GLuint reticle;
        GLuint meter;

        GLuint enemy;
        GLuint turd;
        GLuint viewport;
        GLuint nova;
        GLuint xpchunk;

        GLuint post_blur;
        GLuint post_fade;
    } shaders;

    // VBO for each major ent type
    struct _VBOs {
        VBO enemy;
        VBO player;
        VBO square;
        VBO nova;
        VBO reticle;
        VBO viewport;
    } vbo;

    // Two framebuffers for two-pass glow
    struct _FBOs {
        FBO a;
        FBO b;
    } fbo;
} RenderState;

typedef struct _RenderParams {
    struct _Bullet {
        float rotspeed;
    } bullet;
    struct _Enemy {
        float rotspeed;
    } enemy;
} RenderParams;

typedef struct _RenderArgs {
    const RenderParams& params;
    const GameState& gs;
    const RenderState& rs;
    u32 ticks;
    bool debug;
} RenderArgs;

typedef const RenderState& RS;
typedef const GameState& GS;

RenderState _renderstate;
RenderParams _params = {
    { 0.01f },
    { 0.0025f }
};

// Check for GL errors
void check_error(const string& message, bool debug=FORCE_DEBUG)
{
    GLenum error = glGetError();
    if (error && debug)
    {
        logger << "Error " << message.c_str() << ": [" << error << "] ";
        switch (error)
        {
        case 1282:
            logger << "invalid operation\n";
            return;
        default:
            logger << "unknown error\n";
            return;
        }
    }
}

// Set a uniform shader param
void set_uniform(GLuint shader, const string& name, float f)
{
    GLuint loc = glGetUniformLocation(shader, name.c_str());
    glUniform1f(loc, f);
}
void set_uniform(GLuint shader, const string& name, float x, float y)
{
    GLuint loc = glGetUniformLocation(shader, name.c_str());
    glUniform2f(loc, x, y);
}
void set_uniform(GLuint shader, const string& name, const Vec& v)
{
    set_uniform(shader, name, v.x, v.y);
}

FBO make_fbo(int width, int height)
{
    GLuint fbo;
    GLuint texture;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    check_error("teximage");
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    check_error("framebuftex");
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (GL_FRAMEBUFFER_COMPLETE != status)
        cerr << "ERROR Framebuffer is incomplete. Status was " << status << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    FBO ret;
    ret.handle = fbo;
    ret.texture = texture;
    ret.width = width;
    ret.height = height;
    return ret;

}

void set_viewport(int x, int y)
{
    int maxdim = x > y ? x : y;
    int xoffset = min( (x - y) / 2, 0);
    int yoffset = min(-(x - y) / 2, 0);
    cerr << "Setting viewport to " << xoffset << ',' << yoffset << ' ' << maxdim << ',' << maxdim << endl;
    glViewport(xoffset, yoffset, maxdim, maxdim);
    _renderstate.fbo.a = make_fbo(maxdim, maxdim);
    _renderstate.fbo.b = make_fbo(maxdim, maxdim);
}

float* make_polygon_vertex_array(int sides, float innerradius, float outerradius)
{
    // 3x4 coordinates per triangle
    float* vertices = new float[sides * 24];
    for (int i = 0; i < sides; ++i)
    {

        int index = i * 24;
        float angle = i * 2 * M_PI / sides;
        vertices[index  ] = outerradius * cos(angle);
        vertices[index+1] = outerradius * sin(angle);
        vertices[index+2] = 0;
        vertices[index+3] = 1;

        angle = (i+1) * 2 * M_PI / sides;
        vertices[index+4] = outerradius * cos(angle);
        vertices[index+5] = outerradius * sin(angle);
        vertices[index+6] = 0;
        vertices[index+7] = 1;

        vertices[index+8]  = innerradius * cos(angle);
        vertices[index+9]  = innerradius * sin(angle);
        vertices[index+10] = 0;
        vertices[index+11] = 1;

        vertices[index+12] = innerradius * cos(angle);
        vertices[index+13] = innerradius * sin(angle);
        vertices[index+14] = 0;
        vertices[index+15] = 1;

        angle = i * 2 * M_PI / sides;
        vertices[index+16] = innerradius * cos(angle);
        vertices[index+17] = innerradius * sin(angle);
        vertices[index+18] = 0;
        vertices[index+19] = 1;

        vertices[index+20]  = outerradius * cos(angle);
        vertices[index+21]  = outerradius * sin(angle);
        vertices[index+22] = 0;
        vertices[index+23] = 1;
    }
    return vertices;
}

// Prepare a triangle array for an N-gon
float* make_polygon_vertex_array(int sides, float radius)
{
    // 3x4 coordinates per triangle
    float* vertices = new float[sides * 12];
    for (int i = 0; i < sides; ++i)
    {

        int index = i * 12;
        float angle = i * 2 * M_PI / sides;
        vertices[index  ] = radius * cos(angle);
        vertices[index+1] = radius * sin(angle);
        vertices[index+2] = 0;
        vertices[index+3] = 1;

        angle = (i+1) * 2 * M_PI / sides;
        vertices[index+4] = radius * cos(angle);
        vertices[index+5] = radius * sin(angle);
        vertices[index+6] = 0;
        vertices[index+7] = 1;

        vertices[index+8] = 0;
        vertices[index+9] = 0;
        vertices[index+10] = 0;
        vertices[index+11] = 1;
    }
    return vertices;
}

GLuint make_vbo(size_t size, float* vertices)
{
    GLuint ret;
    glGenBuffers(1, &ret);
    glBindBuffer(GL_ARRAY_BUFFER, ret);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    check_error("buffering");
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return ret;
}

VBO make_polygon_vbo(int sides, float inner, float radius)
{
    VBO ret;
    int vcount = 6 * sides;
    size_t memsize = vcount * 4 * sizeof(float);
    float* vertices = make_polygon_vertex_array(sides, inner, radius);
    ret.handle = make_vbo(memsize, vertices);
    delete[] vertices;
    ret.size = vcount;
    return ret;
}

void draw_array(VBO vbo, GLenum type = GL_TRIANGLES)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo.handle);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(type, 0, vbo.size);
    glDisableVertexAttribArray(0);
}

GLuint make_shader(GLuint vertex, GLuint fragment)
{
    return arcsynthesis::CreateProgram(vertex, fragment);
}

void use_framebuffer(const FBO& fbo)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo.handle);
    check_error("binding fbo");
}

void init()
{
    RenderState& renderstate = _renderstate;

    // Compile shaders
    GLuint vs_noop = arcsynthesis::CreateShader
                     (GL_VERTEX_SHADER, GLSL_VERSION
#include "noop.vs"
                     );
    GLuint vs_stretch = arcsynthesis::CreateShader
                      (GL_VERTEX_SHADER, GLSL_VERSION
#include "stretch.vs"
                      );

    GLuint vs_pulse = arcsynthesis::CreateShader
                      (GL_VERTEX_SHADER, GLSL_VERSION
#include "pulse.vs"
                      );
    GLuint vs_wiggle = arcsynthesis::CreateShader
                       (GL_VERTEX_SHADER, GLSL_VERSION
#include "wiggle.vs"
                       );
    GLuint fs_scintillate = arcsynthesis::CreateShader
                            (GL_FRAGMENT_SHADER, GLSL_VERSION
#include "scintillate.fs"
                            );
    GLuint fs_pulse = arcsynthesis::CreateShader
                      (GL_FRAGMENT_SHADER, GLSL_VERSION
#include "pulse.fs"
                      );
    GLuint fs_circle = arcsynthesis::CreateShader
                       (GL_FRAGMENT_SHADER, GLSL_VERSION
#include "circle.fs"
                       );
    GLuint fs_glow = arcsynthesis::CreateShader
                     (GL_FRAGMENT_SHADER, GLSL_VERSION
#include "glow.fs"
                     );
    GLuint fs_fade = arcsynthesis::CreateShader
                     (GL_FRAGMENT_SHADER, GLSL_VERSION
#include "fade.fs"
                     );
    GLuint fs_swirl = arcsynthesis::CreateShader
                      (GL_FRAGMENT_SHADER, GLSL_VERSION
#include "swirl.fs"
                      );
    GLuint fs_meter = arcsynthesis::CreateShader
                      (GL_FRAGMENT_SHADER, GLSL_VERSION
#include "meter.fs"
                      );



    // Set up VBO
    renderstate.vbo.player = make_polygon_vbo(3, 0.0, 0.5);
    renderstate.vbo.square = make_polygon_vbo(4, 0.0, 0.5 * ROOT_2);
    renderstate.vbo.nova = make_polygon_vbo(3, 0.48, 0.5);
    renderstate.vbo.reticle = make_polygon_vbo(5, 0.09, 0.12);
    renderstate.vbo.enemy = make_polygon_vbo(6, 0.03, 0.09);

    VertexBuffer<4> viewportVertices = {
        -1, 1, 0, 1,
        -1, -1, 0, 1,
        1, -1, 0, 1,
        1,  1, 0, 1,
    };
    renderstate.vbo.viewport.handle = make_vbo(sizeof(viewportVertices), viewportVertices.flat);
    renderstate.vbo.viewport.size = 4;

    // Init shaders
    renderstate.shaders.player = make_shader(vs_pulse, fs_scintillate);
    renderstate.shaders.square = make_shader(vs_pulse, fs_scintillate);
    renderstate.shaders.reticle = make_shader(vs_pulse, fs_scintillate);
    renderstate.shaders.meter = make_shader(vs_stretch, fs_meter);

    renderstate.shaders.enemy = make_shader(vs_wiggle, fs_pulse);
    renderstate.shaders.turd = make_shader(vs_pulse, fs_scintillate);
    renderstate.shaders.nova = make_shader(vs_pulse, fs_circle);
    renderstate.shaders.xpchunk = make_shader(vs_pulse, fs_pulse);
    renderstate.shaders.post_blur = make_shader(vs_noop, fs_glow);
    renderstate.shaders.viewport = make_shader(vs_noop, fs_swirl);

    // Framebuffers
    renderstate.fbo.a = make_fbo(400, 400);
    renderstate.fbo.b = make_fbo(400, 400);

    // Misc setup
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    check_error("clearcolor");

}

void draw_background(const RenderArgs& args)
{
    GLuint shader = args.rs.shaders.viewport;
    VBO vbo = args.rs.vbo.viewport;
    glUseProgram(shader);
    set_uniform(shader, "ticks", args.ticks);
    draw_array(vbo, GL_QUADS);
}

void draw_triangle(const RenderArgs& args)
{
    RS renderstate = args.rs;
    GS state = args.gs;

    GLuint shader = renderstate.shaders.player;
    glUseProgram(shader);
    set_uniform(shader, "offset", state.player.pos);
    set_uniform(shader, "rotation", state.player.rotation);
    set_uniform(shader, "ticks", args.ticks);
    set_uniform(shader, "phase", state.player.phase);
    set_uniform(shader, "value", 1);
    float checkpoint = fabs(state.player.phase - 0.5) * 12;
    if (checkpoint < 1.0)
        set_uniform(shader, "value", checkpoint);
    set_uniform(shader, "scale", state.player.size);
    draw_array(renderstate.vbo.player);

}

void draw_square(const RenderArgs& args)
{
    RS renderstate = args.rs;
    GS state = args.gs;

    GLuint shader = renderstate.shaders.square;
    glUseProgram(shader);
    set_uniform(shader, "offset", state.square.pos);
    set_uniform(shader, "rotation", π / 4);
    set_uniform(shader, "ticks", args.ticks);
    set_uniform(shader, "phase", 0.5 + state.player.phase);
    set_uniform(shader, "scale", state.square.size);
    draw_array(renderstate.vbo.square);
}

void draw_reticle(const RenderArgs& args)
{
    RS renderstate = args.rs;
    GS state = args.gs;
    u32 ticks = args.ticks;

    GLuint shader = renderstate.shaders.reticle;
    glUseProgram(shader);
    set_uniform(shader, "offset", state.player.reticle);
    set_uniform(shader, "rotation", ticks / 1000.0f);
    set_uniform(shader, "phase", 0.02 + state.player.phase);
    set_uniform(shader, "ticks", ticks);
    set_uniform(shader, "scale", 1.05);
    set_uniform(shader, "value", 0);
    draw_array(renderstate.vbo.reticle);
    set_uniform(shader, "scale", 1);
    set_uniform(shader, "value", 1);
    float checkpoint = fabs(state.player.phase - 0.5) * 20;
    if (checkpoint < 1.0)
        set_uniform(shader, "value", checkpoint);
    draw_array(renderstate.vbo.reticle);
}

void draw_life_meter(const RenderArgs& args)
{
    GLuint shader = args.rs.shaders.meter;
    glUseProgram(shader);
    set_uniform(shader, "percent", args.gs.player.life);
    set_uniform(shader, "ticks", args.ticks);
    draw_array(args.rs.vbo.viewport, GL_QUADS);
}

void draw_entities(const RenderArgs& args)
{
    RS renderstate = args.rs;
    GS state = args.gs;
    u32 ticks = args.ticks;

    for (int i = 0; i < MAX_ENTITIES; ++i)
    {
        const Entity& e = state.entities[i];
        if (e.life <= 0) continue;

        if (e.type == E_ROCKET)
        {
            GLuint shader = renderstate.shaders.player;
            glUseProgram(shader);
            set_uniform(shader, "offset", e.pos);
            set_uniform(shader, "rotation", e.rotation);
            set_uniform(shader, "ticks", ticks);
            set_uniform(shader, "scale", state.player.size);
            draw_array(renderstate.vbo.player);
        }
        if (e.type == E_BULLET)
        {
            GLuint shader = renderstate.shaders.player;
            glUseProgram(shader);
            set_uniform(shader, "offset", e.pos);
            set_uniform(shader, "ticks", ticks);
            set_uniform(shader, "rotation", ticks / 100.0f);
            set_uniform(shader, "scale", state.player.size * 0.1);
            draw_array(renderstate.vbo.player);
        }
        if (e.type == E_TURD)
        {
            GLuint shader = renderstate.shaders.turd;
            glUseProgram(shader);
            set_uniform(shader, "offset", e.pos);
            set_uniform(shader, "scale", 0.04 + 0.01 * (1.0 - e.life));
            set_uniform(shader, "rotation", e.rotation);
            set_uniform(shader, "ticks", ticks);
            set_uniform(shader, "phase", 0.5 - state.player.phase);
            draw_array(renderstate.vbo.player);
        }
        if (e.type == E_NOVA)
        {
            GLuint shader = renderstate.shaders.nova;
            glUseProgram(shader);
            set_uniform(shader, "scale", (100 - 100*e.life));
            set_uniform(shader, "offset", e.pos);
            set_uniform(shader, "rotation", e.rotation);
            set_uniform(shader, "center", e.pos);
            set_uniform(shader, "ticks", 0); // HACK
            set_uniform(shader, "radius", (100 - 100*e.life));
            draw_array(renderstate.vbo.nova);
        }
        if (e.type == E_XPCHUNK)
        {
            GLuint shader = renderstate.shaders.xpchunk;
            glUseProgram(shader);
            set_uniform(shader, "offset", e.pos);
            set_uniform(shader, "rotation", 0.0);
            set_uniform(shader, "scale", 0.3);
            set_uniform(shader, "ticks", ticks);
            set_uniform(shader, "hue", e.hue);
            draw_array(renderstate.vbo.enemy);
        }
        if (e.type == E_ENEMY)
        {
            GLuint shader = renderstate.shaders.enemy;
            glUseProgram(shader);
            set_uniform(shader, "offset", e.pos);
            set_uniform(shader, "rotation", ticks / 400.0f);
            set_uniform(shader, "scale", 0.9);
            set_uniform(shader, "ticks", ticks);
            set_uniform(shader, "hue", e.hue);
            draw_array(renderstate.vbo.enemy);
        }
    }
}

void draw_glowy_things(const RenderArgs& args)
{
    // Render "player" items
    draw_square(args);
    draw_triangle(args);
    draw_reticle(args);
    draw_life_meter(args);

}

void apply_two_pass_glow(const RenderState& renderstate)
{
    glUseProgram(renderstate.shaders.post_blur);
    glBindTexture(GL_TEXTURE_2D, renderstate.fbo.a.texture);
    use_framebuffer(renderstate.fbo.b);
    Vec uX = {1, 0};
    set_uniform(renderstate.shaders.post_blur, "texSource", renderstate.fbo.a.texture);
    check_error("setting texture uniform 2");
    set_uniform(renderstate.shaders.post_blur, "dir", uX);
    set_uniform(renderstate.shaders.post_blur, "textureSize", renderstate.fbo.a.width);
    draw_array(renderstate.vbo.viewport, GL_QUADS);

    glBindTexture(GL_TEXTURE_2D, renderstate.fbo.b.texture);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Vec uY = {0, 1};
    set_uniform(renderstate.shaders.post_blur, "texSource", renderstate.fbo.b.texture);
    check_error("setting texture uniform 3");
    set_uniform(renderstate.shaders.post_blur, "dir", uY);
    set_uniform(renderstate.shaders.post_blur, "textureSize", renderstate.fbo.a.height);
    draw_array(renderstate.vbo.viewport, GL_QUADS);
    glUseProgram(0);

}

// Render a frame
void render(GameState& state, u32 ticks, bool debug, const Input& input)
{
    RenderArgs args = { _params, state, _renderstate, ticks, debug };

    static bool glow = false;
    glow ^= input.sys.glowtoggle;

    // Clear
    if (glow)
    { 
        use_framebuffer(args.rs.fbo.a);
    }

    glClear(GL_COLOR_BUFFER_BIT);

    // Render background
    draw_background(args);

    // Render gameplay
    draw_glowy_things(args);

    // Glow filter to screen; Render gameplay again
    if (glow) 
    {
        apply_two_pass_glow(args.rs);
        draw_glowy_things(args);
    }

    // Render bullets, enemies and turds
    draw_entities(args);

}

} // namespace gfx

