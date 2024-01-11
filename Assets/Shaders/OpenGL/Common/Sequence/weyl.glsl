
vec2 weyl(const int i)
{
	//return fract(float(n) * vec2(0.754877669, 0.569840296));
    
    // integer mul to avoid round-off
    return fract(vec2(i * ivec2(12664745, 9560333)) / exp2(24.0));
}