#version 300 es

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
    v_Position = a_Vertex.xyz + a_Translation;
    v_Position += a_Normal*enableWireFrame*0.001;
    mat3 ca = setCamera(_ro, _ta, 0.0);
    gl_Position = getProjectionMatrix() * vec4((v_Position-_ro)*ca, 1);
}

#else
out vec4 fragColor;
void main(void)
{
    if (enableWireFrame > 0.5) {
        fragColor = vec4(1.0);
    } else {
        vec3 nor = normalize(v_Normal);
        fragColor = vec4(nor*.5+.5, 1);
    }
}
#endif
