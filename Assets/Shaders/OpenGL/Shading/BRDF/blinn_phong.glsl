
////////////////////////////////////////////////////////////////////////////////
float diffuseBlinnPhong(const vec3 pos, const vec3 normal, const vec3 toLight)
{
    return saturate(dot(normal, toLight));
}

////////////////////////////////////////////////////////////////////////////////
float specularBlinnPhong(const vec3 pos, const vec3 normal, const vec3 toLight, const vec3 toEye)
{
    return saturate(dot(normalize(toLight + toEye), normal));
}

////////////////////////////////////////////////////////////////////////////////
vec3 brdfBlinnPhongDirect(const SurfaceInfo surface, const MaterialInfo material, const LightInfo light)
{
    // The vector pointing towards the camera
    const vec3 toEye = normalize(sCameraData.vEye - surface.position);
    const float specularPower = mix(sRenderData.fSpecularPowerMin, sRenderData.fSpecularPowerMax, 1.0 - material.roughness);
        
    // Calculate the diffuse and speculate intensities
    const float diffuseIntensity = diffuseBlinnPhong(surface.position, surface.normal, light.toLight);
    const float specularIntensity = pow(specularBlinnPhong(surface.position, surface.normal, light.toLight, toEye), specularPower);

    // Evaluate all the terms
    const vec3 ambientTerm = light.ambient * material.albedo;
    const vec3 diffuseTerm = diffuseIntensity *  light.diffuse * material.albedo;
    const vec3 specularTerm = specularIntensity * light.specular * material.specular;

    // Return the result
    return light.attenuation * light.shadow * (diffuseTerm + specularTerm) + ambientTerm;
}

////////////////////////////////////////////////////////////////////////////////
vec3 brdfBlinnPhongIndirect(const SurfaceInfo surface, const MaterialInfo material, const LightInfo light)
{
    return brdfBlinnPhongDirect(surface, material, light);
}