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

vec2 matcap(vec3 eye, vec3 normal)
{
    vec3 reflected = reflect(eye, normal);
    float m = 2.8284271247461903 * sqrt( reflected.z+1.0 );
    return reflected.xy / m + 0.5;
}

vec3 Decode( vec2 f )
{
    f = f * 2.0 - 1.0;

    // https://twitter.com/Stubbesaurus/status/937994790553227264
    vec3 n = vec3( f.x, f.y, 1.0 - abs( f.x ) - abs( f.y ) );
    float t = max( -n.z, 0.0 );
    n.xy -= sign(n.xy) * t;
    return normalize( n );
}

mat4 ortho(float l, float r, float b, float t, float n, float f)
{
    return mat4(
                2./(r-l),0,0,0,
                0,2./(t-b),0,0,
                0,0,2./(n-f),0,
                -(r+l)/(r-l),
                -(t+b)/(t-b),
                -(f+n)/(f-n),
                1.);
}

vec4 World2Clip(vec3 pos)
{
    float r = 1.5;
    mat3 ca = setCamera(vec3(0), normalize(vec3(1,2,3)), 0.);
    return ortho(-r,r,-r,r,-r,r) * vec4(pos*ca, 1.);
}

precision mediump sampler2DShadow;
uniform sampler2DShadow iChannel3;
float calcShadow(vec3 pos)
{
    float inShadow = 0.0;
    float count    = 0.0;
    vec2 mapSize = vec2(textureSize(iChannel3, 0));
    vec4 shadowPos = World2Clip(pos) * 0.5 + 0.5;
    for (int si = -2; si <= 2; ++si)
    for (int sj = -2; sj <= 2; ++sj)
    {
        count += 1.0;
        inShadow += textureProj( iChannel3, shadowPos + vec4(vec2(si,sj)/mapSize, vec2(-0.007,0.005)) );
    }

    return inShadow/count;
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
    d1 = pos.y;
    float id = 2.;
//    d2 = length(pos-vec3(0,1,0)) - .1;
//    if (d2 < d1) { d1=d2; id=1.; }
//    d2 = sdBox(pos-vec3(-.2,1,0), .3) - .02;
//    d1 = smin(d1,d2,.2);

    return vec2(d1, id);
}

vec3 calcNormal(vec3 pos)
{
    vec2 e = vec2(1.0,-1.0)*0.5773*0.001;
    return normalize(e.xyy*map( pos + e.xyy ).x +
                     e.yyx*map( pos + e.yyx ).x +
                     e.yxy*map( pos + e.yxy ).x +
                     e.xxx*map( pos + e.xxx ).x );
}

precision mediump sampler2DArray;
const float fov = 1.2;
uniform sampler2D iChannel0, iChannel2;
uniform sampler2DArray iChannel1;
out vec4 fragColor;
void main()
{
    vec2 screenUV = gl_FragCoord.xy/iResolution.xy;
    vec4 gBuffer1 = texture(iChannel1, vec3(screenUV, 0));
//    fragColor = texture( iChannel3, vec4(screenUV, vec2(0.0)).rg );

    vec3 ro = _ro, ta = _ta;
    vec2 uv = (2.0*gl_FragCoord.xy-iResolution.xy)/iResolution.y;
    mat3 ca = setCamera(ro, ta, 0.0);
    vec3 rd = ca * normalize(vec3(uv, fov));

    // compute rasterized polygon depth in world space
    float dep = texture(iChannel0, screenUV).r *2.0-1.0;
    vec3 nor = Decode(gBuffer1.rg);
    const float n = 0.1, f = 1000.0;
    const float p10 = (f+n)/(f-n), p11 = -2.0*f*n/(f-n); // from perspective matrix
        dep = p11 / ( (dep-p10) * dot(rd, normalize(ta-ro)) );

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

        float sha = calcShadow( ro + rd*min(t, dep) );

#define saturate(x) clamp(x,0.,1.)
        const vec3 sun_dir = normalize(vec3(1,2,3));
        float sun_dif = saturate(dot(nor, sun_dir))*.9+.1;
        float sky_dif = saturate(dot(nor, vec3(0,1,0)))*.15;
        col += vec3(0.9,0.9,0.5)*sun_dif*mate*sha;
        col += vec3(0.5,0.6,0.9)*sky_dif;
    } else {
        col += vec3(0.5,0.6,0.9)*1.2 - rd.y*.4;
    }

    col += i/50. * .1;
    col = pow(col, vec3(0.4545));
    fragColor = vec4(col, 1);

    { // depth test
        // convert camera dist to screen space dist
        float ssd = min(dep,t) * dot(rd, normalize(ta-ro));
        float ndc = p10+p11/ssd; // inverse of linear depth
        gl_FragDepth = (ndc*gl_DepthRange.diff + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
    }
}
