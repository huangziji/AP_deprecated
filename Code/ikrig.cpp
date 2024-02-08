#include <glm/glm.hpp>
using namespace glm;

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
    Foot_R,
    Toe_R,

    Shoulder_L,
    Elbow_L,
    Wrist_L,
    Hand_L,
    Leg_L,
    Knee_L,
    Foot_L,
    Toe_L,

    Joint_Max,
}IkRig;

extern const IkRig parentTable[] = {
    Null,
    Root,
    Hips,
    Spine1,
    Spine2,
    Spine2,
    Neck,
    Head,

    Spine2,
    Shoulder_R,
    Elbow_R,
    Wrist_R,
    Hips,
    Leg_R,
    Knee_R,
    Foot_R,

    Spine2,
    Shoulder_L,
    Elbow_L,
    Wrist_L,
    Hips,
    Leg_L,
    Knee_L,
    Foot_L,
};

const vec3 pNeck = {0,1.7,0};
const vec3 pWrist = {.7,1.6,0};
const vec3 pFoot = {.15,0,0};

static const vec3 Mirror = vec3(-1,1,1);

extern const vec3 joints[] = {
    vec3(0), // root
    vec3(0,1,0), // Hips
    mix(joints[Hips], pNeck, 0.25), // Spine1
    mix(joints[Hips], pNeck, 0.50), // Spine2
    mix(joints[Hips], pNeck, 0.75), // Spine3
    pNeck, // Neck
    vec3(0,1.75,0), // Head
    joints[Head] + vec3(0,0.2,0), // Head_End

    vec3(0.2,1.6,0), // Shoulder_R
    mix(joints[Shoulder_R], pWrist, 0.5), // Elbow_R
    pWrist, // Wrist_R
    joints[Wrist_R] + vec3(0.1,0,0), // Hand_R
    vec3(0.15,1.1,0), // Leg_R
    mix(joints[Leg_R], pFoot, 0.5) + vec3(0,0,.001), // Knee_R
    pFoot, // Foot_R
    joints[Foot_R] + vec3(0,0,0.2), // Toe_R

    joints[Shoulder_R] * Mirror,
    joints[Elbow_R] * Mirror,
    joints[Wrist_R] * Mirror,
    joints[Hand_R] * Mirror,
    joints[Leg_R] * Mirror,
    joints[Knee_R] * Mirror,
    joints[Foot_R] * Mirror,
    joints[Toe_R] * Mirror,
};

extern const bool hasMesh[] = {
    0,0,1,1,1,1,1,1,
    0,1,1,1,0,1,1,1,
    0,1,1,1,0,1,1,1,
};
