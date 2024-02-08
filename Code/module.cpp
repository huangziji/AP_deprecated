#include "common.h"
#include <stdio.h>
using glm::outerProduct;

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
        V.push_back({ uv, cubemap[i][2]*vec3(-1,-1,1) });
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

    return { (uint)F.size()-firstIndex, 0, firstIndex, baseVertex, 0 };
}

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

vector<Command> mainGeometry()
{
    vector<Vertex> V;
    vector<ushort> F;
    vector<Command> C;

    return C;
}

typedef mat4x3 Obb;
typedef mat4x3 Transform;
vector<vec3> &operator<<(vector<vec3> &a, mat4x3 const& b);

typedef struct {
    vec3 ro;
    float _pad1;
    vec3 ta;
    float _pad2;
    vector<vec3> line;
    vector<Command> cmd;
    vector<Instance> xform;
    vector<Vertex> vertex;
    vector<ushort> index;
}Output;

extern "C" void mainAnimation(Output &out, float t)
{
    static uint frame = 0;
    if (!frame++)
    {
        uint firstIndex = 0;
        uint baseVertex = 0;
        out.cmd << genCubeMap(out.vertex, out.index, 2);
        firstIndex = out.vertex.size();
        baseVertex = out.index.size();
    }

    out.ta = vec3(0,1,0);
    float m = sin(t*3.0)*0.17 + .5;
    out.ro = out.ta + vec3(sin(m),.5,cos(m))*1.5f;

    out.line.clear();
    out.xform.clear();

    out.line << Aabb{ vec3(-.2,1.,-1.2), vec3(-.2,1.,-1.2)+vec3(.4) };
    mat3 rot = rotateY(t)*.2f;
    out.line << mat4x3{ rot[0], rot[1], rot[2], vec3(0,1.2,-1.) };
    out.line << BoundingSphere{ vec3(0,1.5,0.6), .05 };

    extern const vec3 joints[];
    extern const IkRig parentTable[];
    extern const bool hasMesh[];

    vector<vec3> gSkeleton;
    gSkeleton << vec3(0);
    for (int i=Hips; i<Joint_Max; i++)
    {
        int parentId = parentTable[i];
        gSkeleton << joints[i]-joints[parentId];
    }

    vector<vec3> world(Joint_Max, vec3(0));
    vector<mat3> local(Joint_Max, mat3(1));
    local[Hips] = rotateY(t);

    for (int i=Hips; i<Joint_Max; i++)
    {
        int p = parentTable[i];
        world[i] = world[p] + gSkeleton[i] * local[p];
        local[i] *= local[p];
    }


#if 0

    for (int i : {
         Wrist_R,
         Wrist_L,
         Foot_R,
         Foot_L,})
    {
        world[i] = joints[i];
        world[parentTable[i]] = vec3(1,0,0);
    }

    world[Hips] = joints[Hips]-.2f*(sin(t)*.5f+.5f) * vec3(0,1,0);
    world[Wrist_R] = joints[Wrist_R]*.8f;
    world[Elbow_R] = vec3(0,-1,0);
    world[Wrist_L] = joints[Wrist_L]*.8f;
    world[Elbow_L] = vec3(0,1,0);

    // rig
    for (int i : {
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
        int p = parentTable[i];
        world[i] = world[p] + gSkeleton[i] * local[p];
        local[i] *= local[p];
    }

    for (int i : {
         Wrist_R,
         Wrist_L,
         Foot_R,
         Foot_L,
    })
    {
        int x = parentTable[i];
        int o = parentTable[x];
        vec3 dir = world[x];
        float r1 = length(gSkeleton[x]);
        float r2 = length(gSkeleton[i]);
        world[x] = world[o] + solve(world[i]-world[o], r1, r2, dir);

        float ir2 = 1. / dot(gSkeleton[i], gSkeleton[i]);
        local[i] = rotationAlign(world[i]-world[x], gSkeleton[i]) * ir2;
    }

    for (int i : {
        Toe_R,Toe_L,Hand_R,Hand_L,
    })
    {
        int p = parentTable[i];
        world[i] = world[p] + gSkeleton[i] * local[p];
        local[i] *= local[p];
    }
#endif


    for (int i=Root; i<Joint_Max; i++)
    {
        if (!hasMesh[i]) { continue; }

        int parentId = parentTable[i];

        float w = .2 + hash11(i + 349) * .1;
            w *= 1 - (i>Shoulder_R || parentId == Neck) * .5;

        float r = length(world[i] - world[parentId]);
        mat3 rot = rotationAlign(gSkeleton[i]/r, vec3(0,0,1));
        vec3 sca = abs(rot * vec3(w, w, r)) * .5f;

        mat3 swi = matrixCompMult(local[i], mat3(sca,sca,sca));

        vec3 ce = mix(world[i], world[parentId], .5f);
        out.xform << Instance{ swi, ce };
        out.line << mat4x3{ swi[0]*1.02f, swi[1]*1.02f, swi[2]*1.02f, ce };
    }

    out.cmd[0].instanceCount = out.xform.size();
}
