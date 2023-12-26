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

static vector<GLushort> &operator<<(vector<GLushort> &a, GLushort b)
{
    a.push_back(b);
    return a;
}

static vector<GLushort> &operator,(vector<GLushort> &a, GLushort b)
{
    return a << b;
}

typedef struct {
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    GLint baseVertex;
    GLuint baseInstance;
}DrawElementsIndirectCommand;

typedef struct {
    vec3 position, normal;
}Vertex;

typedef vec3 Instance;

// c++ class isn't working with hot reloading, so I use C style OO instead
typedef struct {
    vector<Vertex> V;
    vector<GLushort> I;
    vector<Instance> X;
    vector<DrawElementsIndirectCommand> cmd;
} PolygonContext;

void pctx_Cylinder(PolygonContext &self, int N, float h, float s)
{
    auto &V = self.V;
    auto &I = self.I;
    auto &X = self.X;
    auto &cmd = self.cmd;
    int baseVertex = V.size();
    uint firstIndex = I.size();
    uint baseInstance = X.size();
    V.push_back({ vec3(0, h*s,0), vec3(0, 1,0) });
    V.push_back({ vec3(0,-h*s,0), vec3(0,-1,0) });
    for (int i=0; i<=N; i++) {
        float  t = 2*M_PI*i/N;
        vec3 n = vec3(sin(t),0,cos(t));
        vec3 a = (n + vec3(0, h,0)) * s;
        vec3 b = (n + vec3(0,-h,0)) * s;
        V.push_back({ a, n });
        V.push_back({ b, n });
        V.push_back({ a, vec3(0, 1,0) });
        V.push_back({ b, vec3(0,-1,0) });
        if (i != N) {
            int M = 4, idx = i*M + 2;
            I << idx,idx+1,idx+M, idx+M,idx+1,idx+1+M;
            I << 0,  idx+2,idx+M+2, 1, idx+M+3, idx+3;
        }
    }
    uint count = I.size()-firstIndex;
    cmd.push_back({ count, 0, firstIndex, baseVertex, baseInstance });
}

void pctx_CubeMap(PolygonContext &self, uint N, Vertex fun(vec3))
{
    auto &V = self.V;
    auto &I = self.I;
    auto &X = self.X;
    auto &cmd = self.cmd;
    int baseVertex = V.size();
    uint firstIndex = I.size();
    uint baseInstance = X.size();
    static const mat3x3 ToCubeMapSpace[] = {
        { 1,0,0, 0,1,0, 0,0,1, }, // forward
        { -1,0,0, 0,1,0, 0,0,-1, }, // back
        { 0,0,1, 0,1,0, -1,0,0, }, // right
        { 0,0,-1, 0,1,0, 1,0,0, }, // left
        { 1,0,0, 0,0,1, 0,-1,0, }, // top
        { 1,0,0, 0,0,-1, 0,1,0, }, // bottom
    };
    for (int f=0; f<6; f++)
        for (int t=0; t<N; t++)
            for (int s=0; s<N; s++)
    {
        vec3 uv = vec3(vec2(s,t)/(N-1.0f)*2.0f-1.0f, 1) * ToCubeMapSpace[f];
        V.push_back(fun(uv));
        if (s != N-1 && t != N-1) {
            int idx = f*N*N + t*N + s;
            I << idx,idx+1,idx+N,idx+N,idx+1,idx+1+N;
        }
    }
    uint count = I.size()-firstIndex;
    cmd.push_back({ count, 0, firstIndex, baseVertex, baseInstance });
}

void pctx_Normal(PolygonContext &self)
{
    auto      & V   = self.V;
    auto const& I   = self.I;
    auto const& cmd = self.cmd;
    uint firstIndex = cmd.back().firstIndex;
    int baseVertex = cmd.back().baseVertex;
    for (int i=baseVertex; i<V.size(); i++) {
        V[i].normal = vec3(0);
    }
    for (int i=firstIndex; i<I.size(); i+=3) {
        int i0 = I[i+0],
            i1 = I[i+1],
            i2 = I[i+2];
        vec3 p0 = V[baseVertex+i0].position;
        vec3 p1 = V[baseVertex+i1].position;
        vec3 p2 = V[baseVertex+i2].position;
        vec3 nor = cross(p1-p0, p2-p1);
        V[baseVertex+i0].normal += nor;
        V[baseVertex+i1].normal += nor;
        V[baseVertex+i2].normal += nor;
    }
    for (int i=baseVertex; i<V.size(); i++) {
        V[i].normal = normalize(V[i].normal);
    }
}

int pctx_End(PolygonContext &self)
{
    auto &V = self.V;
    auto &I = self.I;
    auto &X = self.X;
    auto &cmd = self.cmd;
    size_t offset = V.size() * sizeof V[0];
    size_t size = X.size() * sizeof X[0];
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 12, (void*)offset);
    glBufferData(GL_ARRAY_BUFFER, offset+size, V.data(), GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, X.data());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, I.size()*sizeof I[0], I.data(), GL_STATIC_DRAW);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, cmd.size()*sizeof cmd[0], cmd.data(), GL_STATIC_DRAW);
    return cmd.size();
}

PolygonContext &operator<<(PolygonContext &ctx, vec3 pos)
{
    ctx.X.push_back(pos);
    ctx.cmd.back().instanceCount++;
    return ctx;
};

PolygonContext &operator,(PolygonContext &ctx, vec3 pos)
{
    return ctx << pos;
};

Vertex CubeMap2Cube(vec3 uv)
{
    return {uv*.1f,uv};
}

Vertex CubeMap2Sphere(vec3 uv)
{
    vec3 pos = normalize(uv);
    return {pos*.15f,pos};
}

Vertex CubeMap2Capsule(vec3 uv)
{
    float h = 4.;
    vec3 nor = normalize(uv);
    vec3 pos = sign(uv.y)*vec3(0,h,0) + nor;
    return {pos*.1f, nor};
}

extern "C" int mainGeometry()
{
    PolygonContext ctx;
    pctx_CubeMap(ctx, 2, CubeMap2Cube);
    pctx_Normal(ctx);
    ctx << vec3(0,0,0);
    pctx_CubeMap(ctx, 10, CubeMap2Sphere);
    ctx << vec3(1,0,0), vec3(0,0,1), vec3(0,-1,0);
    pctx_CubeMap(ctx, 8, CubeMap2Capsule);
    ctx << vec3(-1,0,0);
    pctx_Cylinder(ctx, 10, 4, .1);
    ctx << vec3(0,0,-1);
    return pctx_End(ctx);
}

extern "C" void mainAnimation(float t)
{
//    t = 1.;
    vec3 ta = vec3(0, sin(t), 0)*0.f;
    vec3 ro = ta + vec3(sin(t),.4,cos(t))*2.f;
    mat2x4 data = mat2x4(vec4(ro, 1.2), vec4(ta, 0));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(vec4), sizeof(data), &data);
}
