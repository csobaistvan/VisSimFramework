// 'i'th element of the Halton sequence of base 'b'
float halton(int b, int i)
{
    float r = 0.0;
    float f = 1.0;
    while (i > 0)
    {
        f = f / float(b);
        r = r + f * float(i % b);
        i = int(floor(float(i) / float(b)));
    }
    return r;
}

// 'i'th element of the base2 Halton sequence
float halton2(const int i)
{
	//return halton(2, i);
	return float(bitfieldReverse(uint(i))) / 4294967296.0;
}

// 'i'th element of the Halton[2, 3] sequence
vec2 halton23(const int i)
{
    return vec2(halton2(i), halton(3, i));
}