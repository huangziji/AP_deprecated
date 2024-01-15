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
layout (location = 4) in vec3 a_sca;
layout (location = 5) in vec3 a_ce;
layout (location = 6) in vec3 a_off;
layout (location = 7) in vec3 a_swi;
void main()
{
    mat3 swi = rotationAlign(normalize(a_swi), vec3(0,1,0));
    //swi = rotationAlign(vec3(0,cos(iTime),sin(iTime)), vec3(0,1,0));

    v_Normal = a_Normal * swi;
    v_Position = ( (a_Vertex.xyz*a_sca+a_ce) * swi + a_off );
    v_Position += a_Normal * enableWireFrame*0.001;

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
        fragColor = vec4(nor*0.5+0.5, 1.0);
    }
}
#endif
