#include <stdio.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <boost/container/vector.hpp>
using boost::container::vector;
using namespace glm;

float hash11(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

mat3x3 rotationAlign( vec3 d, vec3 z )
{
    vec3  v = cross( z, d );
    float c = dot( z, d );
    float k = 1.0f/(1.0f+c);

    return mat3x3( v.x*v.x*k + c,     v.y*v.x*k - v.z,    v.z*v.x*k + v.y,
                   v.x*v.y*k + v.z,   v.y*v.y*k + c,      v.z*v.y*k - v.x,
                   v.x*v.z*k - v.y,   v.y*v.z*k + v.x,    v.z*v.z*k + c    );
}

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

static const int NullIndex = -1;

typedef struct {
    vec3 dir;
    int boneIdx1, boneIdx2, boneIdx3;
    float r1, r2;
}SimpleIk;

typedef struct {
    vec3 local;
    int objectIndex;
    int parentIndex;
}Node;

typedef enum {
    Root,
    Hips,
    Neck,
    Head,
    Shoulder_L, Shoulder_R,
    Wrist_L, Wrist_R,
    Leg_L, Leg_R,
    Foot_L, Foot_R,

    Spine1,Spine2,Head_Top,
    Knee_L,Knee_R,
    Toe_L,Toe_R,
    Elbow_L,Elbow_R,
    Hand_L,Hand_R,
}IkRig;

template<class T> vector<T> &operator<<(vector<T> &a, T const& b) { a.push_back(b); return a; }
template<class T> vector<T> &operator, (vector<T> &a, T const& b) { return a << b; }
int loadTexture(GLuint tex, const char *filename);
vector<Command> InitGeometry();

extern "C" ivec4 mainGeometry()
{
    vector<vec3> joints;
    vector<ivec2> bones;

    const float heaHei = 1.75, shdHei = 1.6, hipHei = 1.1;
    joints <<
        vec3(0),
        vec3(0,hipHei,0),
        vec3(0,shdHei+.1,0),
        vec3(0,heaHei,0),
        vec3(-.15,shdHei,0),vec3(.15,shdHei,0),
        vec3(-.7,shdHei,0),vec3(.7,shdHei,0),
        vec3(-.15,hipHei,0),vec3(.15,hipHei,0),
        vec3(-.15,0,0),vec3(.15,0,0),

        mix(joints[Hips], joints[Neck], .33), // Spine1
        mix(joints[Hips], joints[Neck], .66), // Spine2
            joints[Head] + vec3(0,.2,0), // Top
        mix(joints[Leg_L], joints[Foot_L], .5) + vec3(0,0,.001), // Knee_L
        mix(joints[Leg_R], joints[Foot_R], .5) + vec3(0,0,.001), // Knee_R
            joints[Foot_L] + vec3(0,0,.2), // Toe_L
            joints[Foot_R] + vec3(0,0,.2), // Toe_R
        mix(joints[Shoulder_L], joints[Wrist_L], .5), // Elbow_L
        mix(joints[Shoulder_R], joints[Wrist_R], .5), // Elbow_R
            joints[Wrist_L] + vec3(-.1,0,0), // Hand_L
            joints[Wrist_R] + vec3(.1,0,0); // Hand_R


    bones <<
        ivec2(Root, Hips),
        ivec2(Hips, Spine1),
        ivec2(Spine1, Spine2),
        ivec2(Spine2, Neck),
        ivec2(Neck, Head),
        ivec2(Head, Head_Top),

        ivec2(Hips, Leg_L),
        ivec2(Leg_L, Knee_L),
        ivec2(Knee_L, Foot_L),
        ivec2(Foot_L, Toe_L),
        ivec2(Hips, Leg_R),
        ivec2(Leg_R, Knee_R),
        ivec2(Knee_R, Foot_R),
        ivec2(Foot_R, Toe_R),

        ivec2(Spine2, Shoulder_L),
        ivec2(Shoulder_L, Elbow_L),
        ivec2(Elbow_L, Wrist_L),
        ivec2(Wrist_L, Hand_L),
        ivec2(Spine2, Shoulder_R),
        ivec2(Shoulder_R, Elbow_R),
        ivec2(Elbow_R, Wrist_R),
        ivec2(Wrist_R, Hand_R);

    static vector<Command> C = InitGeometry();
    vector<Instance> I;
    for (int i=0; i<bones.size(); i++)
    {
        ivec2 b = bones[i];
        if (b.x == Root ||
            b.y == Shoulder_L ||
            b.y == Shoulder_R ||
            b.y == Leg_L ||
            b.y == Leg_R) continue;

        float w = .1 + hash11(i + 154) * .05;
        if (i > 5) w *= .5;
        w *= 1. - (b.x == Neck) * .5;
        w *= 1. + (b.y == Neck) * .5;

        vec3 local = joints[b.y]-joints[b.x];
        float r = length(local);
        mat3 swi = rotationAlign(local, vec3(0,1/r,0));
        I << Instance{ vec3(w,.5*r,w), joints[b.x], swi };
        C[1].instanceCount ++;
    }

//    I << Instance{ vec3(.3), vec3(1,0,0), mat3(1) };
//    C[1].instanceCount++;

    static GLuint ibo, cbo1, cbo2, tex, frame = 0;
    if (!frame++)
    {
        glGenBuffers(1, &ibo);
        glGenBuffers(1, &cbo1);
        glGenBuffers(1, &cbo2);
        glGenTextures(1, &tex);
    }

    int success = loadTexture(tex, "../T_Satin_J.png");
    if (success)
    {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, tex);
    }

    glPointSize(3.0);
    glLineWidth(1.0);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo1);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, C.size() * sizeof C[0], C.data(), GL_STATIC_DRAW);
//    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo2);
//    glBufferData(GL_DRAW_INDIRECT_BUFFER, D.size() * sizeof D[0], D.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, ibo);
    glBufferData(GL_ARRAY_BUFFER, I.size() * sizeof I[0], I.data(), GL_STATIC_DRAW);
    for (size_t off=0, i=3; i<8; i++, off+=12)
    {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, sizeof(Instance), (void*)off);
        glVertexAttribDivisor(i, 1);
    }

    return { C.size(), cbo1, 0, 0 };
}

extern "C" void mainAnimation(float t)
{
    //t = sin(t*3.0)*0.17 + 0.1;
    vec3 ta = vec3(0,1,0);
    float m = t;
    vec3 ro = ta + vec3(sin(m),.5,cos(m))*1.5f;
    float data[] = { ro.x,ro.y,ro.z, 1.2, ta.x,ta.y,ta.z, 0 };
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(vec4), sizeof data , data);
}
