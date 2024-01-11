#include <Shaders/OpenGL/Common/interpolation.glsl>

//TODO: implement 2D-4D

// Linear interpolation
float valueNoiseRandomNoise(const int x)
{
    const int xx = (x << 13) ^ x;
    return (1.0 - ((xx * (xx * xx * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);    
}

float valueNoiseSmoothedNoise(const float x)
{
    return valueNoiseRandomNoise(int(x)) / 2 + valueNoiseRandomNoise(int(x - 1)) / 4 + valueNoiseRandomNoise(int(x + 1)) / 4;
}

float valueNoiseInterpolatedNoise(const float x)
{
    int integer_X = int(x);
    float fractional_X = x - integer_X;

    float v1 = valueNoiseSmoothedNoise(integer_X);
    float v2 = valueNoiseSmoothedNoise(integer_X + 1);

    return cosineInterpolation(v1 , v2 , fractional_X);
}

// Value noise generator, based on the pseudo code provided by Hugo Elias
float valueNoise(const float x, const int numOctaves, const float persistence)
{
    // Control variables to loop over the octaves
    float amplitude = 1.0; // Current amplitude
    float maxAmplitude = 0.0; // Maximum possible amplitude
    float freq = 1.0; // Current frequency
    float noise = 0.0; // Sum of the noises

    // Evaluate each octave and compute the sum
    for (int i = 0; i < numOctaves; ++i)
    {
        noise += valueNoiseInterpolatedNoise(x * freq) * amplitude;
        maxAmplitude += amplitude;
        amplitude *= persistence;
        freq *= 2;
    }

    // Normalize the result by the maximum amplitude and return it
    return noise / maxAmplitude;
}

float valueNoise(const float x, const int numOctaves)
{
    return valueNoise(x, numOctaves, 0.5);
}

float valueNoise(const vec2 v, const int numOctaves)
{
    return valueNoise((v.x + v.y) * 0.5, numOctaves, 0.5);
}