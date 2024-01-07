#version 300 es

#ifdef _VS
in vec4 a_Vertex;
void main()
{
    vec3 pos = a_Vertex.xyz;
    mat3 ca = setCamera(_ro, _ta, 0.0);
    gl_Position = getProjectionMatrix() * vec4((pos-_ro)*ca, 1);
}

#else
out vec4 fragColor;
void main()
{
    fragColor = vec4(1);
}
#endif
