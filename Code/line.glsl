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

flat _varying float v_id;

#ifdef _VS
layout (location = 8) in vec4 a_Vertex;
void main()
{
    v_id = a_Vertex.w;
    vec3 pos = a_Vertex.xyz * 1.01;
    gl_Position = World2Clip(pos);
}

#else
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 id;
void main()
{
    vec3 col = normalize(sin(vec3(13.144,412.32,141.212)*v_id));
    fragColor = vec4(col, 1);
    id = vec4(1);
}
#endif
