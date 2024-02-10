#ifndef COMMON_H
#define COMMON_H
#include <boost/container/vector.hpp>
using boost::container::vector;
#include <glm/glm.hpp>
using namespace glm;

typedef struct {
    vec3 pos, nor;
}Vertex;
typedef unsigned short Index;

void lBox(vector<vec3> & V, mat3 rot, vec3 pos);

void lCircle(vector<vec3> & V, vec3 ce, float r, vec3 dir);

void lSphere(vector<vec3> & V, vec3 ce, float r);

void lCapsule(vector<vec3> & V, vec3 a, vec3 b, float r);

void tCubeMap(vector<Vertex> & V, vector<Index> & F, int N);

void tCapsule(vector<Vertex> & V, vector<Index> & F, float t);

float hash11(float p);

mat3x3 rotationAlign( vec3 d, vec3 z );

vec3 solve( vec3 p, float r1, float r2, vec3 dir );

mat3 rotateX(float a);

mat3 rotateY(float a);

mat3 rotateZ(float a);

float spline( const float *k, int n, float t );

#endif // COMMON_H
