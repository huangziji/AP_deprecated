#version 300 es
precision mediump float;

layout (std140) uniform INPUT {
    vec4 iResolution;
    float iTime, iFrame, fov, _pad1;
    vec3 cameraPos, _pad2;
    vec3 targetPos, _pad3;
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
    return length(max(q, 0.0)) + min(0.0, max(max(q.x, q.y), q.z));
}

vec2 map(vec3 pos)
{
    float d = pos.y;
    float d2 = sdBox(pos-vec3(0,1,-2), 1.) - 0.02;

    d = min(d, d2);
    d2 = length(pos) - .15;
    d = min(d, d2);

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

#define saturate(x) clamp(x, 0., 1.)
out vec4 fragColor;
uniform sampler2D iChannel0, iChannel1;
void main()
{
    vec3 ro = cameraPos, ta = targetPos;
    vec2 uv = (2.0*gl_FragCoord.xy-iResolution.xy)/iResolution.y;
    mat3 ca = setCamera(ro, ta, 0.0);
    vec3 rd = ca * normalize(vec3(uv, fov));

    // compute rasterized polygon depth in world unit
    const float n = 0.1, f = 1000.0;
    const float p10 = (f+n)/(f-n), p11 = -2.0*f*n/(f-n); // from perspective matrix
    vec3 nor = texture(iChannel1, gl_FragCoord.xy/iResolution.xy).rgb * 2. - 1.;
    float d = texture(iChannel0, gl_FragCoord.xy/iResolution.xy).r;
        d = p11/(d*2.-1. - p10) / dot(rd, normalize(ta-ro));

    // ray marching
    float t, i;
    vec2 h;
    for (t=0.,i=0.; i<50.; i++) {
        h = map(ro + rd*t);
        t += h.x;
        if (h.x < 0.0001 || t > 100.) break;
    }

    float m = h.y;

    vec3 col = vec3(0);

    // compare rasterized objects to raymarching objects
    if (t < 100. || d < 100.) {
        vec3 mate;
        if (m < 0.5) {
            mate = vec3(0.5,0.7,0.6);
        } else if (m < 1.5) {
            mate = vec3(0.7,0.7,0.5);
        }

        if (t < d) { // raymarching objects
            vec3 pos = ro + rd*t;
            nor = calcNormal(pos);
        } else {
            mate = vec3(0.5,0.6,0.7);
        }

        const vec3 sun_dir = normalize(vec3(1,2,3));
        float sun_dif = saturate(dot(nor, sun_dir))*.9+.1;
        float sky_dif = saturate(dot(nor, vec3(0,1,0)))*.3;
        col += vec3(0.9,0.9,0.5)*sun_dif*mate*1.3;
        col += vec3(0.5,0.6,0.9)*sky_dif;
    } else {
        col += vec3(0.5,0.6,0.9)*1.2 - rd.y;
    }

    //col += i/50. * .2;

    col = pow(col, vec3(0.4545));
    fragColor = vec4(col, 1);

#if 0
    { // depth test
        const float n = 0.1, f = 1000.0;
        float p10 = (f+n)/(f-n), p11 = -2.0*f*n/(f-n); // from perspective matrix
        float ssd = t * dot(rd, normalize(ta-ro)); // convert camera dist to screen space dist
        float ndc = p10+p11/ssd; // inverse of linear depth
        gl_FragDepth = (ndc*gl_DepthRange.diff + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
    }
#endif
}
