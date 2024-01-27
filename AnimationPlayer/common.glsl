
void f()
{
    vec3 dir = normalize(vec3(1,2,3));
    mat3 ca = setCamera(vec3(0), dir, 0.0);
    mat4 proj = ortho(-50, 50, -50, 50, -50, 50); // only apply 100 unit bounding box around origin
    mat4 lightvp = proj * ca;
}
