#include "common.h"
#include <stdio.h>

template<class T> static vector<T> &operator<<(vector<T> &a, T const& b) { a.push_back(b); return a; }

template<class T> static vector<T> &operator, (vector<T> &a, T const& b) { return a << b; }

/// @link https://www.shadertoy.com/view/4djSRW
float hash11(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

typedef struct {
    mat3 rot;
    vec3 pos;
}Instance;

typedef struct {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
}Command;

typedef enum {
    Null = -1,

    Root,
    Hips,
    Spine1,
    Spine2,
    Spine3,
    Neck,
    Head,
    Head_End,

    Shoulder_R,
    Elbow_R,
    Wrist_R,
    Hand_R,
    Leg_R,
    Knee_R,
    Ankle_R,
    Toe_R,

    Shoulder_L,
    Elbow_L,
    Wrist_L,
    Hand_L,
    Leg_L,
    Knee_L,
    Ankle_L,
    Toe_L,

    Joint_Max,
}IkRig;

static const int RagdollJoints[][2] = {
    Hips, Neck,
    Head, Head_End,

    Shoulder_L, Elbow_L,
    Elbow_L, Wrist_L,
    Wrist_L, Hand_L,
    Leg_L, Knee_L,
    Knee_L, Ankle_L,
    Ankle_L, Toe_L,

    Shoulder_R, Elbow_R,
    Elbow_R, Wrist_R,
    Wrist_R, Hand_R,
    Leg_R, Knee_R,
    Knee_R, Ankle_R,
    Ankle_R, Toe_R,
};

typedef struct {
    vec3 lowerBound, upperBound;
}Aabb;

typedef struct {
    Aabb box;
    int parentIndex;
    int child1;
    int child2;
    bool isLeaf;
}Node;

struct AabbTree
{
    enum { NullIndex = -1 };
    vector<Node> _nodes;
    uint _rootIndex = NullIndex;

    void InsertLeaf(Aabb box)
    {
        const int leafIndex = _nodes.size();
        _nodes.push_back({ box, NullIndex, NullIndex, NullIndex, true });
        if (_rootIndex == NullIndex)
        {
            _rootIndex = leafIndex;
            return;
        }
    }
};

#include <glad/glad.h>
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>

void _init()
{
// place holders, this solve some dynamic linking problem
// where program crashes while adding or removing constraints
// my guess is some lazy initialization were performed during constraint creation
// this will probably different from compiler to compiler
    btTransform x;
    x.setIdentity();
    btVector3 p = x.getOrigin();
    btRigidBody rb = btRigidBody(0., NULL, NULL);
    btTypedConstraint *joint1 = new btConeTwistConstraint(rb, x);
    btTypedConstraint *joint2 = new btHingeConstraint(rb, p, p);
    btTypedConstraint *joint3 = new btFixedConstraint(rb, rb, x, x);
    btTypedConstraint *joint4 = new btGearConstraint(rb, rb, p, p);
    btTypedConstraint *joint5 = new btSliderConstraint(rb, rb, x, x, false);
    btTypedConstraint *joint6 = new btPoint2PointConstraint(rb, rb, p, p);
    btTypedConstraint *joint7 = new btGeneric6DofConstraint(rb, rb, x, x, false);
}

extern "C" ivec2 mainAnimation(float t, uint32_t iFrame, vec2 res, vec4 iMouse, btDynamicsWorld *dynamicWorld)
{
    if (iFrame == 0)
    {
        btContactSolverInfo &solverInfo = dynamicWorld->getSolverInfo();
        solverInfo.m_solverMode |= SOLVER_RANDMIZE_ORDER;
        solverInfo.m_splitImpulse = true;

        btDispatcherInfo &dispatcherInfo = dynamicWorld->getDispatchInfo();
        dispatcherInfo.m_enableSatConvex = true;
    }

    static int frame = 0;
    if (frame++ == 0)
    {
        btTransform Identity;
        Identity.setIdentity();

        btRigidBody *ground = new btRigidBody( 0,
            new btDefaultMotionState( Identity, Identity ),
            new btStaticPlaneShape(btVector3(0,1,0), -0.05) );
        ground->setDamping(.1, .1);
        ground->setFriction(.5);
        ground->setRestitution(.5);
        dynamicWorld->addRigidBody(ground);

        btTransform x;
        x.setIdentity();

        x.setOrigin(btVector3(1, 5, 0));
        btRigidBody *rb1 = new btRigidBody( .5,
            new btDefaultMotionState( x, Identity ),
            new btCapsuleShape(.2, .3));
        rb1->setDamping(.2, .2);
        rb1->setFriction(.5);
        rb1->setRestitution(.5);
        dynamicWorld->addRigidBody(rb1);

        x.setOrigin(btVector3(-1, 5, 0));
        btRigidBody *rb2 = new btRigidBody( .5,
            new btDefaultMotionState( x, Identity ),
            new btSphereShape(0.3));
        rb2->setDamping(.2, .2);
        rb2->setFriction(.5);
        rb2->setRestitution(.5);
        dynamicWorld->addRigidBody(rb2);
    }

    static float lastFrameTime = 0;
    float dt = t - lastFrameTime;
    lastFrameTime = t;

    dynamicWorld->stepSimulation(dt);

    vector<vec3> U;
    btCollisionObjectArray const& arr = dynamicWorld->getCollisionObjectArray();
    for (int i=0; i<arr.size(); i++)
    {
        btRigidBody *body = btRigidBody::upcast(arr[i]);
        btCollisionShape *shape = body->getCollisionShape();
        btVector3 half1 = ((btBoxShape*)shape)->getHalfExtentsWithMargin();
        float radi1 = ((btSphereShape*)shape)->getRadius();
        float radi2 = ((btCapsuleShape*)shape)->getRadius();
        btTransform pose = body->getWorldTransform();
        btMatrix3x3 q = btMatrix3x3(pose.getRotation());
        btVector3 p = pose.getOrigin();

        switch (shape->getShapeType())
        {
        case BOX_SHAPE_PROXYTYPE:
            lBox(U, mat3(.2), (vec3&)p);
            break;
        case SPHERE_SHAPE_PROXYTYPE:
            lSphere(U, (vec3&)p, radi1);
            break;
        case CAPSULE_SHAPE_PROXYTYPE:
            btVector3 half2 = ((btCapsuleShape*)shape)->getHalfHeight() * btVector3(0,1,0);
            btVector3 a = p - half2 * q;
            btVector3 b = p + half2 * q;
            lCapsule(U, (vec3&)a, (vec3&)b, radi2);
            break;
        }
    }

    // ------------------------------Camera------------------------------//

    vec3 ta = vec3(0,1,0);
    vec2 m = (vec2&)iMouse / res * float(M_PI) * 2.0f + 1.13f;
    vec3 ro = ta + vec3(sin(m.x),.5,cos(m.x)) * 2.5f;

    vector<Instance> I;

    extern const IkRig parentTable[];
    extern const bool hasMesh[];
    extern const vector<vec3> jointsLocal;

    vector<vec3> world(Joint_Max, vec3(0));
    vector<mat3> local(Joint_Max, mat3(1));

    for (int i=Hips; i<Joint_Max; i++)
    {
        int p = parentTable[i];
        world[i] = world[p] + jointsLocal[i] * local[p];
        local[i] *= local[p];
    }

    for (int i=0; i<Joint_Max; i++)
    {
        if (!hasMesh[i]) continue;

        int p = parentTable[i];
        float w = .2 + hash11(i + 349) * .1;
            w *= 1 - (i>Shoulder_R || p == Neck) * .5;
        float r = length(jointsLocal[i]);
        mat3 rot = rotationAlign(jointsLocal[i]/r, vec3(0,0,1));
        vec3 sca = abs(rot * vec3(w,w,r)) * .5f;

        mat3 swi = matrixCompMult(local[i], mat3(sca,sca,sca));
        vec3 ce = mix(world[i], world[p], .5f);
        I << Instance{ swi, ce };
    }

    void loadBuffers(vector<vec3> const& U, vector<Instance> const& I);
    loadBuffers(U, I);
    const float data[] = {
        res.x,res.y, t, 0,
        ro.x,ro.y,ro.z, 0,
        ta.x,ta.y,ta.z, 0,
    };
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof data, data);
    return ivec2(1, U.size());
}

void loadBuffers(vector<vec3> const& U, vector<Instance> const& I)
{
    static vector<Command> C;
    static GLuint vao, vbo1, vbo2, ibo, ebo, ubo, cbo, frame;
    if (!frame++)
    {
        vector<Vertex> V;
        vector<Index> F;

        uint firstIndex = 0;
        uint baseVertex = 0;
        tCubeMap(V, F, 2);
        C << Command{ (uint)F.size()-firstIndex, 0, firstIndex, baseVertex, 0 };
        firstIndex = F.size();
        baseVertex = V.size();
        tCapsule(V, F, 0);
        C << Command{ (uint)F.size()-firstIndex, 0, firstIndex, baseVertex, 0 };
        firstIndex = F.size();
        baseVertex = V.size();

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo1);
        glGenBuffers(1, &vbo2);
        glGenBuffers(1, &ubo);
        glGenBuffers(1, &ebo);
        glGenBuffers(1, &cbo);
        glGenBuffers(1, &ibo);

        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, 64, NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, F.size() * sizeof F[0], F.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, vbo1);
        glBufferData(GL_ARRAY_BUFFER, V.size() * sizeof V[0], V.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);

        glBindBuffer(GL_ARRAY_BUFFER, vbo2);
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, 12, 0);

        glBindBuffer(GL_ARRAY_BUFFER, ibo);
        for (size_t off=0, i=4; i<8; i++, off+=12)
        {
            glEnableVertexAttribArray(i);
            glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, sizeof(Instance), (void*)off);
            glVertexAttribDivisor(i, 1);
        }
    }

    C[0].instanceCount = I.size();

    { // command buffer
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo);
        int oldSize;
        glGetBufferParameteriv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_SIZE, &oldSize);
        int newSize = C.size() * sizeof C[0];
        if (oldSize < newSize)
        {
            glBufferData(GL_DRAW_INDIRECT_BUFFER, newSize, C.data(), GL_DYNAMIC_DRAW);
        }
        else
        {
            glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, newSize, C.data());
        }
    }
    { // index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    }
    { // channel 0 1
        glBindBuffer(GL_ARRAY_BUFFER, vbo1);
    }
    { // channel 4 5 6 7
        glBindBuffer(GL_ARRAY_BUFFER, ibo);
        int oldSize;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &oldSize);
        int newSize = I.size() * sizeof I[0];
        if (oldSize < newSize)
        {
            glBufferData(GL_ARRAY_BUFFER, newSize, I.data(), GL_DYNAMIC_DRAW);
        }
        else
        {
            glBufferSubData(GL_ARRAY_BUFFER, 0, newSize, I.data());
        }
    }
    { // channel 8
        glBindBuffer(GL_ARRAY_BUFFER, vbo2);
        int oldSize;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &oldSize);
        int newSize = U.size() * sizeof U[0];
        if (oldSize < newSize)
        {
            glBufferData(GL_ARRAY_BUFFER, newSize, U.data(), GL_DYNAMIC_DRAW);
        }
        else
        {
            glBufferSubData(GL_ARRAY_BUFFER, 0, newSize, U.data());
        }
    }
}
