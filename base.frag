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

float map(vec3 pos)
{
    return sdBox(pos-vec3(-1,0,0), 1.);
}

vec3 calcNormal(vec3 pos)
{
    vec2 e = vec2(1.0,-1.0)*0.5773*0.001;
    return normalize(e.xyy*map( pos + e.xyy ) +
                     e.yyx*map( pos + e.yyx ) +
                     e.yxy*map( pos + e.yxy ) +
                     e.xxx*map( pos + e.xxx ) );
}

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
    vec3 nor = texture(iChannel1, gl_FragCoord.xy/iResolution.xy).rgb;
    float d = texture(iChannel0, gl_FragCoord.xy/iResolution.xy).r;
        d = p11/(d*2.-1. - p10) / dot(rd, normalize(ta-ro));

    // ray marching
    float t, i;
    for (t=0.,i=0.; i<50.; i++) {
        float h = map(ro + rd*t);
        t += h;
        if (h < 0.0001 || t > 100.) break;
    }

    vec3 col = vec3(0);

    // compare rasterized objects to raymarching objects
    if (t < 100. || d < 100.) {
        if (t < d) { // raymarching objects
            vec3 pos = ro + rd*t;
            nor = calcNormal(pos);
        }

        col += nor;
    }

    //col += min(t, d) / 20.;
    //col += i / 50.;

    fragColor = vec4(col, 1);
    return;

    { // depth test
        const float n = 0.1, f = 1000.0;
        float p10 = (f+n)/(f-n), p11 = -2.0*f*n/(f-n); // from perspective matrix
        float ssd = t * dot(rd, normalize(ta-ro)); // convert camera dist to screen space dist
        float ndc = p10+p11/ssd; // inverse of linear depth
        gl_FragDepth = (ndc*gl_DepthRange.diff + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
    }
}
