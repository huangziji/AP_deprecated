#version 300 es
precision mediump float;

mat3 setCamera(in vec3 ro, in vec3 ta, float cr)
{
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr), 0.0);
    vec3 cu = normalize(cross(cw, cp));
    vec3 cv = cross(cu, cw);
    return mat3(cu, cv, cw);
}

vec4 World2Clip(vec3 pos, vec3 rd)
{
    const float ie = 1./5.;
    mat3 ca = setCamera(vec3(0), rd, 0.);
    return vec4(ie,ie,-ie,1.) * vec4(pos*ca, 1.);
}

#ifdef _VS
layout (location = 0) in vec3 a_Vertex;
layout (location = 3) in vec3 a_Scale;
layout (location = 4) in vec3 a_Position;
layout (location = 5) in mat3 a_Rotation;
void main()
{
    vec3 pos = a_Vertex.xyz * a_Scale * a_Rotation + a_Position;
    gl_Position = World2Clip(pos, normalize(vec3(1,2,3)));
}

#else
void main(void)
{
}
#endif
