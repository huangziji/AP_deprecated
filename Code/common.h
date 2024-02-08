#ifndef COMMON_H
#define COMMON_H
#include <boost/container/vector.hpp>
using boost::container::vector;
#include <glm/glm.hpp>
using namespace glm;

typedef mat2x3 Aabb;
typedef vec4 BoundingSphere;

vector<vec3> &operator<<(vector<vec3> &a, Aabb const& b);

vector<vec3> &operator<<(vector<vec3> &a, BoundingSphere const& b);

float hash11(float p);

mat3x3 rotationAlign( vec3 d, vec3 z );

vec3 solve( vec3 p, float r1, float r2, vec3 dir );

mat3 rotateX(float a);

mat3 rotateY(float a);

mat3 rotateZ(float a);

#endif // COMMON_H
