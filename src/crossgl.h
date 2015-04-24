/* This is why we can't have nice things. */
#if _WIN32
#include <Windows.h>
#include <GL/gl.h>
#elif __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

