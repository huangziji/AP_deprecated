#include <stdio.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
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

float sdBox( vec3 p, float b )
{
    vec3 q = abs(p) - b;
    return length(max(q, 0.f));
}

float sdTorus( vec3 p, vec2 t )
{
    vec2 q = vec2(length(vec2(p.x,p.z))-t.x,p.y);
    return length(q)-t.y;
}

float map(vec3 pos)
{
    float d1, d2;
    d1 = 1e5;
    //d2 = length(pos) - .5;
    //d1 = min(d1, d2);
    d2 = sdBox(pos, .5) - .02;
    d1 = min(d1, d2);
    d2 = sdTorus(pos, vec2(.5,.2));
    d1 = max(-d1, d2);
    return d1;
}

typedef struct {
    vec3 pos, nor;
}Vertex;

typedef struct {
    vec3 off, swi;
}Instance;

typedef struct {
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    GLuint baseVertex;
    GLuint baseInstance;
}Command;

struct Geometry
{
    vector<Vertex> V;
    vector<GLushort> F;
    vector<Instance> I;
    vector<Command> C;
    GLuint vbo, ibo, ebo, cbo;
    GLuint firstIndex = 0, baseVertex = 0;

    void Init()
    {
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);
        glGenBuffers(1, &ebo);
        glGenBuffers(1, &cbo);
    }

    void Clear()
    {
        V.clear();
        F.clear();
        I.clear();
        C.clear();
    }

    void EmitVertex()
    {
        GLuint baseInstance = I.size();
        GLuint count = F.size()-firstIndex;
        C.push_back({ count, 0, firstIndex, baseVertex, baseInstance });
        firstIndex = F.size(), baseVertex = V.size();
    }

    void EndPrimitive()
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, V.size()*sizeof V[0], V.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);

        glBindBuffer(GL_ARRAY_BUFFER, ibo);
        glBufferData(GL_ARRAY_BUFFER, I.size()*sizeof I[0], I.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 24, 0);
        glVertexAttribDivisor(4, 1);
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 24, (void*)16);
        glVertexAttribDivisor(5, 1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, F.size()*sizeof F[0], F.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, C.size()*sizeof C[0], C.data(), GL_STATIC_DRAW);
    }
};

Geometry &operator<<(Geometry &a, Vertex const& b)
{
    a.V.push_back(b);
    return a;
}

Geometry &operator<<(Geometry &a, Instance const& b)
{
    a.I.push_back(b);
    a.C.back().instanceCount += 1;
    return a;
}

Geometry &operator<<(Geometry &a, GLushort b)
{
    a.F.push_back(b);
    return a;
}

Geometry &operator,(Geometry &a, Instance const& b)
{
    return a << b;
};

Geometry &operator,(Geometry &a, Vertex const& b)
{
    return a << b;
};

Geometry &operator,(Geometry &a, GLushort b)
{
    return a << b;
};

// generate normals for last emitted polygon
void GenerateNormals(Geometry &self)
{
    auto &V = self.V;
    auto &F = self.F;
    uint firstIndex = self.firstIndex;
    uint baseVertex = self.baseVertex;
    for (int i=baseVertex; i<V.size(); i++)
    {
        V[i].nor = vec3(0);
    }
    for (int i=firstIndex; i<F.size(); i+=3)
    {
        int i0 = F[i+0],
            i1 = F[i+1],
            i2 = F[i+2];
        vec3 p0 = V[baseVertex+i0].pos,
             p1 = V[baseVertex+i1].pos,
             p2 = V[baseVertex+i2].pos;
        vec3 nor = cross(p1-p0, p2-p1);
        V[baseVertex+i0].nor += nor;
        V[baseVertex+i1].nor += nor;
        V[baseVertex+i2].nor += nor;
    }
    for (int i=baseVertex; i<V.size(); i++)
    {
        V[i].nor = normalize(V[i].nor);
    }
}

int sdf2poly(Geometry &geom, uvec3 id, uint Res)
{
    const int Size = Res+1;
    static vector<GLushort> indexMap(Size*Size*Size);
    const float sca = .8*2/Res;
    const vec3  off = -vec3(.8f);

    static const vec3 vertmap[] = {
            {0,0,0}, {1,0,0}, {0,1,0}, {1,1,0},
            {0,0,1}, {1,0,1}, {0,1,1}, {1,1,1},
    };

    static const uvec2 edgevmap[] = {
            {0,1}, {2,3}, {4,5}, {6,7},
            {0,2}, {1,3}, {4,6}, {5,7},
            {0,4}, {1,5}, {2,6}, {3,7},
    };

    float d[8]; // density
    int mask = 0;
    for (int i=0; i<8; i++)
    {
        d[i] = map( (vec3(id)+vertmap[i])*sca + off );
        mask |= (d[i] > 0) << i;
    }

    if (mask == 0 || mask == 0xff)
        return 0;

    int n_edges = 0;
    vec3 ce = vec3(0);
    for (int i=0; i<12; i++)
    {
        int  a = edgevmap[i][0];
        int  b = edgevmap[i][1];
        int m1 = (mask >> a) & 1;
        int m2 = (mask >> b) & 1;
        if (m1 ^ m2) {
            float t = d[a] / (d[a] - d[b]);
            assert(0 <= t && t <= 1);
            ce += mix(vertmap[a], vertmap[b], t);
            n_edges += 1;
        }
    }
    ce /= n_edges;

    int i0 = id.x + id.y*Size + id.z*Size*Size;
    indexMap[i0] = GLushort(geom.V.size());
    geom << Vertex{ off + sca*(vec3(id)+ce), };

    if (id.x*id.y*id.z == 0)
        return 1;

    const int axismap[] = { 1, Size, Size*Size, 1, Size }; // wrap
    for (int i=0; i<3; i++)
    {
        const int lvl = 1 << i;
        const int m1 = mask & 1;
        const int m2 = (mask >> lvl) & 1;
        if (m1 ^ m2) { // compare to current cell with adjacent cell from each axes
            // flip face depends on the sign of density from lower end
            int i1 = i0 - axismap[i+1+m1],
                i2 = i0 - axismap[i+2-m1],
                i3 = i1 + i2 - i0;
            geom <<
                indexMap[i0], indexMap[i1], indexMap[i2],
                indexMap[i2], indexMap[i1], indexMap[i3];
        }
    }

    return 2;
}

void GenerateCubeMap(Geometry &self, uint N, Vertex f(vec3))
{
    static const mat3x3 ToCubeMapSpace[] = {
        { 1,0,0, 0,1,0, 0,0,1, }, // forward
        { -1,0,0, 0,1,0, 0,0,-1, }, // back
        { 0,0,1, 0,1,0, -1,0,0, }, // right
        { 0,0,-1, 0,1,0, 1,0,0, }, // left
        { 1,0,0, 0,0,1, 0,-1,0, }, // top
        { 1,0,0, 0,0,-1, 0,1,0, }, // bottom
    };

    for (int i=0; i<6; i++)
        for (int t=0; t<N; t++)
            for (int s=0; s<N; s++)
    {
        vec3 uv = vec3(vec2(s,t)/(N-1.f)*2.f-1.f, 1) * ToCubeMapSpace[i];
        GLushort idx = GLushort(self.V.size()-self.baseVertex);
        self << f(uv);
        if (s != N-1 && t != N-1)
        {
            self << idx,idx+1,idx+N, idx+N,idx+1,idx+1+N;
        }
    }
}

Vertex CubeMap2Cube(vec3 uv)
{
    return { uv, };
}

Vertex CubeMap2Sphere(vec3 uv)
{
    vec3 pos = normalize(uv);
    return { pos, pos };
}

Vertex CubeMap2Capsule(vec3 uv)
{
    float h = 4.;
    vec3 nor = normalize(uv);
    vec3 pos = sign(uv.y)*vec3(0,h,0) + nor;
    return { pos, nor };
}

typedef enum {
    Root,
    Hips,
    Neck,
    UpperArmL, UpperArmR,
    LowerArmL, LowerArmR,
    UpperLegL, UpperLegR,
    LowerLegL, LowerLegR,
//    Head,
} IkRig;

struct SimpleIk
{
    int o, p;
    vec3 dir;
    int x;
    float r1, r2;
};

template<class T> vector<T> &operator<<(vector<T> &a, T const& b) { a.push_back(b); return a; }
template<class T> vector<T> &operator, (vector<T> &a, T const& b) { return a << b; }

typedef struct {
    int parentIndex;
    vec3 localPosition;
}Node;

static vector<Node> sceneGraph;

extern "C" int mainGeometry()
{
    vector<vec3> joints;
    vector<int> parents;
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

    parents << 0, 1, 2, 2, 2, 4, 5, 1, 1, 8, 9;

    int idx = 1;
    vector<Instance> I;
    for (auto p : parents)
    {
        sceneGraph << Node{ p, joints[idx++]-joints[p] };
        I << Instance{};
    }


    static GLuint vbo, frame = 0;
    if (!frame++) {
//        glLineWidth(1.0);
        glGenBuffers(1, &vbo);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, I.size() * sizeof I[0], I.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 24, 0);
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    glVertexAttribDivisor(5, 1);

    return I.size();
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


extern "C" int mainAnimation(float t)
{
    //t = sin(t*3.0)*0.17 + 0.1;
    vec3 ta = vec3(0,1,0);
    float m = 1.;//t;
    vec3 ro = ta + vec3(sin(m),.3,cos(m))*1.5f;
    float data[] = { ro.x,ro.y,ro.z, 1.2, ta.x,ta.y,ta.z, 0 };
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(vec4), sizeof data , data);

    vector<mat3> rotations(sceneGraph.size()+1, mat3(1));
    rotations[4] = rotateZ(-1.);
    rotations[5] = rotateZ(1.);

    // fk
    int idx = 1;
    vector<Instance> U;
    vector<vec3> positions(sceneGraph.size()+1, vec3(0));
    for (auto n : sceneGraph)
    {
        int p = n.parentIndex;
        positions[idx] = positions[p] + n.localPosition * rotations[p];
        rotations[idx] = rotations[idx] * rotations[p];
        idx += 1;
        U << Instance{ positions[p], n.localPosition * rotations[p] };
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, U.size() * sizeof U[0], U.data());
    return 0;
}
