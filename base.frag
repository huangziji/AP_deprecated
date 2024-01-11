#version 300 es

float sdBox(vec3 pos, float b)
{
    vec3 q = abs(pos) - b;
    return length(max(q, 0.0));
}

float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}

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

//============================================================//

vec2 map(vec3 pos)
{
    float d = pos.y+.5;
    float d2;
    //d2 = sdBox(pos, .7) - 0.02;
    //d2 = sdTorus(pos, vec2(.5,.2));
    //d2 = length(pos) - .15;
    //d = min(d, d2);

    return vec2(d, 1.);
}

vec3 calcNormal(vec3 pos)
{
    vec2 e = vec2(1.0,-1.0)*0.5773*0.001;
    return normalize(e.xyy*map( pos + e.xyy ).x +
                     e.yyx*map( pos + e.yyx ).x +
                     e.yxy*map( pos + e.yxy ).x +
                     e.xxx*map( pos + e.xxx ).x );
}

#define saturate(x) clamp(x,0.,1.)
out vec4 fragColor;
void main()
{
    vec3 ro = _ro, ta = _ta;
    vec2 uv = (2.0*gl_FragCoord.xy-iResolution.xy)/iResolution.y;
    mat3 ca = setCamera(ro, ta, 0.0);
    vec3 rd = ca * normalize(vec3(uv, _fov));

    // compute rasterized polygon depth in world space
    float dep = texture(iChannel0, gl_FragCoord.xy/iResolution.xy).r   *2.0-1.0;
    vec3  nor = texture(iChannel1, gl_FragCoord.xy/iResolution.xy).rgb *2.0-1.0;
    const float n = 0.1, f = 1000.0;
    const float p10 = (f+n)/(f-n), p11 = -2.0*f*n/(f-n); // from perspective matrix
        dep = p11 / ( (dep-p10) * dot(rd, normalize(ta-ro)) );
        nor = normalize(nor);

    // ray marching
    float t, i, m;
    for (t=0.,i=0.; i<50.; i++) {
        vec2 h = map(ro + rd*t);
        t += h.x;
        m = h.y;
        if (h.x < 0.0001 || t > 100.) break;
    }

    vec3 col = vec3(0);

#if 1 // voxel ray marching
    {
        vec3 nor;
        vec3 off = vec3(.8);
        vec2 tt = boxIntersection(ro, rd, off, nor);
        if (tt.y > tt.x)
        {
            int size = textureSize(iChannel2, 0).r;
            vec3 sca = float(size)/(off*2.0);

            // world space to voxel space
            vec3 pos = ro + rd*max(tt.x, 0.); // start with ro if ro is inside of box
            vec3 voxelPos = ( pos+off )*sca;

            int lvl = 3;
            int voxelSize = 1<<lvl;

            vec3 deltaDist = abs(vec3(length(rd)) / rd);
            ivec3 rayStep = ivec3(sign(rd));
            ivec3 mapPos = ivec3(floor(voxelPos - nor*.0001));
                mapPos -= mapPos%voxelSize;
            vec3 sideDist = ( (vec3(mapPos)-voxelPos) + float(voxelSize)*max(sign(rd),0.0) )*sign(rd)*deltaDist;

            bool hit;
            int steps = 0;
            vec3 mask = abs(nor);
            float tF;

            while ( true )
            {
                vec3 sp = (vec3(mapPos)+.5)/float(size);
                //hit = texelFetch( iChannel2, mapPos/voxelSize, lvl ).r > 0.0001;
                hit = textureLod( iChannel2, sp, float(lvl) ).r > 0.0001;

                if (hit)
                {
//                    break;
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

//            col += float(steps)/150.;
            col += dot(normalize(mask), vec3(.5,.7,.9))*.7*float(hit);
        }
        fragColor = vec4(col, 1);
        return;
    }
#endif

    // compare rasterized objects to raymarching objects
    if (t < 100. || dep < 100.) {
        vec3 mate;
        if (m < 0.5) {
            mate = vec3(0.5,0.7,0.6);
        } else if (m < 1.5) {
            mate = vec3(0.7,0.7,0.5);
        }

        if (t < dep) { // raymarching objects
            vec3 pos = ro + rd*t;
            nor = calcNormal(pos);
        } else { // polygons
            mate = vec3(0.5,0.6,0.7);
        }

        const vec3 sun_dir = normalize(vec3(1,2,3));
        float sun_dif = saturate(dot(nor, sun_dir))*.9+.1;
        float sky_dif = saturate(dot(nor, vec3(0,1,0)))*.15;
        col += vec3(0.9,0.9,0.5)*sun_dif*mate;
        col += vec3(0.5,0.6,0.9)*sky_dif;
    } else {
        col += vec3(0.5,0.6,0.9)*1.2 - rd.y*.4;
    }

    //col += i/50. * .2;

    col = pow(col, vec3(0.4545));
    fragColor = vec4(col, 1);
}
