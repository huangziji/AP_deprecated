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

mat4 ortho(float l, float r, float b, float t, float n, float f)
{
    return mat4(
                2./(r-l),0,0,0,
                0,2./(t-b),0,0,
                0,0,2./(n-f),0,
                -(r+l)/(r-l),
                -(t+b)/(t-b),
                -(f+n)/(f-n),
                1.);
}

vec4 World2Clip(vec3 pos)
{
    float r = 1.5;
    mat3 ca = setCamera(vec3(0), normalize(vec3(1,2,3)), 0.);
    return ortho(-r,r,-r,r,-r,r) * vec4(pos*ca, 1.);
}

#ifdef _VS
layout (location = 0) in vec3 a_Vertex;
layout (location = 3) in vec3 a_Scale;
layout (location = 4) in vec3 a_Position;
layout (location = 5) in mat3 a_Rotation;
void main()
{
    vec3 pos = a_Vertex.xyz * a_Scale * a_Rotation + a_Position;
    gl_Position = World2Clip(pos);
}

#else
void main(void)
{
}
#endif
