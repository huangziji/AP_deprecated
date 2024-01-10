#version 430

float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}

float map(vec3 pos)
{
    return sdTorus(pos, vec2(.5,.2));
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
layout (binding = 2, r8) writeonly uniform image3D outImage;
void main()
{
    vec3 off = vec3(.8), sca = off*2./64.;

    ivec3 p = ivec3(gl_GlobalInvocationID.xyz);
    float d = map(vec3(p)*sca - off);
    imageStore(outImage, p, vec4(d > 0));
}
