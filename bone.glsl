#version 300 es
#define r3 1.732050808

const float r2 = .05, r1 = .3/r2;
const vec3 verts[] = vec3[](
    vec3(0),
    vec3(r3,r1,1)*r2,
    vec3(-r3,r1,1)*r2,
    vec3(0,r1,-2)*r2,
    vec3(0,1,0)
);

const int edges[] = int[](
    0,1, 0,2, 0,3,
    1,2, 2,3, 3,1,
    4,1, 4,2, 4,3
);

#ifdef _VS
layout (location = 4) in vec3 a_off;
layout (location = 5) in vec3 a_swi;
void main(void)
{
    mat3 swi = rotationAlign(a_swi + .0001, vec3(0,1,0));
    vec3 pos = verts[edges[gl_VertexID]]*swi + a_off;
    mat3 ca = setCamera(_ro, _ta, 0.0);
    gl_Position = getProjectionMatrix() * vec4((pos-_ro)*ca, 1);
}
#else
out vec4 fragColor;
void main(void)
{
    fragColor = vec4(1);
}
#endif
