#version 300 es
precision mediump float;

layout (std140) uniform INPUT {
    vec2 iResolution; float iTime, iFrame;
    vec3 cameraPos; float fov;
    vec3 targetPos; float _pad1;
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

uniform float enableWireFrame;
_varying vec3 v_Position;
_varying vec3 v_Normal;

#ifdef _VS
layout (location = 0) in vec3 a_Vertex;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec3 a_Translation;
void main()
{
    v_Normal = a_Normal;
    v_Position = a_Vertex.xyz + a_Translation;//vec3(-3+a_DrawID*3,0,0);
    v_Position += a_Normal*enableWireFrame*0.01;

    float ar = iResolution.x/iResolution.y;
    mat3 ca = setCamera(cameraPos, targetPos, 0.0);
    gl_Position = perspective(fov, ar, 0.1, 1000.0) * vec4((v_Position-cameraPos)*ca, 1);
}

#else
out vec4 fragColor;
void main(void)
{
    if (enableWireFrame > 0.5) {
        fragColor = vec4(1.0);
    } else {
        vec3 nor = normalize(v_Normal);
        fragColor = vec4(nor*.5+.5, 1.0);
    }
    //vec3 rd = normalize(v_Position - cameraPos);
    //float aa = smoothstep(0.0, 0.15, -dot(normalize(nor), rd));
}
#endif
