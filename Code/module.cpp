#include "common.h"
#include <stdio.h>

template<class T> static vector<T> &operator<<(vector<T> &a, T const& b) { a.push_back(b); return a; }

template<class T> static vector<T> &operator, (vector<T> &a, T const& b) { return a << b; }

typedef struct {
    mat3 rot;
    vec3 pos;
}Instance;

typedef struct {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
}Command;

typedef enum {
    Null = -1,

    Root,
    Hips,
    Spine1,
    Spine2,
    Spine3,
    Neck,
    Head,
    Head_End,

    Shoulder_R,
    Elbow_R,
    Wrist_R,
    Hand_R,
    Leg_R,
    Knee_R,
    Ankle_R,
    Toe_R,

    Shoulder_L,
    Elbow_L,
    Wrist_L,
    Hand_L,
    Leg_L,
    Knee_L,
    Ankle_L,
    Toe_L,

    Joint_Max,
}IkRig;


#include <glad/glad.h>

extern "C" ivec2 mainAnimation(float t, vec2 res, vec4 iMouse)
{
    vec3 ta = vec3(0,1,0);
    float m = t;//sin(t*3.0)*0.17 + .5;
    vec3 ro = ta + vec3(sin(m),.5,cos(m))*1.5f;

    vector<vec3> U;
    vector<Instance> I;

    // extern const vec3 joints[];
    extern const IkRig parentTable[];
    extern const bool hasMesh[];
    extern const vector<vec3> jointsLocal;

    vector<vec3> world(Joint_Max, vec3(0));
    vector<mat3> local(Joint_Max, mat3(1));
    // local[Hips] = rotateY(t);

    for (int i=Hips; i<Joint_Max; i++)
    {
        int p = parentTable[i];
        world[i] = world[p] + jointsLocal[i] * local[p];
        local[i] *= local[p];
    }

    for (int i=0; i<Joint_Max; i++)
    {
        if (!hasMesh[i]) { continue; }

        int p = parentTable[i];
        float w = .2 + hash11(i + 349) * .1;
            w *= 1 - (i>Shoulder_R || p == Neck) * .5;
        float r = length(jointsLocal[i]);
        mat3 rot = rotationAlign(jointsLocal[i]/r, vec3(0,0,1));
        vec3 sca = abs(rot * vec3(w, w, r)) * .5f;

        mat3 swi = matrixCompMult(local[i], mat3(sca,sca,sca));
        vec3 ce = mix(world[i], world[p], .5f);
        I << Instance{ swi, ce };
    }

    void loadBuffers(vector<vec3> const& U, vector<Instance> const& I);
    loadBuffers(U, I);
    const float data[] = {
        res.x,res.y, t, 0,
        ro.x,ro.y,ro.z, 0,
        ta.x,ta.y,ta.z, 0,
    };
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof data, data);
    return ivec2(1, U.size());
}

void loadBuffers(vector<vec3> const& U, vector<Instance> const& I)
{
    static vector<Command> C;
    static GLuint vao, vbo1, vbo2, ibo, ebo, ubo, cbo, frame;
    if (!frame++)
    {
        vector<Vertex> V;
        vector<Index> F;

        uint firstIndex = 0;
        uint baseVertex = 0;
        tCubeMap(V, F, 2);
        C << Command{ (uint)F.size()-firstIndex, 0, firstIndex, baseVertex, 0 };
        firstIndex = F.size();
        baseVertex = V.size();
        tCapsule(V, F, 0);
        C << Command{ (uint)F.size()-firstIndex, 0, firstIndex, baseVertex, 0 };
        firstIndex = F.size();
        baseVertex = V.size();

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo1);
        glGenBuffers(1, &vbo2);
        glGenBuffers(1, &ubo);
        glGenBuffers(1, &ebo);
        glGenBuffers(1, &cbo);
        glGenBuffers(1, &ibo);

        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, 64, NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, F.size() * sizeof F[0], F.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, vbo1);
        glBufferData(GL_ARRAY_BUFFER, V.size() * sizeof V[0], V.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);

        glBindBuffer(GL_ARRAY_BUFFER, vbo2);
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, 12, 0);

        glBindBuffer(GL_ARRAY_BUFFER, ibo);
        for (size_t off=0, i=4; i<8; i++, off+=12)
        {
            glEnableVertexAttribArray(i);
            glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, sizeof(Instance), (void*)off);
            glVertexAttribDivisor(i, 1);
        }
    }

    C[0].instanceCount = I.size();

    { // command buffer
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo);
        int oldSize;
        glGetBufferParameteriv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_SIZE, &oldSize);
        int newSize = C.size() * sizeof C[0];
        if (oldSize < newSize)
        {
            glBufferData(GL_DRAW_INDIRECT_BUFFER, newSize, C.data(), GL_DYNAMIC_DRAW);
        }
        else
        {
            glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, newSize, C.data());
        }
    }
    { // index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    }
    { // channel 0 1
        glBindBuffer(GL_ARRAY_BUFFER, vbo1);
    }
    { // channel 4 5 6 7
        glBindBuffer(GL_ARRAY_BUFFER, ibo);
        int oldSize;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &oldSize);
        int newSize = I.size() * sizeof I[0];
        if (oldSize < newSize)
        {
            glBufferData(GL_ARRAY_BUFFER, newSize, I.data(), GL_DYNAMIC_DRAW);
        }
        else
        {
            glBufferSubData(GL_ARRAY_BUFFER, 0, newSize, I.data());
        }
    }
    { // channel 8
        glBindBuffer(GL_ARRAY_BUFFER, vbo2);
        int oldSize;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &oldSize);
        int newSize = U.size() * sizeof U[0];
        if (oldSize < newSize)
        {
            glBufferData(GL_ARRAY_BUFFER, newSize, U.data(), GL_DYNAMIC_DRAW);
        }
        else
        {
            glBufferSubData(GL_ARRAY_BUFFER, 0, newSize, U.data());
        }
    }
}
