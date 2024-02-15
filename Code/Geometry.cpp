#include <stdio.h>
#include <glm/glm.hpp>
using namespace glm;
#include <boost/container/vector.hpp>
using boost::container::vector;

#include <bullet/btBulletDynamicsCommon.h>


void Geometry_Init()
{
    btTransform x;
    btBoxShape *shape = new btBoxShape(btVector3(1.,1.,1.));
    btMotionState *state = NULL;
    btRigidBody body = btRigidBody(1., state, shape);
    btTypedConstraint *cone = new btConeTwistConstraint( body, x);
    printf("Hello, Geometry.cpp!\n");
}
