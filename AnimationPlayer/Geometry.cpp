#include <glad/glad.h>
#include <glm/glm.hpp>
#include <boost/container/vector.hpp>
using boost::container::vector;
using namespace glm;

typedef struct {
    vec3 pos, nor;
}Vertex;

typedef struct {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
}Command;

static Command genCubeMap(vector<Vertex> &V, vector<ushort> &F, uint N)
{
    const mat3x3 Rots[] = {
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

template<class T> static vector<T> &operator<<(vector<T> &a, T const& b) { a.push_back(b); return a; }
template<class T> static vector<T> &operator, (vector<T> &a, T const& b) { return a << b; }

vector<Command> InitGeometry()
{
    vector<Command> C;
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

    GLuint ebo, vbo;
    glGenBuffers(1, &ebo);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, F.size() * sizeof F[0], F.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, V.size() * sizeof V[0], V.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);
    return C;
}

static void Circle(vector<vec3> &V, int N)
{
    vector<vec3> verts;
    vector<ivec2> edges;
    for (int i=0; i<N; i++)
    {
        float t = i / float(N-1) * M_PI * 2.;
        float c=cos(t), s=sin(t);
        verts.push_back( vec3(c,s,0) );
        edges.push_back( ivec2(i,(i+1)%N) );
    }
    for (size_t i=0; i<edges.size(); i++)
    {
        V.push_back(verts[edges[i][0]]);
        V.push_back(verts[edges[i][1]]);
    }
}

static void Cube(vector<vec3> &V)
{
    vec3 verts[] = {
        {0,0,0},{1,0,0},{0,1,0},{1,1,0},
        {0,0,1},{1,0,1},{0,1,1},{1,1,1},
    };
    ushort edges[][2] = {
        {0,1},{2,3},{4,5},{6,7},
        {0,2},{1,3},{4,6},{5,7},
        {0,4},{1,5},{2,6},{3,7},
    };
    int N = sizeof edges / sizeof *edges;
    for (int i=0; i<N; i++)
    {
        V.push_back(verts[edges[i][0]]*2.f-1.f);
        V.push_back(verts[edges[i][1]]*2.f-1.f);
    }
}

static void Octahedron(vector<vec3> &V)
{
    vector<vec3> verts;
    const ushort edges[][2] = {
        {0,1},{0,2},{0,3},
        {1,2},{2,3},{3,1},
        {4,1},{4,2},{4,3},
    };

    verts.push_back(vec3(0,-1,0));
    for (int i=0; i<3; i++)
    {
        float t = i / 2.;
        verts.push_back(vec3(cos(t), 0, sin(t)));
    }
    verts.push_back(vec3(0,1,0));

    int N = sizeof edges / sizeof *edges;
    for (int i=0; i<N; i++)
    {
        V.push_back(verts[edges[i][0]]);
        V.push_back(verts[edges[i][1]]);
    }
}

vector<Command> InitGizmo()
{
    vector<Command> C;
    vector<vec3> U;

    uint baseVertex = 0;
    Cube(U);
    C.push_back({ (uint)U.size()-baseVertex, 0, baseVertex, 0 });
    baseVertex = U.size();
    Circle(U, 32);
    C.push_back({ (uint)U.size()-baseVertex, 0, baseVertex, 0 });
    baseVertex = U.size();

    GLuint vbo2;
    glGenBuffers(1, &vbo2);
    glBindBuffer(GL_ARRAY_BUFFER, vbo2);
    glBufferData(GL_ARRAY_BUFFER, U.size() * sizeof U[0], U.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);

    return C;
}
