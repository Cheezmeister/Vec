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

namespace gfx
{

    // Nasty globals
    GLuint vbo;
    GLuint reticle_vbo;
    GLuint shader;
    GLuint reticle_shader;

    // Update a VBO
    void update_vbo(VertexBuffer<3> vertexPositions, GLuint which)
    {
        glBindBuffer(GL_ARRAY_BUFFER, which);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions.flat), vertexPositions.flat, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // Check for GL errors
    void check_error(const string& message)
    {
        GLenum error = glGetError();
        if (error)
        {
            // TODO get text of error
            log << message << " reported error: " << error << endl;
        }
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
                          /* "out vec3 color; \n" */
                          "varying vec4 glPos; \n"
                          "void main() { \n"
                          "  vec3 c = cross(vec3(1, 0, 0), vec3(glPos.x, glPos.y, 0)); \n"
                          "  float green = length(c); \n"
                          "  gl_FragColor = vec4(glPos.x,green,glPos.y,0); \n"
                          "} \n"
        );
        GLuint program = arcsynthesis::CreateProgram(vertex, fragment);
        return program;
    }
    void init()
    {
        // Set up VBO
        VertexBuffer<3> vertexPositions = {
             0.75f, 0.0f, 0.0f, 1.0f,
            -0.75f, 0.75f, 0.0f, 1.0f,
            -0.75f, -0.75f, 0.0f, 1.0f,
        };
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions.flat, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        VertexBuffer<4> reticleVertices = {
            0.1f,  0.1f, 0.0f, 1.0f,
            0.1f, -0.1f, 0.0f, 1.0f,
            -0.1f, -0.1f, 0.0f, 1.0f,
            -0.1f,  0.1f, 0.0f, 1.0f,
        };
        glGenBuffers(1, &reticle_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, reticle_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(reticleVertices), reticleVertices.flat, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Init shaders
        shader = make_shader();
        reticle_shader = make_shader();

        // Misc setup
        glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
        check_error("clearcolor");
    }

    // Render a frame
    void render(GameState& state, u32 ticks)
    {
        // Clear
        glClear(GL_COLOR_BUFFER_BIT);
        check_error("clearing to blue");

        // Render "player"
        GLuint loc = glGetUniformLocation(shader, "offset");      check_error("getting param");
        glUseProgram(shader);                                     check_error("binding shader");
        glUniform2f(loc, state.player.pos.x, state.player.pos.y); check_error("setting uniform");
        loc = glGetUniformLocation(shader, "rotation");           check_error("getting param");
        glUniform1f(loc, state.player.rotation); check_error("setting uniform");
        loc = glGetUniformLocation(shader, "ticks");           check_error("getting param");
        glUniform1f(loc, ticks / 100.0f); check_error("setting ticks");
        glBindBuffer(GL_ARRAY_BUFFER, vbo);                       check_error("binding buf");
        glEnableVertexAttribArray(0);                             check_error("enabling vaa");
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);    check_error("calling vap");
        glDrawArrays(GL_TRIANGLES, 0, 3);                         check_error("drawing arrays");
        glDisableVertexAttribArray(0);                            check_error("disabling vaa");

        // Render reticle
        loc = glGetUniformLocation(reticle_shader, "offset");   check_error("getting param");
        glUseProgram(reticle_shader);                             check_error("binding shader");
        glUniform2f(loc, 2*state.player.reticle.x, 2*state.player.reticle.y); check_error("setting uniform");
        glBindBuffer(GL_ARRAY_BUFFER, reticle_vbo);               check_error("binding buf");
        glEnableVertexAttribArray(0);                             check_error("enabling vaa");
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);    check_error("calling vap");
        glDrawArrays(GL_QUADS, 0, 4);                         check_error("drawing arrays");
        glDisableVertexAttribArray(0);                            check_error("disabling vaa");


    }
}

