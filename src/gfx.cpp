#include <cmath>
#include "GL/glew.h"
#include "crossgl.h"
#include "vec.h"

// http://stackoverflow.com/a/13874526
#define MAKE_SHADER(version, shader)  "#version " #version "\n" #shader

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

typedef struct _RenderState {
    struct _Shaders {
        GLuint enemy;
        GLuint player;
        GLuint reticle;
        GLuint turd;
        GLuint viewport;
        GLuint nova;
        GLuint xpchunk;
    } shaders;
    struct _VBOs {
        VBO enemy;
        VBO player;
        VBO nova;
        VBO reticle;
        VBO viewport;
    } vbo;
} RenderState;

typedef struct _RenderParams {
    struct _Bullet {
        float rotspeed;
    } bullet;
    struct _Enemy {
        float rotspeed;
    } enemy;
} RenderParams;

RenderState renderstate;
RenderParams params = {
    { 0.01f },
    { 0.0025f }
};

// Check for GL errors
void check_error(const string& message)
{
    GLenum error = glGetError();
    if (error)
    {
        logger << message.c_str() << " reported error: " << error << ' ' << gluErrorString(error) << endl;
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

void draw_array(VBO vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo.handle);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLES, 0, vbo.size);
    glDisableVertexAttribArray(0);
}

GLuint make_shader(GLuint vertex, GLuint fragment)
{
    return arcsynthesis::CreateProgram(vertex, fragment);
}

void init()
{
    // Compile shaders
    GLuint vs_pulse = arcsynthesis::CreateShader(GL_VERTEX_SHADER, "#version 120\n"
#include "pulse.vs"
                                                );
    GLuint vs_wiggle = arcsynthesis::CreateShader(GL_VERTEX_SHADER, "#version 120  \n"
#include "wiggle.vs"
                                                 );
    GLuint fs_scintillate = arcsynthesis::CreateShader(GL_FRAGMENT_SHADER, "#version 120 \n"
#include "scintillate.fs"
                                                      );
    GLuint fs_pulse = arcsynthesis::CreateShader(GL_FRAGMENT_SHADER, "#version 120 \n"
#include "pulse.fs"
                                                );
    GLuint fs_circle = arcsynthesis::CreateShader(GL_FRAGMENT_SHADER, "#version 120 \n"
#include "circle.fs"
                                                 );

    // Set up VBO
    renderstate.vbo.player = make_polygon_vbo(3, 0, 0.5);
    renderstate.vbo.nova = make_polygon_vbo(3, 0.48, 0.5);
    renderstate.vbo.reticle = make_polygon_vbo(5, 0.3, 0.4);
    renderstate.vbo.enemy = make_polygon_vbo(6, 0.1, 0.3);

    VertexBuffer<4> viewportVertices = {
        -1, 1, 0, 1,
        -1, -1, 0, 1,
        1, -1, 0, 1,
        1,  1, 0, 1,
    };
    renderstate.vbo.viewport.handle = make_vbo(sizeof(viewportVertices), viewportVertices.flat);
    renderstate.vbo.viewport.size = 4;

    // Init shaders
    // TODO profile this
    renderstate.shaders.player = make_shader(vs_pulse, fs_scintillate);
    renderstate.shaders.reticle = make_shader(vs_pulse, fs_pulse);
    renderstate.shaders.enemy = make_shader(vs_wiggle, fs_pulse);
    renderstate.shaders.viewport = make_shader(vs_pulse, fs_scintillate);
    renderstate.shaders.turd = make_shader(vs_wiggle, fs_scintillate);
    renderstate.shaders.nova = make_shader(vs_wiggle, fs_circle);
    renderstate.shaders.xpchunk = make_shader(vs_pulse, fs_pulse);

    // Misc setup
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    check_error("clearcolor");
}

void draw_background(GameState& state, u32 ticks)
{
    GLuint shader = renderstate.shaders.viewport;
    VBO vbo = renderstate.vbo.viewport;
    glUseProgram(shader);

    // TODO figure out why can't set this prior to glUseProgram
    set_uniform(renderstate.shaders.viewport, "value", 0.3); //darken

    set_uniform(shader, "scale", 4);
    set_uniform(shader, "ticks", ticks + 1400);
    set_uniform(shader, "phase", state.player.phase + 0.5);
    glBindBuffer(GL_ARRAY_BUFFER, vbo.handle);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_QUADS, 0, vbo.size);

    set_uniform(shader, "scale", 2);
    set_uniform(shader, "ticks", ticks + 1200);
    set_uniform(shader, "phase", state.player.phase + 0.6);
    glDrawArrays(GL_QUADS, 0, vbo.size);

    set_uniform(shader, "scale", 1);
    set_uniform(shader, "ticks", ticks + 1000);
    set_uniform(shader, "phase", state.player.phase + 0.7);
    glDrawArrays(GL_QUADS, 0, vbo.size);

    glDisableVertexAttribArray(0);
}

void draw_player(GameState& state, u32 ticks)
{
    GLuint shader = renderstate.shaders.player;
    glUseProgram(shader);
    set_uniform(shader, "offset", state.player.pos);
    set_uniform(shader, "rotation", state.player.rotation);
    set_uniform(shader, "ticks", ticks);
    set_uniform(shader, "phase", state.player.phase);
    float checkpoint = fabs(state.player.phase - 0.5) * 40;
    if (checkpoint < 1.0)
        set_uniform(shader, "value", checkpoint);
    set_uniform(shader, "scale", state.player.size);
    draw_array(renderstate.vbo.player);
}

void draw_reticle(GameState& state, u32 ticks)
{
    GLuint shader = renderstate.shaders.reticle;
    glUseProgram(shader);
    set_uniform(shader, "offset", state.player.reticle);
    set_uniform(shader, "rotation", ticks / 1000.0f);
    set_uniform(shader, "ticks", ticks);
    set_uniform(shader, "scale", 1);
    draw_array(renderstate.vbo.reticle);
}

void draw_entities(GameState& state, u32 ticks)
{
    for (int i = 0; i < MAX_ENTITIES; ++i)
    {
        Entity& e = state.entities[i];
        if (e.life <= 0) continue;

        if (e.type == E_ROCKET)
        {
            GLuint shader = renderstate.shaders.player;
            glUseProgram(shader);
            set_uniform(shader, "offset", e.pos);
            set_uniform(shader, "rotation", e.rotation);
            set_uniform(shader, "ticks", ticks);
            set_uniform(shader, "scale", 1);
            draw_array(renderstate.vbo.player);
        }
        if (e.type == E_BULLET)
        {
            GLuint shader = renderstate.shaders.player;
            glUseProgram(shader);
            set_uniform(shader, "offset", e.pos);
            set_uniform(shader, "ticks", ticks);
            set_uniform(shader, "rotation", ticks / 100.0f);
            set_uniform(shader, "scale", 0.2);
            draw_array(renderstate.vbo.player);
        }
        if (e.type == E_TURD)
        {
            GLuint shader = renderstate.shaders.turd;
            glUseProgram(shader);
            set_uniform(shader, "offset", e.pos);
            set_uniform(shader, "scale", 0.2 + 0.5 * (1.0 - e.life));
            set_uniform(shader, "rotation", e.rotation);
            set_uniform(shader, "ticks", ticks);
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
            set_uniform(shader, "scale", 0.4);
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
            set_uniform(shader, "scale", 0.6);
            set_uniform(shader, "ticks", ticks);
            set_uniform(shader, "hue", e.hue);
            draw_array(renderstate.vbo.enemy);
        }
    }
}

// Render a frame
void render(GameState& state, u32 ticks, bool debug)
{

    // TODO compress state+ticks into some param struct

    // Clear
    glClear(GL_COLOR_BUFFER_BIT);
    check_error("clearing to black");

    // Render background thingie
    draw_background(state, ticks);

    // Render "player" triangle
    draw_player(state, ticks);

    // Render reticle
    draw_reticle(state, ticks);

    // Render bullets, enemies and turds
    draw_entities(state, ticks);
}

} // namespace gfx

