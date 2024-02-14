#include "common.h"
#include <stdio.h>

template<class T> static vector<T> &operator<<(vector<T> &a, T const& b) { a.push_back(b); return a; }

template<class T> static vector<T> &operator, (vector<T> &a, T const& b) { return a << b; }

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

#include <glad/glad.h>
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
void Geometry_Init();

extern "C" ivec2 mainAnimation(float t, uint32_t iFrame, vec2 res, vec4 iMouse, btSoftRigidDynamicsWorld *dynamicWorld)
{
    static int frame = 0;
    if (frame++ == 0)
    {
        Geometry_Init();

        btTransform Identity;
        Identity.setIdentity();

        dynamicWorld->setGravity(btVector3(0,-10,0));
        btStaticPlaneShape *plane = new btStaticPlaneShape(btVector3(0,1,0), -0.05);
        btMotionState *state = new btDefaultMotionState( Identity, Identity );
        // btRigidBodyConstructionInfo;
        btRigidBody *ground = new btRigidBody(0., state, plane);
        ground->setDamping(.1, .1);
        ground->setFriction(.5);
        ground->setRestitution(.5);
        dynamicWorld->addRigidBody(ground);

        btTransform x;
        x.setIdentity();
        x.setOrigin(btVector3(1, 5, 0));
        state = new btDefaultMotionState( x, Identity );
        btCollisionShape *shape = new btBoxShape(btVector3(0.2, 0.2, 0.2));
        btRigidBody *body1 = new btRigidBody(1., state, shape);
        body1->setDamping(.0, .0);
        body1->setFriction(.1);
        body1->setRestitution(1.);
        dynamicWorld->addRigidBody(body1);

        x.setOrigin(btVector3(-1, 5, 0));
        state = new btDefaultMotionState( x, Identity );
        shape = new btSphereShape(0.3);
        btRigidBody *body2 = new btRigidBody(1., state, shape);
        body2->setDamping(0, 0);
        body2->setFriction(.1);
        body2->setRestitution(1.);
        dynamicWorld->addRigidBody(body2);

        btVector3 axis;
        // btTypedObject *cone = new btConeTwistConstraint(*body2, x);
        // btHingeConstraint *hinge1 = new btHingeConstraint(*body2, axis, axis);
        // btHinge2Constraint hinge2 = btHinge2Constraint(*body2, *body1, axis, axis, axis);
        // printf("Hot loading\n");
    }

    static float lastFrameTime = 0;
    float dt = t - lastFrameTime;
    lastFrameTime = t;

    dynamicWorld->stepSimulation(dt);
#if 1

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
            btVector3 half2 = ((btCapsuleShape*)shape)->getHalfHeight() * btVector3(1,0,0);
            btVector3 a = p - q * half2;
            btVector3 b = p + q * half2;
            lCapsule(U, (vec3&)a, (vec3&)b, radi2);
            break;
        }
    }
#endif
       // vector<vec3> U;

    // -------------------Camera-------------------//

    vec3 ta = vec3(0,1,0);
    vec2 mouse = clamp((vec2)iMouse, vec2(0), res);
    vec2 m = mouse / res * float(M_PI) * 2.0f + 1.13f;
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
