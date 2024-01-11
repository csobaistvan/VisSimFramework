function D = surface_diam( s, R, k )
%
% calculate maximum diameter of the surface defined by sag s, radius R,
% and conic constant k
%
% Copyright: Yury Petrov, 2016
%

if k == -1 % paraboloid
    D = sqrt( s * ( 8 * abs( R ) ) );
else % spheroids, hyperboloids
    if isinf( R )
        D = 0;
    else
        x = abs( R / ( 1 + k ) );
        D = sqrt( ( ( 4 * R^2 ) * abs( 1 - ( 1 + s / x ) .^ 2 ) ) / abs( 1 + k ) );
    end
end