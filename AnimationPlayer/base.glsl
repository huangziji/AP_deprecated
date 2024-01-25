#version 300 es
precision mediump float;

layout (std140) uniform INPUT {
    vec2 iResolution; float iTime, _pad1;
    vec3 _ro; float _fov;
    vec3 _ta; float _pad2;
};

mat3 setCamera(in vec3 ro, in vec3 ta, float cr)
{
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr), 0.0);
    vec3 cu = normalize(cross(cw, cp));
    vec3 cv = cross(cu, cw);
    return mat3(cu, cv, cw);
}

mat4 getProjectionMatrix()
{
    float n = 0.1, f = 1000.0;
    float p1 = (f+n)/(f-n);
    float p2 = -2.0*f*n/(f-n);
    float ar = iResolution.x/iResolution.y;
    return mat4(_fov/ar, 0,0,0,0, _fov, 0,0,0,0, p1,1,0,0,p2,0);
}

vec4 World2Clip(vec3 pos)
{
    mat3 ca = setCamera(_ro, _ta, 0.);
    return getProjectionMatrix() * vec4((pos-_ro)*ca, 1.);
}
#ifdef _VS
#define _varying out
#else
#define _varying in
#endif

_varying vec3 v_Position;
_varying vec3 v_Normal;

#ifdef _VS
layout (location = 0) in vec3 a_Vertex;
layout (location = 1) in vec3 a_Normal;
layout (location = 4) in vec3 a_pos;
layout (location = 5) in mat3 a_rot;
void main()
{
    v_Normal = a_Normal * a_rot;
    v_Position = a_Vertex.xyz * a_rot + a_pos;
    mat3 ca = setCamera(_ro, _ta, 0.0);
    gl_Position = getProjectionMatrix() * vec4((v_Position-_ro)*ca, 1);
}

#else
out vec4 fragColor;
void main(void)
{
    vec3 nor = normalize(v_Normal);
    fragColor = vec4(nor*0.5+0.5, 1.0);
}
#endif
