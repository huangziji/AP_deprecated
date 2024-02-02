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

vec3 palette( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
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
    d1 = pos.y + .05;
    float id = 1.;
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

vec4 World2Clip(vec3 pos, vec3 rd)
{
    const vec3 ro = vec3(0);
    const float ie = 1./5.;
    mat3 ca = setCamera(ro, rd, 0.);
    return vec4(ie,ie,-ie, 1.) * vec4((pos-ro)*ca, 1.);
}

precision mediump sampler2DShadow;
uniform sampler2DShadow iChannel3;
float calcShadow(vec3 ro, vec3 rd)
{
    float sha = 0.0;
    float count = 0.0;
    vec2 mapSize = vec2(textureSize(iChannel3, 0));
    vec4 shadowPos = World2Clip(ro, rd) * 0.5 + 0.5;
    for (int si = -2; si <= 2; ++si)
    for (int sj = -2; sj <= 2; ++sj)
    {
        vec4 sp = shadowPos + vec4(vec2(si,sj)/mapSize, vec2(-0.003,0.001));
        sha += textureProj( iChannel3, sp );
        count += 1.0;
    }
    return sha/count;
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
    vec4 gBuffer2 = texture(iChannel1, vec3(screenUV, 1));

    vec3 ro = _ro, ta = _ta;
    vec2 uv = (2.0*gl_FragCoord.xy-iResolution.xy)/iResolution.y;
    mat3 ca = setCamera(ro, ta, 0.0);
    vec3 rd = ca * normalize(vec3(uv, fov));

    // compute rasterized polygon depth in world space
    float dep = texture(iChannel0, screenUV).r *2.0-1.0;
    vec3 nor = gBuffer1.rgb;
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
        vec3 mate = palette(m/4., vec3(0.5,0.7,0.6), vec3(0.5,0.4,0.7),
                vec3(0.7,0.7,0.5), vec3(.8,.7,.7));

        if (t < dep)
        { // raymarching objects
            vec3 pos = ro + rd*t;
            nor = calcNormal(pos);
        }
        else
        { // polygons
            vec2 uv = matcap(rd, nor);
            mate = textureLod(iChannel2, uv, 2.0).rgb;
            float id = gBuffer2.r * float(0xff);
            mate *= 1.7 * palette(id/20., vec3(0.5,0.7,0.6), vec3(0.5,0.4,0.7),
                    vec3(0.7,0.7,0.5), vec3(.8,.7,.1));
        }

        const vec3 sun_dir = normalize(vec3(1,2,3));
        float sha = calcShadow( ro + rd*min(t, dep) + nor*.006, sun_dir ) * .7 + .3;

#define saturate(x) clamp(x,0.,1.)
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
