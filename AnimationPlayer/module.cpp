#include <stdio.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <bullet/Bullet3Common/b3AlignedAllocator.h>
#include <boost/container/vector.hpp>
using boost::container::vector;
using namespace glm;

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
    vec3 pos, nor;
}Vertex;

typedef struct {
    vec3 pos;
    mat3 rot;
}Instance;

typedef struct {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
}Command;

void GenerateCubeMap(vector<Vertex> &V, vector<GLushort> &F, uint N, Vertex f(vec3))
{
    const mat3x3 Rots[] =
    {
        { 1,0,0, 0,1,0, 0,0,1, }, // forward
        { -1,0,0, 0,1,0, 0,0,-1, }, // back
        { 0,0,1, 0,1,0, -1,0,0, }, // right
        { 0,0,-1, 0,1,0, 1,0,0, }, // left
        { 1,0,0, 0,0,1, 0,-1,0, }, // top
        { 1,0,0, 0,0,-1, 0,1,0, }, // bottom
    };

    uint baseVertex = V.size();
    uint firstIndex = F.size();

    for (int i=0; i<6; i++)
        for (int t=0; t<N; t++)
            for (int s=0; s<N; s++)
    {
        vec3 uv = vec3(vec2(s,t)/(N-1.f)*2.f-1.f, 1) * Rots[i];
        uint idx = V.size() - baseVertex;
        V.push_back(f(uv));
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
}

Vertex Cube(vec3 uv)
{
    vec3 off = vec3(0,1,0);
    vec3 h = vec3(2,3,6)*.15f;
    return{ (uv+off)*h, };
}

Vertex RoundedCube(vec3 uv)
{
    float sca = 1.;
    vec3 h = vec3(0,0,0);
    vec3 off = vec3(0,1,0)*h;
    vec3 nor = normalize(uv);
    vec3 pos = nor + sign(uv)*h;
    return { (pos+off)*sca, nor };
}

void Sphere(vector<vec3> &V, int N)
{
    const mat3 Rots[] = {
        { 1,0,0, 0,1,0, 0,0,1, },
        { 1,0,0, 0,0,1, 0,1,0, },
        { 0,1,0, 1,0,0, 0,0,1, },
    };

    vector<vec3> verts;
    vector<ivec2> edges;
    for (int f=0; f<3; f++)
    {
        for (int i=0; i<N; i++)
        {
            float t = i / float(N-1) * M_PI * 2.;
            float c=cos(t), s=sin(t);
            verts.push_back( vec3(c,0,s)*Rots[f] );
            edges.push_back( ivec2(i,(i+1)%N) + N*f );
        }
    }

    for (size_t i=0; i<edges.size(); i++)
    {
        V.push_back(verts[edges[i][0]]);
        V.push_back(verts[edges[i][1]]);
    }
}

void Cube(vector<vec3> &V)
{
    vec3 verts[] = {
        {0,0,0},{1,0,0},{0,1,0},{1,1,0},
        {0,0,1},{1,0,1},{0,1,1},{1,1,1},
    };
    ivec2 edges[] = {
        {0,1},{2,3},{4,5},{6,7},
        {0,2},{1,3},{4,6},{5,7},
        {0,4},{1,5},{2,6},{3,7},
    };
    int N = sizeof edges / sizeof *edges;
    for (int i=0; i<N; i++)
    {
        V.push_back(verts[edges[i][0]]);
        V.push_back(verts[edges[i][1]]);
    }
}

void Polyhedron(vector<vec3> &V)
{
    const float r = sqrt(3);
    const vec3 verts[] = {
        vec3(0,-1,0),
        vec3(r,0,1)*.5f,
        vec3(-r,0,1)*.5f,
        vec3(0,0,-1),
        vec3(0,1,0),
    };
    const ivec2 edges[] = {
        {0,1},{0,2},{0,3},
        {1,2},{2,3},{3,1},
        {4,1},{4,2},{4,3},
    };
    int N = sizeof edges / sizeof *edges;
    for (int i=0; i<N; i++)
    {
        V.push_back(verts[edges[i][0]]);
        V.push_back(verts[edges[i][1]]);
    }
}

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
    ShoulderL, ShoulderR,
    HandL, HandR,
    LegL, LegR,
    FootL, FootR,
}IkRig;

template<class T> vector<T> &operator<<(vector<T> &a, T const& b) { a.push_back(b); return a; }
template<class T> vector<T> &operator, (vector<T> &a, T const& b) { return a << b; }
GLuint loadTexture(const char *filename);

extern "C" ivec2 mainGeometry()
{
    vector<vec3> joints;

    const float heaHei = 1.75, shdHei = 1.6, hipHei = 1.1;
    joints <<
        vec3(0),
        vec3(0,hipHei,0),
        vec3(0,shdHei,0),
        vec3(0,heaHei,0),
        vec3(-.2,shdHei,0),vec3(.2,shdHei,0),
        vec3(-.7,shdHei,0),vec3(.7,shdHei,0),
        vec3(-.15,hipHei,0),vec3(.15,hipHei,0),
        vec3(-.15,0,0),vec3(.15,0,0);

    vector<Vertex> V;
    vector<GLushort> F;
    vector<Instance> I;
    vector<Command> C;
    uint firstIndex = 0;
    uint baseVertex = 0;
    uint baseInstance = 0;

    GenerateCubeMap(V, F, 10, RoundedCube);
    I << Instance{ vec3(0), mat3(1) };
    C.push_back({ (uint)F.size()-firstIndex,
                  (uint)I.size()-baseInstance,
            firstIndex, baseVertex, baseInstance });
    firstIndex = F.size();
    baseVertex = V.size();
    baseInstance = I.size();

    static GLuint vbo1, vbo2, vbo3, ibo, cbo, frame = 0;
    if (!frame++)
    {
        glGenBuffers(1, &ibo);
        glGenBuffers(1, &cbo);
        glGenBuffers(1, &vbo1);
        glGenBuffers(1, &vbo2);
        glGenBuffers(1, &vbo3);

        GLuint tex = loadTexture("../T_Satin_J.png");
        if (tex)
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, tex);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo1);
    glBufferData(GL_ARRAY_BUFFER, V.size() * sizeof V[0], V.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, F.size() * sizeof F[0], F.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, C.size() * sizeof C[0], C.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo2);
    glBufferData(GL_ARRAY_BUFFER, I.size() * sizeof I[0], I.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Instance), 0);
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Instance), (void*)12);
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(Instance), (void*)24);
    glVertexAttribDivisor(6, 1);
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(Instance), (void*)36);
    glVertexAttribDivisor(7, 1);

    vector<vec3> U;
    Sphere(U, 50);
//    Polyhedron(U);
    glBindBuffer(GL_ARRAY_BUFFER, vbo3);
    glBufferData(GL_ARRAY_BUFFER, U.size() * sizeof U[0], U.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
    glPointSize(3.0);
    glLineWidth(1.0);

    return { (int)C.size(), (int)U.size() };
}

extern "C" void mainAnimation(float t)
{
    //t = sin(t*3.0)*0.17 + 0.1;
    vec3 ta = vec3(0,0.5,0);
    float m = t;
    vec3 ro = ta + vec3(sin(m),.5,cos(m))*2.5f;
    float data[] = { ro.x,ro.y,ro.z, 1.2, ta.x,ta.y,ta.z, 0 };
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(vec4), sizeof data , data);
}
