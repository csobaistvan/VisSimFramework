// Smoothed min and max functions
float smin(float a, float b, float k)
{
    float diff = b - a;
    float h = clamp(0.5 + 0.5 * diff / k, 0.0, 1.0);
    return b - h * (diff + k * (1.0f - h));
}
 
float smax(float a, float b, float k)
{
    float diff = a - b;
    float h = clamp(0.5 + 0.5 * diff / k, 0.0, 1.0);
    return b + h * (diff + k * (1.0f - h));
}
