#ifdef SHADER_TYPE_GEOMETRY_SHADER

////////////////////////////////////////////////////////////////////////////////
// Viewport-array extension
////////////////////////////////////////////////////////////////////////////////
#if defined(EXT_GL_ARB_viewport_array) && defined(EXT_GL_NV_viewport_array2_ENABLED)
    // Number of VS invocations per primitive
    #define NUM_GS_INVOCATIONS_FOR_LAYERS 1

    // Handle the gl_Layer variable
    layout(viewport_relative) out int gl_Layer;

    // Whether we should skip this layer invocation or not
    bool shouldSkipLayerInvocation(const int layer, const bool singlepass)
    {
        return false;
    }

    // Function for handling the layer mask
    void handleLayerMask(const int layer, const bool singlepass)
    {
        if (singlepass) gl_ViewportMask[0] = (1 << sRenderData.iLayers) - 1;
        else            gl_ViewportMask[0] = (1 << layer);
    }

////////////////////////////////////////////////////////////////////////////////
// No viewport-array extension
////////////////////////////////////////////////////////////////////////////////
#else
    // Number of VS invocations per primitive
    #define NUM_GS_INVOCATIONS_FOR_LAYERS 8 

    // Whether we should skip this layer invocation or not
    bool shouldSkipLayerInvocation(const int layer, const bool singlepass)
    {
        return gl_InvocationID >= sRenderData.iLayers || (!singlepass && gl_InvocationID != layer);
    }

    // Function for handling the layer mask
    void handleLayerMask(const int layer, const bool singlepass)
    {
        gl_Layer = gl_InvocationID;
    }
#endif

////////////////////////////////////////////////////////////////////////////////
// Common functions
////////////////////////////////////////////////////////////////////////////////

// Whether we can run in single-pass mode or not
bool isSinglePass()
{
    return true;
}

// Layer invocation skip handling, with some defaults
bool shouldSkipLayerInvocation()
{
    return shouldSkipLayerInvocation(0, true);
}

// Layer mask handling with, with some defaults
void handleLayerMask()
{
    handleLayerMask(0, true);
}

#endif