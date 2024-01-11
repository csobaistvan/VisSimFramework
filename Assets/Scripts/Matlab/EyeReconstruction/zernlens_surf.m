function x = zernlens_surf( R, k, Z, y, z, ang, rad )

% evaluate the conic
a = 1 + k;
r2yz = y.^2 / R(1) + z.^2 / R(2);
if a == 0 % paraboloid, special case
    x = r2yz / 2;
else
    x = r2yz ./ ( 1 + sqrt( 1 - a * ( y.^2 / R(1)^2 + (R(2) / R(1) * z).^2 / R(2)^2 ) ) );
end

x = x + ZernikeEvalSurface( Z, rad, ang );
x( ~isreal( x ) ) = 1e+20; % prevent complex values

end
