#include <stdio.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <boost/container/vector.hpp>
using boost::container::vector;
using namespace glm;

static vector<uint16_t> &operator<<(vector<uint16_t> &a, uint16_t b)
{
    a.push_back(b);
    return a;
}

static vector<uint16_t> &operator,(vector<uint16_t> &a, uint16_t b)
{
    return a << b;
}

typedef struct {
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    GLuint baseVertex;
    GLuint baseInstance;
}DrawElementsIndirectCommand;

extern "C" int mainGeometry()
{
    typedef struct { vec3 position, normal; } Vertex;

    vector<Vertex> V;
    vector<uint16> I;

    const mat3x3 Cube_Map_Rotations[] = {
        { 1,0,0, 0,1,0, 0,0,1, }, // forward
        { -1,0,0, 0,1,0, 0,0,-1, }, // back
        { 0,0,-1, 0,1,0, 1,0,0, }, // left
        { 0,0,1, 0,1,0, -1,0,0, }, // right
        { 1,0,0, 0,0,1, 0,-1,0, }, // top
        { 1,0,0, 0,0,-1, 0,1,0, }, // bottom
    };

    // cube map sampling
    const ivec3 Size = {4, 4, 8};
    const float Scale = 1./8;

    for (int f=0; f<6; f++) {
        const ivec3 Axes[] = {
            {0,1,2}, {0,1,2}, {2,1,0},
            {2,1,0}, {0,2,1}, {0,2,1},
        };
        const int sizeX = Size[Axes[f].x];
        const int sizeY = Size[Axes[f].y];
        const int sizeZ = Size[Axes[f].z];
        size_t rows = sizeX*2+1;

        for (int t=-sizeY; t<=sizeY; t++) {
            for (int s=-sizeX; s<=sizeX; s++) {
                vec3 p = vec3(s, t, sizeZ) * Scale * Cube_Map_Rotations[f];

                const vec3 Bounds = vec3(Size)*Scale;
                const float roundness = 0.05;
                vec3 inner = min(max(p, -Bounds+roundness), Bounds-roundness);
                p = inner + normalize(p - inner) * roundness;

                uint16_t idx = (uint16_t)V.size();
                V.push_back({ p, vec3(0) });
                if (s != sizeX && t != sizeY) {
                    I << idx,idx+1,idx+rows,idx+rows,idx+1,idx+1+rows;
                }
            }
        }
    }

    for (size_t i=0; i<I.size(); i+=3) {
        size_t  i0 = (size_t)I[i+0],
                i1 = (size_t)I[i+1],
                i2 = (size_t)I[i+2];
        vec3    p0 = V[i0].position,
                p1 = V[i1].position,
                p2 = V[i2].position;
        vec3 n = cross(p1-p0, p2-p1);
        V[i0].normal += n;
        V[i1].normal += n;
        V[i2].normal += n;
    }

    for (auto &v : V) v.normal = normalize(v.normal);

    // TODO: box modeling cube sphere capsule cylinder
    // instancing

    size_t instanceStartIndex = V.size() * sizeof V[0];
    uint8_t data[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12 };

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, 1, (void*)instanceStartIndex);
    glVertexAttribDivisor(2, 1);

    glBufferData(GL_ARRAY_BUFFER, instanceStartIndex + sizeof data, V.data(), GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, instanceStartIndex, sizeof data, data);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, I.size()*sizeof I[0], I.data(), GL_STATIC_DRAW);
    DrawElementsIndirectCommand cmd[] = { GLuint(I.size()), 1, 0, 0, 1 };
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof cmd, cmd, GL_STATIC_DRAW);

    return countof(cmd);
}

extern "C" void mainAnimation(float *o, float t)
{
    t = 1.;
    vec3 ta = vec3(0);
    vec3 ro = ta + vec3(sin(t),.5,cos(t))*5.f;
    *(mat2x4*)o = mat2x4(mat2x3(ro, ta));
}
