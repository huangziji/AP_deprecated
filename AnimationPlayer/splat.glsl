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

#ifdef _VS
#define _varying out
#else
#define _varying in
#endif

_varying vec2 UV;

#ifdef _VS
layout (location = 0) in vec3 a_Vertex;
layout (location = 1) in vec3 a_Normal;
void main()
{
    UV = vec2(gl_VertexID&1, gl_VertexID/2) *2.-1.;
    vec3 pos = vec3(UV, 0);

    float t = iTime*5.;
    float c = cos(t), s = sin(t);
    pos.xy *= mat2(c,s,-s,c);
    pos.y += c + 1.;
    pos *= .2;

    mat3 ca = setCamera(_ro, _ta, 0.);
    gl_Position = getProjectionMatrix() * vec4(pos - _ro*ca, 1);
}
#else
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 id;
void main()
{
    float d = smoothstep(0., -.1, length(UV)-1.);
    vec3 col = vec3(1,0,1);
    fragColor = vec4(col, d);
    id = vec4(d);
}
#endif
