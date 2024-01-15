#version 300 es

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
    vec3 pos = a_Vertex.xyz;
    mat3 ca = setCamera(_ro, _ta, 0.0);
    gl_Position = getProjectionMatrix() * vec4((pos-_ro)*ca, 1);
}

#else
out vec4 fragColor;
void main()
{
    vec3 col = sin(vec3(0.14,214.32,14.212)*v_id)*.5+.5;
    fragColor = vec4(col, 1);
}
#endif
