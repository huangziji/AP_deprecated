#include <stdio.h>
#include <assert.h>
#include <boost/container/vector.hpp>
using boost::container::vector;
#include <glm/glm.hpp>
using namespace glm;

mat3x3 rotationAlign( vec3 d, vec3 z );

/************************************************************
 *                        Lines                             *
************************************************************/

static const vec3 Verts[] = {
    {0,0,0},{1,0,0},{0,1,0},{1,1,0},
    {0,0,1},{1,0,1},{0,1,1},{1,1,1},
};

static const ivec2 Edges[] = {
    {0,1},{2,3},{4,5},{6,7},
    {0,2},{1,3},{4,6},{5,7},
    {0,4},{1,5},{2,6},{3,7},
};

static const mat3x3 Faces[] = {
    { 0,0,1,  0,1,0,  -1,0,0, }, // +x
    { 0,0,-1, 0,1,0,  1,0,0,  }, // -x
    { 1,0,0,  0,0,1,  0,-1,0, }, // +y
    { 1,0,0,  0,0,-1, 0,1,0,  }, // -y
    { -1,0,0, 0,1,0,  0,0,-1, }, // +z
    { 1,0,0,  0,1,0,  0,0,1,  }, // -z
};

void lBox(vector<vec3> & V, mat3 rot, vec3 pos)
{
    int N = sizeof Edges / sizeof *Edges;
    for (int i=0; i<N; i++)
    {
        vec3 p1 = Verts[Edges[i][0]] * 2.f - 1.f;
        vec3 p2 = Verts[Edges[i][1]] * 2.f - 1.f;
        V.push_back( p1 * rot + pos );
        V.push_back( p2 * rot + pos );
    }
}

void lCircle(vector<vec3> & V, vec3 ce, float r, vec3 dir)
{
    const mat3 rot = rotationAlign(dir, vec3(0,0,1));

    const int N = 32;
    const float f = 2.0f*M_PI/(float)N;
    const float a = cos(f);
    const float b = sin(f);

    float s = 0.0;
    float c = 1.0;
    for (int i=0; i<N; i++)
    {
        const float ns = b*c + a*s;
        const float nc = a*c - b*s;

        // do something with s and c
        vec3 p1 = vec3( c, s,0);
        vec3 p2 = vec3(nc,ns,0);
        V.push_back(p1 * rot * r + ce);
        V.push_back(p2 * rot * r + ce);

        s = ns;
        c = nc;
    }
}

void lSphere(vector<vec3> & V, vec3 ce, float r)
{
    lCircle(V, ce, r, vec3(1,0,0));
    lCircle(V, ce, r, vec3(0,1,0));
    lCircle(V, ce, r, vec3(0,0,1));
}

void lCapsule(vector<vec3> & V, vec3 pa, vec3 pb, float r)
{
    vec3 ba = pb - pa;
    float lba = length(ba);
    lCircle(V, pa, r, ba/lba);
    lCircle(V, pb, r, ba/lba);

    mat3 rot = rotationAlign(ba/lba, vec3(0,1,0));

    const int N = 32;
    const float f = 2.0f*M_PI/(float)N;
    const float a = cos(f);
    const float b = sin(f);

    for (int i : { 0, 4 })
    {
        float s = 0.0f;
        float c = 1.0f;
        for( int n=0; n<N; n++ )
        {
            const float ns = b*c + a*s;
            const float nc = a*c - b*s;

            // do something with s and c
            vec3 p1 = vec3( c,  s, 0);
            vec3 p2 = vec3(nc, ns, 0);
            float r1 = 1 - step(p1.y, 0.f);
            float r2 = 1 - step(p2.y, 0.f);
            V.push_back((p1 * r + vec3(0,lba * r1,0)) * Faces[i] * rot + pa );
            V.push_back((p2 * r + vec3(0,lba * r2,0)) * Faces[i] * rot + pa );

            c = nc;
            s = ns;
        }
    }
}

/************************************************************
 *                         Triangles                        *
************************************************************/

typedef struct { vec3 pos, nor; }Vertex;
typedef unsigned short Index;

void tCubeMap(vector<Vertex> &V, vector<Index> &F, int N)
{
    size_t baseVertex = V.size();
    for (int i=0; i<6; i++)
        for (int t=0; t<N; t++)
            for (int s=0; s<N; s++)
    {
        vec3 uv = vec3(vec2(s,t)/(N-1.0f)*2.0f-1.0f, 1) * Faces[i];
        int idx = V.size() - baseVertex;
        V.push_back({ uv, Faces[i][2]*vec3(-1,-1,1) });
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
}

void tCapsule(vector<Vertex> &V, vector<Index> &F, float t)
{
    vec3 h = vec3(0,t,0);
    size_t baseVertex = V.size();
    tCubeMap(V, F, 32);
    for (size_t i=baseVertex; i<V.size(); i++)
    {
        vec3 uv = V[i].pos;
        vec3 nor = normalize(uv);
        vec3 pos = nor + sign(uv)*h;
        V[i] = { pos, nor };
    }
}

/************************************************************
 *                       Processing                         *
************************************************************/

struct Slice
{
    float *data;
    const size_t length;
    const size_t stride;

    vec3 *begin() { return (vec3*)(data); }
    vec3 *end() { return (vec3*)(data + length * stride); }
};

template <class T> Slice sliced(vector<T> & arr)
{
    size_t stride = sizeof arr[0]/sizeof(float);
    return{ (float*)arr.data(), arr.size(), stride };
}

void opElongate(Slice arr, vec3 h)
{
    for (vec3 & pos : arr)
    {
        pos += sign(pos) * h;
    }
}

/************************************************************
 *                       Utilities                          *
************************************************************/


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
    return mat3( 1, 0, 0,
                 0, c, s,
                 0,-s, c);
}

mat3 rotateY(float a)
{
    float c=cos(a), s=sin(a);
    return mat3( c, 0, s,
                 0, 1, 0,
                -s, 0, c);
}

mat3 rotateZ(float a)
{
    float c=cos(a), s=sin(a);
    return mat3( c, s, 0,
                -s, c, 0,
                 0, 0, 1);
}

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
