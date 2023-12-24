#version 300 es
precision mediump float;

layout (std140) uniform INPUT {
    vec4 iResolution;
    float iTime, iFrame, fov, iDrawWire;
    vec3 cameraPos, _pad2;
    vec3 targetPos, _pad3;
};

mat3 setCamera(in vec3 ro, in vec3 ta, float cr)
{
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr), 0.0);
    vec3 cu = normalize(cross(cw, cp));
    vec3 cv = cross(cu, cw);
    return mat3(cu, cv, cw);
}

mat4 perspective(float fov, float ar, float n, float f)
{
    return mat4(fov/ar, 0,0,0,0, fov, 0,0,0,0, (f+n)/(f-n),1, 0,0, -2.0*f*n/(f-n), 0);
}

#ifdef _VS
#define _varying out
#else
#define _varying in
#endif

_varying vec3 pos;
_varying vec3 nor;

#ifdef _VS
layout (location = 0) in vec3 a_Vertex;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in int a_DrawID;
void main()
{
    nor = a_Normal;
    pos = a_Vertex.xyz + vec3(0,a_DrawID,0);

    float ar = iResolution.x/iResolution.y;
    mat3 ca = setCamera(cameraPos, targetPos, 0.0);
    gl_Position = perspective(fov, ar, 0.1, 1000.0) * vec4((pos-cameraPos)*ca, 1);
}

#else
out vec4 fragColor;
void main(void)
{
    if (iDrawWire > 0.5) {
        fragColor = vec4(1.0);
        return;
    }
    vec3 fnor = normalize(cross(dFdx(pos), dFdy(pos)));
    vec3 rd = normalize(pos - cameraPos);
    float aa = 1.;//smoothstep(0.0, 0.15, -dot(normalize(nor), rd));
    vec3 col = vec3(0);
    col += nor * .5 + .5;//mix(nor, fnor, .2);
    fragColor = vec4(col, aa);
}
#endif