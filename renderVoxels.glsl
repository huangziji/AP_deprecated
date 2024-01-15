#version 430

// axis aligned box centered at the origin, with size boxSize
vec2 boxIntersection( in vec3 ro, in vec3 rd, vec3 boxSize, out vec3 outNormal )
{
    vec3 m = 1.0/rd; // can precompute if traversing a set of aligned boxes
    vec3 n = m*ro;   // can precompute if traversing a set of aligned boxes
    vec3 k = abs(m)*boxSize;
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;
    float tN = max( max( t1.x, t1.y ), t1.z );
    float tF = min( min( t2.x, t2.y ), t2.z );
    if( tN>tF || tF<0.0) return vec2(-1.0); // no intersection
    outNormal = (tN>0.0) ? step(vec3(tN),t1) : // ro ouside the box
                           step(t2,vec3(tF));  // ro inside the box
    outNormal *= -sign(rd);
    return vec2( tN, tF );
}

#define saturate(x) clamp(x,0.,1.)
layout (local_size_x = 32, local_size_y = 18) in;
layout (binding = 2, rgba8) writeonly uniform image2D outImage;
void main(void)
{
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
    vec2 fragCoord = vec2(gl_GlobalInvocationID.xy);

    vec3 ro = _ro, ta = _ta;
    vec2 uv = (2.0*fragCoord-iResolution.xy)/iResolution.y;
    mat3 ca = setCamera(ro, ta, 0.0);
    vec3 rd = ca * normalize(vec3(uv, _fov));

    vec3 col = vec3(0);

    vec3 nor;
    vec3 off = vec3(.8);
    vec2 tt = boxIntersection(ro, rd, off, nor);
    if (tt.y > tt.x)
    {
        int size = textureSize(iChannel3, 0).r;
        vec3 sca = float(size)/(off*2.0);

        // world space to voxel space
        vec3 pos = ro + rd*max(tt.x, 0.); // start with ro if ro is inside of box
        vec3 voxelPos = ( pos+off )*sca;

        int lvl = int(floor(4.5 * saturate(sin(iTime)*.5+.5)) + .0001);
        int voxelSize = 1<<lvl;

        vec3 deltaDist = abs(vec3(length(rd)) / rd);
        ivec3 rayStep = ivec3(sign(rd));
        ivec3 mapPos = ivec3(floor(voxelPos - nor*.0001));
            mapPos -= mapPos%voxelSize;
        vec3 sideDist = ( (vec3(mapPos)-voxelPos) + float(voxelSize)*max(sign(rd),0.0) )*sign(rd)*deltaDist;

        bool hit;
        int steps = 0;
        vec3 mask = abs(nor);
        float tF = 0.;

        while ( true )
        {
            vec3 sp = (vec3(mapPos) + .5)/float(size);
            //hit = texelFetch( iChannel3, mapPos/voxelSize, lvl ).r > 0.0001;
            hit = textureLod( iChannel3, sp, float(lvl) ).r > 0.0001;

            if (hit)
            {
                if (lvl > 0)
                {
                    lvl--;
                    voxelSize = 1<<lvl;
                    mapPos = ivec3(floor(voxelPos + rd*tF + mask*sign(rd)*.0001));
                    mapPos -= mapPos%(voxelSize);
                    sideDist = ( (vec3(mapPos)-voxelPos) + float(voxelSize)*max(sign(rd),0.0) )*sign(rd)*deltaDist;
                    continue;
                }
                else
                {
                    break;
                }
            }

            mask = step(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
            tF = min(min(sideDist.x, sideDist.y), sideDist.z);
            sideDist += mask * deltaDist * float(voxelSize);
            mapPos += ivec3(mask) * rayStep * voxelSize;

            steps++;

            if ( any(greaterThanEqual(mapPos, ivec3(size))) ||
                 any(lessThan(mapPos, ivec3(0))) )
                break;
        }

        col += float(steps)/150.;
        col += dot(normalize(mask), vec3(.5,.7,.9))*.7*float(hit);
    }

    imageStore(outImage, id, vec4(col, 1));
}
