#include <cmath>
#include <OpenGL/gl.h>
#include "bml.h"

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

typedef struct _RenderState {
  struct _Shaders {
    GLuint player;
    GLuint reticle;
  } shaders;
  struct _VBO {
    GLuint player;
    GLuint reticle;
  } vbo;
} RenderState;

namespace gfx
{

    RenderState renderstate;


    // Check for GL errors
    void check_error(const string& message)
    {
        GLenum error = glGetError();
        if (error)
        {
            // TODO get text of error
            logger << message << " reported error: " << error << endl;
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
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW); check_error("buffering");
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return ret;
    }

    GLuint make_polygon_vbo(int sides, float radius)
    {
        float* vertices = make_polygon_vertex_array(sides, radius);
        size_t memsize = 12 * sides * sizeof(float);
        GLuint ret = make_vbo(memsize, vertices);
        delete[] vertices;
        return ret;
    }

    // Player shader
    GLuint make_shader()
    {
        GLuint vertex = arcsynthesis::CreateShader(GL_VERTEX_SHADER,
            "#version 120  \n"
            "attribute vec4 inPos; \n"
            "uniform vec2 offset; \n"
            "uniform float rotation; \n"
            "uniform float ticks; \n"
            "varying vec4 glPos; \n"
            "void main() { \n"
            "  vec2 rotated;\n"
            "  rotated.x = inPos.x * cos(rotation) - inPos.y * sin(rotation);\n"
            "  rotated.y = inPos.x * sin(rotation) + inPos.y * cos(rotation);\n"
            "  vec2 pos = rotated * (0.2 + 0.1 * sin(ticks));\n"
            "  gl_Position = glPos = vec4(offset + pos, 0, 1); \n"
            "} \n"
        );

        GLuint fragment = arcsynthesis::CreateShader(GL_FRAGMENT_SHADER,
            "#version 120 \n"
            "varying vec4 glPos; \n"
            "uniform float ticks; \n"
            "void main() { \n"
            "  float r = 0.0 + 0.5 * sin(ticks/2); \n"
            "  float g = 0.5 + 0.5 * sin(ticks/3); \n"
            "  float b = 0.0 + 0.5 * sin(ticks*1.5); \n"
            "  gl_FragColor = vec4(r, g, b, 0); \n"
            "} \n"
        );
        GLuint program = arcsynthesis::CreateProgram(vertex, fragment);
        return program;
    }

    void init()
    {
        // Set up VBO
        renderstate.vbo.player = make_polygon_vbo(3, 0.5);
        renderstate.vbo.reticle = make_polygon_vbo(5, 0.2);

        // Init shaders
        renderstate.shaders.player = make_shader();
        renderstate.shaders.reticle = make_shader();

        // Misc setup
        glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
        check_error("clearcolor");
    }

    // Render a frame
    void render(GameState& state, u32 ticks)
    {
        GLuint shader;
        GLuint vbo;

        // Clear
        glClear(GL_COLOR_BUFFER_BIT);
        check_error("clearing to pink");

        // Render "player"
        shader = renderstate.shaders.player;
        vbo = renderstate.vbo.player;
        glUseProgram(shader);                                     check_error("binding shader");
        set_uniform(shader, "offset", state.player.pos);          check_error("setting offset");
        set_uniform(shader, "rotation", state.player.rotation);   check_error("setting uniform");
        set_uniform(shader, "ticks", ticks / 100.0f);             check_error("setting ticks");
        glBindBuffer(GL_ARRAY_BUFFER, vbo);                       check_error("binding buf");
        glEnableVertexAttribArray(0);                             check_error("enabling vaa");
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);    check_error("calling vap");
        glDrawArrays(GL_TRIANGLES, 0, 12*3);                         check_error("drawing arrays");
        glDisableVertexAttribArray(0);                            check_error("disabling vaa");

        // Render reticle
        shader = renderstate.shaders.reticle;
        vbo = renderstate.vbo.reticle;
        glUseProgram(shader);                             check_error("binding shader");
        set_uniform(shader, "offset", 2*state.player.reticle); check_error("getting param");
        set_uniform(shader, "rotation", ticks / 100.0f); check_error("getting param");
        glBindBuffer(GL_ARRAY_BUFFER, vbo);               check_error("binding buf");
        glEnableVertexAttribArray(0);                             check_error("enabling vaa");
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);    check_error("calling vap");
        glDrawArrays(GL_TRIANGLES, 0, 12*5);                        check_error("drawing arrays");
        glDisableVertexAttribArray(0);                            check_error("disabling vaa");


    }
}

