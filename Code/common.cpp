#include <stdio.h>
#include <assert.h>
#include <boost/container/vector.hpp>
using boost::container::vector;
#include <glm/glm.hpp>
using namespace glm;

/************************************************************
 *                      Draw Gizmo                          *
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

vector<vec3> SineTable(int N)
{
    vector<vec3> out;
    for (int i=0; i<N+1; i++)
    {
        float t = 2 * M_PI * i / (N-1);
        out.push_back(vec3(cos(t),sin(t),0));
    }
    return out;
}

static const vector<vec3> sineTable = SineTable(128);

vector<vec3> &operator<<(vector<vec3> &a, vec4 const& b)
{
    vec3 ce = (vec3)(b);
    float r = b.w;
    const int step = 4;
    const int N = sineTable.size() - 1;
    for (int f : { 0, 2, 4 })
    {
        for (int i=0; i<N; i+=step)
        {
            a.push_back(sineTable[i+   0] * Faces[f] * r + ce);
            a.push_back(sineTable[i+step] * Faces[f] * r + ce);
        }
    }
    return a;
}

vector<vec3> &operator<<(vector<vec3> &a, mat2x3 const& b)
{
    int N = sizeof Edges / sizeof *Edges;
    vec3 d = b[1] - b[0];
    for (int i=0; i<N; i++)
    {
        a.push_back( Verts[Edges[i][0]] * d + b[0] );
        a.push_back( Verts[Edges[i][1]] * d + b[0] );
    }
    return a;
}

typedef struct{
    mat3 orient; vec3 center;
}Obb;

vector<vec3> &operator<<(vector<vec3> &a, mat4x3 const& b)
{
    int N = sizeof Edges / sizeof *Edges;
    for (int i=0; i<N; i++)
    {
        vec3 p1 = Verts[Edges[i][0]] * 2.f - 1.f;
        vec3 p2 = Verts[Edges[i][1]] * 2.f - 1.f;
        a.push_back( p1 * mat3(b) + b[3] );
        a.push_back( p2 * mat3(b) + b[3] );
    }
    return a;
}

/************************************************************
 *                       Utilities                          *
************************************************************/

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
