#version 430
float sdBox( vec3 p, float b )
{
    vec3 q = abs(p) - b;
    return length(max(q, 0.f));
}

float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}

float map(vec3 pos)
{
    float d = sdBox(pos, .5) - .02;
    float d2 = sdTorus(pos, vec2(.65,.2));
    //d = max(-d, d2);
    return d2;
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
layout (binding = 3, r8) writeonly uniform image3D outImage;
void main()
{
    vec3 off = vec3(.8), sca = off*2./64.;

    ivec3 p = ivec3(gl_GlobalInvocationID.xyz);
    float d = map(vec3(p)*sca - off);
    imageStore(outImage, p, vec4(d < 0));
}
