#include "common.h"

#include <stdio.h>
#include <glad/glad.h>
#include <boost/container/vector.hpp>
using boost::container::vector;

template<class T> static vector<T> &operator<<(vector<T> &a, T const& b) { a.push_back(b); return a; }

template<class T> static vector<T> &operator, (vector<T> &a, T const& b) { return a << b; }

// Catmull Rom
static mat4 coefs = {
     0, 2, 0, 0,
    -1, 0, 1, 0,
     2,-5, 4,-1,
    -1, 3,-3, 1 };

/// @source: Texturing And Modeling A Procedural Approach
float spline( const float *k, int n, float t )
{
    t *= n - 3;
    int i = int(floor(t));
    t -= i;
    k += i;

    vec4 f = (coefs * .5f) * vec4(1, t, t*t, t*t*t);
    vec4 j = *(vec4*)k;
    return dot(f, j);
}

typedef struct {
    vec3 pos, nor;
}Vertex;

typedef struct {
    vec3 sca, pos;
    mat3 rot;
}Instance;

typedef struct {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
}Command;


static const mat3x3 cubemap[] = {
    { 1,0,0, 0,1,0, 0,0,1, }, // forward
    { -1,0,0, 0,1,0, 0,0,-1, }, // back
    { 0,0,1, 0,1,0, -1,0,0, }, // right
    { 0,0,-1, 0,1,0, 1,0,0, }, // left
    { 1,0,0, 0,0,1, 0,-1,0, }, // top
    { 1,0,0, 0,0,-1, 0,1,0, }, // bottom
};

static Command genCubeMap(vector<Vertex> &V, vector<ushort> &F, uint N)
{
    uint baseVertex = V.size();
    uint firstIndex = F.size();

    for (int i=0; i<6; i++)
        for (int t=0; t<N; t++)
            for (int s=0; s<N; s++)
    {
        vec3 uv = vec3(vec2(s,t)/(N-1.f)*2.f-1.f, 1) * cubemap[i];
        uint idx = V.size() - baseVertex;
        V.push_back({ uv, });
        if (s != N-1 && t != N-1)
        {
            F.push_back(idx);
            F.push_back(idx+1);
            F.push_back(idx+N);
            F.push_back(idx+N);
            F.push_back(idx+1);
            F.push_back(idx+1+N);
        }
    }

    // generate normals
    if (N <= 2)
    {
        for (size_t i=baseVertex; i<V.size(); i++)
        {
            V[i].nor = vec3(0);
        }
        for (size_t i=firstIndex; i<F.size(); i+=3)
        {
            int i0 = F[i+0] + baseVertex,
                i1 = F[i+1] + baseVertex,
                i2 = F[i+2] + baseVertex;
            vec3 p0 = V[i0].pos,
                 p1 = V[i1].pos,
                 p2 = V[i2].pos;
            vec3 nor = cross(p1-p0, p2-p1);
            V[i0].nor += nor;
            V[i1].nor += nor;
            V[i2].nor += nor;
        }
        for (size_t i=baseVertex; i<V.size(); i++)
        {
            V[i].nor = normalize(V[i].nor);
        }
    }

    return { (uint)F.size()-firstIndex, 0, firstIndex, baseVertex, 0 };
}

typedef struct {
    vec3 local;
    int objectId;
    int parentId;
    vector<int> childIds;
}Node;

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
    Foot_R,
    Toe_R,

    Shoulder_L,
    Elbow_L,
    Wrist_L,
    Hand_L,
    Leg_L,
    Knee_L,
    Foot_L,
    Toe_L,

    Joint_Max,
}IkRig;

const vec3 pNeck = {0,1.7,0};
const vec3 pWrist = {.7,1.6,0};
const vec3 pFoot = {.15,0,0};

const vec3 Mirror = vec3(-1,1,1);
const vec3 joints[] = {
    {},
    {0,1.0,0}, // Hips
    mix(joints[Hips], pNeck, .25), // Spine1
    mix(joints[Hips], pNeck, .50), // Spine2
    mix(joints[Hips], pNeck, .75), // Spine3
    pNeck, // Neck
    {0,1.75,0}, // Head
    joints[Head] + vec3(0,.2,0), // Head_End

    {.2,1.6,0}, // Shoulder_R
    mix(joints[Shoulder_R], pWrist, .5), // Elbow_R
    pWrist, // Wrist_R
    joints[Wrist_R] + vec3(.1,0,0), // Hand_R
    {.15,1.1,0}, // Leg_R
    mix(joints[Leg_R], pFoot, .5) + vec3(0,0,.001), // Knee_R
    pFoot, // Foot_R
    joints[Foot_R] + vec3(0,0,.2), // Toe_R

    joints[Shoulder_R] * Mirror,
    joints[Elbow_R] * Mirror,
    joints[Wrist_R] * Mirror,
    joints[Hand_R] * Mirror,
    joints[Leg_R] * Mirror,
    joints[Knee_R] * Mirror,
    joints[Foot_R] * Mirror,
    joints[Toe_R] * Mirror,
};

const int bones[][3] = {
    Root,Null,Null,
    Hips,Root,Null,
    Spine1,Hips,1,
    Spine2,Spine1,1,
    Spine3,Spine2,1,
    Neck,Spine2,1,
    Head,Neck,1,
    Head_End,Head,1,

    Shoulder_R,Spine2,Null,
    Elbow_R,Shoulder_R,1,
    Wrist_R,Elbow_R,1,
    Hand_R,Wrist_R,1,
    Leg_R,Hips,Null,
    Knee_R,Leg_R,1,
    Foot_R,Knee_R,1,
    Toe_R,Foot_R,1,

    Shoulder_L,Spine2,Null,
    Elbow_L,Shoulder_L,1,
    Wrist_L,Elbow_L,1,
    Hand_L,Wrist_L,1,
    Leg_L,Hips,Null,
    Knee_L,Leg_L,1,
    Foot_L,Knee_L,1,
    Toe_L,Foot_L,1,
};

static vector<Node> gSkeleton;
static GLuint ibo, vbo, ebo, cbo, tex, frame = 0;
static vector<Command> C;

extern "C" ivec4 mainGeometry(vector<float> &)
{
    gSkeleton << Node{ {}, Null, Null, };
    for (int i=Hips; i<Joint_Max; i++)
    {
        int parentId = bones[i][1];
        int objectId = bones[i][2];
        gSkeleton << Node{ joints[i]-joints[parentId], objectId, parentId, };
        gSkeleton[parentId].childIds.push_back(i);
    }

    if (!frame++)
    {
        glGenBuffers(1, &ebo);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);
        glGenBuffers(1, &cbo);
        glGenTextures(1, &tex);
    }

    {
        vector<Vertex> V;
        vector<ushort> F;

        uint firstIndex = 0;
        uint baseVertex = 0;

        C << genCubeMap(V, F, 2);
        firstIndex = F.size();
        baseVertex = V.size();

        C << genCubeMap(V, F, 2);
        for (uint i=baseVertex; i<V.size(); i++)
        {
            V[i].pos += vec3(0,1,0);
            V[i].pos *= .5;
        }
        firstIndex = F.size();
        baseVertex = V.size();

        // sphere
        C << genCubeMap(V, F, 10);
        for (uint i=baseVertex; i<V.size(); i++)
        {
            vec3 uv = V[i].pos;
            float sca = 1.;
            vec3 h = vec3(0,0,0);
            vec3 off = vec3(0,1,0)*h;
            vec3 nor = normalize(uv);
            vec3 pos = nor + sign(uv)*h;
            V[i] = { (pos+off)*sca, nor };
        }
        firstIndex = F.size();
        baseVertex = V.size();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, F.size() * sizeof F[0], F.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, V.size() * sizeof V[0], V.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);
    }

    int success = loadTexture(tex, "../Metal.png");
    if (success)
    {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, tex);
    }

    glPointSize(3.0);
    glLineWidth(1.0);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, C.size() * sizeof C[0], C.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, ibo);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
    for (size_t off=0, i=3; i<8; i++, off+=12)
    {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, sizeof(Instance), (void*)off);
        glVertexAttribDivisor(i, 1);
    }

    return { C.size(), cbo, 0, 0 };
}

extern "C" void mainAnimation(vector<vec3>& lines, float t)
{
    vec3 ta = vec3(0,1,0);
    float m = sin(t*3.0)*0.17 + 1.2;
    vec3 ro = ta + vec3(sin(m),.5,cos(m))*1.5f;
    float data[] = { ro.x,ro.y,ro.z, 1.2, ta.x,ta.y,ta.z, 0 };
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(vec4), sizeof data , data);

    vector<vec3> world(Joint_Max, vec3(0));
    vector<mat3> local(Joint_Max, mat3(1));

    for (int i : {
         Wrist_R,
         Wrist_L,
         Foot_R,
         Foot_L,})
    {
        world[i] = joints[i];
        world[gSkeleton[i].parentId] = vec3(1,0,0);
    }

    world[Hips] = joints[Hips]-.2f*(sin(t)*.5f+.5f) * vec3(0,1,0);
    world[Wrist_R] = joints[Wrist_R]*.8f;
    world[Elbow_R] = vec3(0,-1,0);
    world[Wrist_L] = joints[Wrist_L]*.8f;
    world[Elbow_L] = vec3(0,1,0);

    // rig
    for (int i : {
        // Hips,
        Spine1,
        Spine2,
        Spine3,
        Neck,
        Head,
        Head_End,
        Shoulder_R,
        Shoulder_L,
        Leg_R,
        Leg_L, })
    {
        int p = gSkeleton[i].parentId;
        world[i] = world[p] + gSkeleton[i].local * local[p];
        local[i] *= local[p];
    }

    for (int i : {
         Wrist_R,
         Wrist_L,
         Foot_R,
         Foot_L,
    })
    {
        int x = gSkeleton[i].parentId;
        int o = gSkeleton[x].parentId;
        vec3 dir = world[x];
        float r1 = length(gSkeleton[x].local);
        float r2 = length(gSkeleton[i].local);
        world[x] = world[o] + solve(world[i]-world[o], r1, r2, dir);

        float ir2 = 1. / dot(gSkeleton[i].local, gSkeleton[i].local);
        local[i] = rotationAlign(world[i]-world[x], gSkeleton[i].local) * ir2;
    }

    for (int i : {
        Toe_R,Toe_L,Hand_R,Hand_L,
    })
    {
        int p = gSkeleton[i].parentId;
        world[i] = world[p] + gSkeleton[i].local * local[p];
        local[i] *= local[p];
    }

    vector<Instance> I;
    for (int i=Root; i<Joint_Max; i++)
    {
        int objectId = gSkeleton[i].objectId;
        if (Null == objectId) { continue; }

        int parentId = gSkeleton[i].parentId;

        float w = .2 + hash11(i + 349) * .1;
            w *= 1 - (i>Shoulder_R || parentId == Neck) * .5;

        vec3 local = world[i] - world[parentId];
        float r = length(local);
        mat3 swi = rotationAlign(local/r, vec3(0,1,0));
        I << Instance{ vec3(w, r, w), world[parentId], swi };
    }

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

    C[1].instanceCount = I.size();
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo);
    glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, C.size() * sizeof C[0], C.data());

    lines << Aabb{ vec3(0,1.2,0), vec3(.2,1.5,.2) };
    lines << BoundingSphere{ vec3(1.,1.2,0), .2 };
}
