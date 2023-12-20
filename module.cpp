#include <stdio.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <boost/container/vector.hpp>
using boost::container::vector;
using namespace glm;

static vector<float> &operator<<(vector<float> &a, float b)
{
    a.push_back(b);
    return a;
}

static vector<float> &operator,(vector<float> &a, float b)
{
    return a << b;
}

static vector<float> &operator<<(vector<float> &a, vec3 b)
{
    return a << b.x, b.y, b.z;
}

static vector<float> &operator,(vector<float> &a, vec3 b)
{
    return a << b;
}

typedef struct {
    GLuint vertexCount;
    GLuint instanceCount;
    GLuint firstVertex;
    GLuint baseInstance;
}DrawArraysIndirectCommand;

extern "C" int mainGeometry(float)
{
    const float cubeStrip[] = {
        0,1,1,   1,1,1,   0,0,1,
        1,0,1,   1,0,0,   1,1,1,
        1,1,0,   0,1,1,   0,1,0,
        0,0,1,   0,0,0,   1,0,0,
        0,1,0,   1,1,0
    };

    DrawArraysIndirectCommand cmd[] ={ countof(cubeStrip), 1, 0, 0 };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cubeStrip), cubeStrip);
    glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(cmd), cmd);
    return countof(cmd);
}

extern "C" int mainAnimation(float t)
{
    return 0;
}
