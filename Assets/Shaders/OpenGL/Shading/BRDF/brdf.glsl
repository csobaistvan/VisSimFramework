#include <Shaders/OpenGL/Shading/surface_descriptors.glsl>
#include <Shaders/OpenGL/Shading/BRDF/phong.glsl>
#include <Shaders/OpenGL/Shading/BRDF/blinn_phong.glsl>
#include <Shaders/OpenGL/Shading/BRDF/cook_torrance.glsl>

////////////////////////////////////////////////////////////////////////////////
vec3 applyBrdfDirect(const SurfaceInfo surface, const MaterialInfo material, const LightInfo light)
{
    // Phong model
    if (sRenderData.uiBrdfModel == BrdfModel_Phong)
        return brdfPhongDirect(surface, material, light);

    // Blinn-Phong model
    else if (sRenderData.uiBrdfModel == BrdfModel_BlinnPhong)
        return brdfBlinnPhongDirect(surface, material, light);

    // Cook-Torrance model
    else if (sRenderData.uiBrdfModel == BrdfModel_CookTorrance)
        return brdfCookTorranceDirect(surface, material, light);
}

////////////////////////////////////////////////////////////////////////////////
vec3 applyBrdfIndirect(const SurfaceInfo surface, const MaterialInfo material, const LightInfo light)
{
    // Phong model
    if (sRenderData.uiBrdfModel == BrdfModel_Phong)
        return brdfPhongIndirect(surface, material, light);

    // Blinn-Phong model
    else if (sRenderData.uiBrdfModel == BrdfModel_BlinnPhong)
        return brdfBlinnPhongIndirect(surface, material, light);

    // Cook-Torrance model
    else if (sRenderData.uiBrdfModel == BrdfModel_CookTorrance)
        return brdfCookTorranceIndirect(surface, material, light);
}