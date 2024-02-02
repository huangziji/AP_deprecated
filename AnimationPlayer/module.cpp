#include <stdio.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <boost/container/vector.hpp>
using boost::container::vector;
using namespace glm;

/// @link https://www.shadertoy.com/view/4djSRW
float hash11(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

/// @link https://iquilezles.org/articles/noacos/
mat3x3 rotationAlign( vec3 d, vec3 z )
{
    vec3  v = cross( z, d );
    float c = dot( z, d );
    float k = 1.0f/(1.0f+c);

    return mat3x3( v.x*v.x*k + c,     v.y*v.x*k - v.z,    v.z*v.x*k + v.y,
                   v.x*v.y*k + v.z,   v.y*v.y*k + c,      v.z*v.y*k - v.x,
                   v.x*v.z*k - v.y,   v.y*v.z*k + v.x,    v.z*v.z*k + c    );
}

/// @link https://iquilezles.org/articles/simpleik/
vec3 solve( vec3 p, float r1, float r2, vec3 dir )
{
        vec3 q = p*( 0.5f + 0.5f*(r1*r1-r2*r2)/dot(p,p) );

        float s = r1*r1 - dot(q,q);
        s = max( s, 0.0f );
        q += sqrt(s)*normalize(cross(p,dir));

        return q;
}

mat3 rotateX(float a)
{
    float c=cos(a), s=sin(a);
    return mat3(1, 0, 0,
                0, c, s,
                0, -s, c);
}

mat3 rotateY(float a)
{
    float c=cos(a), s=sin(a);
    return mat3(c, 0, s,
                0, 1, 0,
                -s, 0, c);
}

mat3 rotateZ(float a)
{
    float c=cos(a), s=sin(a);
    return mat3(c, s, 0,
                -s, c, 0,
                0, 0, 1);
}

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

template<class T> static vector<T> &operator<<(vector<T> &a, T const& b) { a.push_back(b); return a; }
template<class T> static vector<T> &operator, (vector<T> &a, T const& b) { return a << b; }
int loadTexture(GLuint tex, const char *filename);
vector<Command> InitGeometry(GLuint &vbo, GLuint &ebo);

static vector<Node> gSkeleton;
static GLuint ibo, vbo, ebo, cbo, tex, frame = 0;
static vector<Command> C = InitGeometry(vbo, ebo);
extern "C" ivec4 mainGeometry()
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
        glGenBuffers(1, &ibo);
        glGenBuffers(1, &cbo);
        glGenTextures(1, &tex);
    }

    int success = loadTexture(tex, "../gold2.png");
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

    return { C.size(), cbo, ebo, 0 };
}

extern "C" void mainAnimation(float t)
{
    vec3 ta = vec3(0,1,0);
    float m = sin(t*3.0)*0.17 + 1.2;
    vec3 ro = ta + vec3(sin(m),.5,cos(m))*1.5f;
    float data[] = { ro.x,ro.y,ro.z, 1.2, ta.x,ta.y,ta.z, 0 };
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(vec4), sizeof data , data);

    vector<vec3> world(Joint_Max, vec3(0));
    vector<mat3> local(Joint_Max, mat3(1));
//    local[Hips] = rotateY(t);
//    world[Hips] = joints[Hips] + vec3(0, -(sin(t)+1.)*.1, 0);

    // Dispatch
    for (int i=1; i<Joint_Max; i++)
    {
        int p = gSkeleton[i].parentId;
        world[i] = world[p] + gSkeleton[i].local * local[p];
        local[i] *= local[p];
    }

    {
        const float keys[] = {
            -.2, -.2,-.1,0,.15,.2,.15,0,-.1,-.2, -.2,
        };

        float z = spline( keys, sizeof keys/sizeof *keys, fract(t) );
        world[Foot_R] = joints[Foot_R] + vec3(0, z+.21, 0 );

        int id = Foot_R;
        vec3 dir = vec3(1,0,0);
        int x = gSkeleton[id].parentId;
        int o = gSkeleton[x].parentId;
        float r1 = length(gSkeleton[x].local);
        float r2 = length(gSkeleton[id].local);
        world[x] = world[o] + solve(world[id]-world[o], r1, r2, dir);

        float ir2 = 1. / dot(gSkeleton[id].local, gSkeleton[id].local);
        local[id] = rotationAlign(world[id]-world[x], gSkeleton[id].local) * ir2;
        world[Toe_R] = world[id] + gSkeleton[Toe_R].local * local[id];
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

    C[1].instanceCount = I.size();

    int size1;
    int size2 = I.size() * sizeof I[0];
    glBindBuffer(GL_ARRAY_BUFFER, ibo);
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size1);
    if (size1 < size2)
    {
        glBufferData(GL_ARRAY_BUFFER, size2, I.data(), GL_DYNAMIC_DRAW);
    }
    else
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, size2, I.data());
    }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo);
    glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, C.size() * sizeof C[0], C.data());
}
