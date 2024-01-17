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

//============================================================//

vec2 map(vec3 pos)
{
    float d1, d2;
    d1 = 1e5;
//    d2 = sdBox(pos, .5) - .02;
//    d1 = min(d1, d2);
//    d2 = sdTorus(pos, vec2(.5,.2));
//    d1 = max(-d1, d2);
    d2 = pos.y;
    d1 = min(d1, d2);
    //d2 = length(pos) - .1;
    //d1 = min(d1, d2);

    return vec2(d1, 1.);
}

vec3 calcNormal(vec3 pos)
{
    vec2 e = vec2(1.0,-1.0)*0.5773*0.001;
    return normalize(e.xyy*map( pos + e.xyy ).x +
                     e.yyx*map( pos + e.yyx ).x +
                     e.yxy*map( pos + e.yxy ).x +
                     e.xxx*map( pos + e.xxx ).x );
}

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
    for (t=0.,i=0.; i<100.; i++) {
        vec2 h = map(ro + rd*t);
        t += h.x;
        m = h.y;
        if (h.x < 0.0001 || t > 100.) break;
    }

    vec3 col = vec3(0);

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

#define saturate(x) clamp(x,0.,1.)
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
