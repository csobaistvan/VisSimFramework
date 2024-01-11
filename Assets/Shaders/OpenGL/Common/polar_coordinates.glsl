const vec3 sph2cart(const float azimuth, const float elevation, const float r)
{
    return r * vec3
    (
        sin(elevation) * cos(azimuth),
        cos(elevation),
        sin(elevation) * sin(azimuth)
    );
}