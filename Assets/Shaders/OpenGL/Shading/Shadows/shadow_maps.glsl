
////////////////////////////////////////////////////////////////////////////////
struct ShadowParameters
{
    vec2 uv;
    float depth;
    bool castsShadow;
    uint algorithm;
    uint smPrecision;
    float depthBias;
    float lightBleedBias;
    float momentsBias;
    float minVariance;
    vec2 exponentialConstants;
};

////////////////////////////////////////////////////////////////////////////////
float transformShadowMapDepth(const uint smAlgorithm, const float depth)
{
    //return depth;
    return depth * 2.0 - 1.0;
}

////////////////////////////////////////////////////////////////////////////////
const float MAX_ESM_EXPONENT_F16 = 5.54;
const float MAX_ESM_EXPONENT_F32 = 42.0;

////////////////////////////////////////////////////////////////////////////////
// Helper function for clamping the ESM exponent
float clampEsmExponent(const float exponent, const uint smPrecision)
{
    return clamp(exponent, 0.0, smPrecision == ShadowMapPrecision_F16 ? MAX_ESM_EXPONENT_F16 : MAX_ESM_EXPONENT_F32);
}

////////////////////////////////////////////////////////////////////////////////
vec2 clampEsmExponent(const vec2 exponents, const uint smPrecision)
{
    return clamp(exponents, vec2(0.0), vec2(smPrecision == ShadowMapPrecision_F16 ? MAX_ESM_EXPONENT_F16 : MAX_ESM_EXPONENT_F32));
}

////////////////////////////////////////////////////////////////////////////////
// Helper function for calculating the first and second moments
vec2 calculateMoments2(const float depth, const float dx, const float dy)
{
    return vec2(depth, depth * depth + 0.25 * (dx * dx + dy * dy));
}

// Helper function for calculating the first and second moments
vec4 calculateMoments4(const float depth, const float dx, const float dy)
{
    const float sqr = depth * depth;
    return vec4(depth, sqr, sqr * depth, sqr * sqr);
}

////////////////////////////////////////////////////////////////////////////////
vec4 calculateOptimizedMoments4(const float depth, const float dx, const float dy)
{
    const float square = depth * depth;
    const vec4 moments = vec4(depth, square, square * depth, square * square);
    vec4 optimized = moments * mat4(-2.07224649,     13.7948857237,  0.105877704,   9.7924062118,
                                     32.23703778,   -59.4683975703, -1.9077466311, -33.7652110555,
                                    -68.571074599,   82.0359750338,  9.3496555107,  47.9456096605,
                                     39.3703274134, -35.364903257,  -6.6543490743, -23.9728048165);
    optimized[0] += 0.035955884801;
    return optimized;
}

////////////////////////////////////////////////////////////////////////////////
vec4 calculateOriginalMoments4(vec4 optimizedMoments)
{
    optimizedMoments[0] -= 0.035955884801;
    return optimizedMoments * mat4(0.2227744146,  0.1549679261,  0.1451988946,  0.163127443,
                                   0.0771972861,  0.1394629426,  0.2120202157,  0.2591432266,
                                   0.7926986636,  0.7963415838,  0.7258694464,  0.6539092497,
                                   0.0319417555, -0.1722823173, -0.2758014811, -0.3376131734);
}

////////////////////////////////////////////////////////////////////////////////
// Basic shadow Fn.
float shadowBasic(const float currentDepth, const float closestDepth, const float depthBias)
{
    return step(currentDepth - depthBias, closestDepth);
}

////////////////////////////////////////////////////////////////////////////////
// Exponential shadow Fn.
float shadowExponential(const float currentDepth, const float closestDepth, const float exponentialConstant)
{
    return saturate(exp(exponentialConstant * (closestDepth - currentDepth)));
}

////////////////////////////////////////////////////////////////////////////////
// Variance shadow Fn.
float shadowVariance(const float currentDepth, const vec2 moments, const float minVariance, const float lightBleedBias)
{
    // Basic depth test
    const float p = step(currentDepth, moments.x);

    // Chebysev's ineq.
    const float sigma = max(moments.y - moments.x * moments.x, minVariance);
    const float d = currentDepth - moments.x;
    const float pMax = linstep(lightBleedBias, 1.0, sigma / (sigma + d * d));
    
    // Use the larger value
    return saturate(max(p, pMax));
}

////////////////////////////////////////////////////////////////////////////////
// Moments shadow Fn.
//
// SOURCE: https://github.com/TheRealMJP/Shadows
float shadowMoments(const float currentDepth, const vec4 moments, const float depthBias, const float lightBleedBias, const float momentsBias)
{
    // Bias input data to avoid artifacts
    const vec4 b = lerp(moments, vec4(0.5, 0.5, 0.5, 0.5), momentsBias);

    // Decision variables
    vec3 z = vec3(0.0);

    // First variable:  depth
    z[0] = currentDepth - depthBias;

    // Compute a Cholesky factorization of the Hankel matrix B storing only non-
    // trivial entries or related products
    const float L32D22 = mad(-b[0], b[1], b[2]);
    const float D22 = mad(-b[0], b[0], b[1]);
    const float squaredDepthVariance = mad(-b[1], b[1], b[3]);
    const float D33D22 = dot(vec2(squaredDepthVariance, -L32D22), vec2(D22, L32D22));
    const float InvD22 = 1.0 / D22;
    const float L32 = L32D22 * InvD22;

    // Obtain a scaled inverse image of bz = (1,z[0],z[0]*z[0])^T
    vec3 c = vec3(1.0, z[0], z[0] * z[0]);

    // Forward substitution to solve L*c1=bz
    c[1] -= b.x;
    c[2] -= b.y + L32 * c[1];

    // Scaling to solve D*c2=c1
    c[1] *= InvD22;
    c[2] *= D22 / D33D22;

    // Backward substitution to solve L^T*c3=c2
    c[1] -= L32 * c[2];
    c[0] -= dot(c.yz, b.xy);

    // Solve the quadratic equation c[0]+c[1]*z+c[2]*z^2 to obtain the second and third decision variables
    const float p = c[1] / c[2];
    const float q = c[0] / c[2];
    const float D = (p * p * 0.25f) - q;
    const float r = sqrt(D);
    z[1] =- p * 0.5 - r;
    z[2] =- p * 0.5 + r;

    // Compute the shadow intensity by summing the appropriate weights
    const vec4 switchVal = (z[2] < z[0]) ? vec4(z[1], z[0], 1.0, 1.0) :
                      ((z[1] < z[0]) ? vec4(z[0], z[1], 0.0, 1.0) :
                      vec4(0.0));
    const float quotient = (switchVal[0] * z[2] - b[0] * (switchVal[0] + z[2]) + b[1])/((z[2] - switchVal[1]) * (z[0] - z[1]));
    const float shadowIntensity = switchVal[2] + switchVal[3] * quotient;

    // Compute the result with light-bleedign correction applied
    return linstep(lightBleedBias, 1.0, 1.0 - saturate(shadowIntensity));
}

////////////////////////////////////////////////////////////////////////////////
// Samples the shadow map at the given light-space coordinates and computes the
// shadow term using the 'BASIC' algorithm
float sampleShadowBasic(sampler2D shadowMap, const vec2 uv, const float currentDepth, 
                        const float depthBias)
{
    // Extract the closest depth from the shadow map
    const float closestDepth = texture2D(shadowMap, uv).r;

    // Compare the supplied depth to the one in the shadow map
    return shadowBasic(currentDepth, closestDepth, depthBias);
}

////////////////////////////////////////////////////////////////////////////////
// Samples the shadow map at the given light-space coordinates and computes the
// shadow term using the 'VARIANCE' algorithm
float sampleShadowMapVariance(sampler2D shadowMap, const vec2 uv, const float currentDepth, 
                              const float minVariance, const float lightBleedBias)
{
    // Extract the closest depth from the shadow map
    const vec2 moments = texture2D(shadowMap, uv).rg;

    // Compare the supplied depth to the moments
    return shadowVariance(currentDepth, moments, minVariance, lightBleedBias);
}

////////////////////////////////////////////////////////////////////////////////
// Samples the shadow map at the given light-space coordinates and computes the
// shadow term using the 'EXPONENTIAL' algorithm
float sampleShadowMapExponential(sampler2D shadowMap, const vec2 uv, const float currentDepth, 
                                 const float exponentialConstant)
{
    // Extract the closest depth from the shadow map
    const float closestdepth = texture2D(shadowMap, uv).r;

    // Perform the shadow test using the exp depths
    return shadowExponential(currentDepth, closestdepth, exponentialConstant);
}

////////////////////////////////////////////////////////////////////////////////
// Samples the shadow map at the given light-space coordinates and computes the
// shadow term using the 'EXPONENTIAL VARIANCE' algorithm
float sampleShadowMapExponentialVariance(sampler2D shadowMap, const vec2 uv, const float currentDepth, 
                                         const uint smPrecision, const float minVarianceLinear, 
                                         const float lightBleedBias, const vec2 exponentialConstants)
{
    // Extract the closest depth from the shadow map
    const vec4 moments = texture2D(shadowMap, uv);

    // Clamp the exponential constants
    const vec2 clampedExponentialConstants = clampEsmExponent(exponentialConstants, smPrecision);

    // Compute the warped depth values
    const vec2 depthsWarped = vec2(exp(clampedExponentialConstants.x * currentDepth), -exp(-clampedExponentialConstants.y * currentDepth));

    // Compute the scaled min variance values
    const vec2 depthScale = minVarianceLinear * 0.01 * clampedExponentialConstants * depthsWarped;
    const vec2 minVariance = depthScale * depthScale;

    // Perform the VSM for both the positive and negative 
    const float pPos = shadowVariance(depthsWarped.x, moments.xy, minVariance.x, lightBleedBias);
    const float pNeg = shadowVariance(depthsWarped.y, moments.zw, minVariance.y, lightBleedBias);

    // Return the smaller one
    return min(pPos, pNeg);
}

////////////////////////////////////////////////////////////////////////////////
// Samples the shadow map at the given light-space coordinates and computes the
// shadow term using the 'MOMENTS' algorithm
float sampleShadowMapMoments(sampler2D shadowMap, const vec2 uv, const float currentDepth, 
                             const float depthBias, const float lightBleedBias, const float momentsBias)
{
    // Extract the closest depth from the shadow map
    //const vec4 moments = calculateOriginalMoments4(texture2D(shadowMap, uv));
    const vec4 moments = texture2D(shadowMap, uv);

    // Evaluate the moments condition
    return shadowMoments(currentDepth, moments, depthBias * 0.001, lightBleedBias, momentsBias * 0.001);
}

////////////////////////////////////////////////////////////////////////////////
// Samples the shadow map, given the parameter light-space coords
float sampleShadowMap(const sampler2D shadowMap, const ShadowParameters shadowParameters)
{
    // No shadow casting case
    if (!shadowParameters.castsShadow) return 1.0;

    // Invoke the basic algorithm
    if (shadowParameters.algorithm == ShadowMapAlgorithm_Basic)
        return sampleShadowBasic(shadowMap, shadowParameters.uv, 
            transformShadowMapDepth(shadowParameters.algorithm, shadowParameters.depth), 
            shadowParameters.depthBias);

    // Invoke the variance shadow map algorithm
    else if (shadowParameters.algorithm == ShadowMapAlgorithm_Variance)
        return sampleShadowMapVariance(shadowMap, shadowParameters.uv, 
            transformShadowMapDepth(shadowParameters.algorithm, shadowParameters.depth), 
            shadowParameters.minVariance, shadowParameters.lightBleedBias);

    // Invoke the exponential shadow map algorithm
    else if (shadowParameters.algorithm == ShadowMapAlgorithm_Exponential)
        return sampleShadowMapExponential(shadowMap, shadowParameters.uv, 
            transformShadowMapDepth(shadowParameters.algorithm, shadowParameters.depth), 
            shadowParameters.exponentialConstants.x);

    // Invoke the exponential variance shadow map algorithm, with exponential sampling
    else if (shadowParameters.algorithm == ShadowMapAlgorithm_ExponentialVariance)
        return sampleShadowMapExponentialVariance(shadowMap, shadowParameters.uv, 
            transformShadowMapDepth(shadowParameters.algorithm, shadowParameters.depth), 
            shadowParameters.smPrecision, shadowParameters.minVariance, 
            shadowParameters.lightBleedBias, shadowParameters.exponentialConstants);

    // Invoke the moments shadow map algorithm, with exponential sampling
    else if (shadowParameters.algorithm == ShadowMapAlgorithm_Moments)
        return sampleShadowMapMoments(shadowMap, shadowParameters.uv, 
            transformShadowMapDepth(shadowParameters.algorithm, shadowParameters.depth), 
            shadowParameters.depthBias, shadowParameters.lightBleedBias, 
            shadowParameters.momentsBias);

    // Default to visible
    return 1.0;
}