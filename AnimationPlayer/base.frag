#version 300 es
precision mediump float;

layout (std140) uniform INPUT {
    vec2 iResolution; float iTime, _pad1;
    vec3 _ro; float _fov;
    vec3 _ta; float _pad2;
};

mat3 setCamera(in vec3 ro, in vec3 ta, float cr)
{
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr), 0.0);
    vec3 cu = normalize(cross(cw, cp));
    vec3 cv = cross(cu, cw);
    return mat3(cu, cv, cw);
}

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

// https://iquilezles.org/articles/smin
float smin( float a, float b, float k )
{
    float h = max(k-abs(a-b),0.0);
    return min(a, b) - h*h*0.25/k;
}

//============================================================//

vec2 map(vec3 pos)
{
    float d1, d2;
    d1 = pos.y + 1.;
    float id = 2.;
//    d2 = length(pos-vec3(0,1,0)) - .1;
//    if (d2 < d1) { d1=d2; id=1.; }
//    d2 = sdBox(pos-vec3(-.2,1,0), .3) - .02;
//    d1 = smin(d1,d2,.2);

    return vec2(d1, id);
}

vec2 matcap(vec3 eye, vec3 normal)
{
    vec3 reflected = reflect(eye, normal);
    float m = 2.8284271247461903 * sqrt( reflected.z+1.0 );
    return reflected.xy / m + 0.5;
}

vec3 calcNormal(vec3 pos)
{
    vec2 e = vec2(1.0,-1.0)*0.5773*0.001;
    return normalize(e.xyy*map( pos + e.xyy ).x +
                     e.yyx*map( pos + e.yyx ).x +
                     e.yxy*map( pos + e.yxy ).x +
                     e.xxx*map( pos + e.xxx ).x );
}

const float fov = 1.2;
uniform sampler2D iChannel0, iChannel1, iChannel2;
out vec4 fragColor;
void main()
{
    vec3 ro = _ro, ta = _ta;
    vec2 uv = (2.0*gl_FragCoord.xy-iResolution.xy)/iResolution.y;
    mat3 ca = setCamera(ro, ta, 0.0);
    vec3 rd = ca * normalize(vec3(uv, fov));

    // compute rasterized polygon depth in world space
    float dep = texture(iChannel0, gl_FragCoord.xy/iResolution.xy).r   *2.0-1.0;
    vec3  nor = texture(iChannel1, gl_FragCoord.xy/iResolution.xy).rgb *2.0-1.0;
    const float n = 0.1, f = 1000.0;
    const float p10 = (f+n)/(f-n), p11 = -2.0*f*n/(f-n); // from perspective matrix
        dep = p11 / ( (dep-p10) * dot(rd, normalize(ta-ro)) );
        nor = normalize(nor);

    // ray marching
    float t, i, m;
    for (t=0.,i=0.; i<100.; i++)
    {
        vec2 h = map(ro + rd*t);
        t += h.x;
        m = h.y;
        if (h.x < 0.0001 || t > 100.) break;
    }

    vec3 col = vec3(0);

    // compare rasterized objects to raymarching objects
    if (t < 100. || dep < 100.)
    {
        vec3 pal[] = vec3[](
                vec3(0.5,0.7,0.6),
                vec3(0.5,0.4,0.7),
                vec3(0.7,0.7,0.5));
        int ci = int(floor(m));
        vec3 mate = mix(pal[ci], pal[ci+1], fract(m));

        if (t < dep)
        { // raymarching objects
            vec3 pos = ro + rd*t;
            nor = calcNormal(pos);
        }
        else
        { // polygons
            vec2 uv = matcap(rd, nor);
            mate = textureLod(iChannel2, uv, 2.0).rgb;
//            mate = vec3(.5,.6,.7);
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
