function x = zernlens( y, z, args, flag )
% ZERNLENS defines a ...
% On flag == 0 the function 
% should return the lens height x for the given position ( y, z ). Otherwise,
% the function should return the lens normal at this position. By convention, 
% the normal should point along the x-axis, i.e. in the same general 
% direction as the traced ray.
%

R = args( 1:2 );
D = args(3);
k = args(4);
Z = args( 5:end );
[ ang, rad ] = cart2pol(y, z);

% compute the surface elevation
if flag == 0
    x = zernlens_surf( R, k, Z, y, z, ang, rad / ( D / 2 ) );
    
% compute the surface normal
else    
    delta = 1e-8;
    rad_delta = delta * ( D / 2 );
    ang_delta = delta * ( 2 * pi );
    
    % evalute ourselves and the neighbors
    rads = [ rad - rad_delta, rad + rad_delta, rad - rad_delta ];
    angs = [ ang - ang_delta, ang - ang_delta, ang + ang_delta ];
    [ y0, z0 ] = pol2cart( angs, rads );
    x0 = zernlens_surf( R, k, Z, y0, z0, angs, rads / ( D / 2 ) );
    
    % 3 vertices in a triangle
    p0 = [ x0( :, 1 ) y0( :, 1 ) z0( :, 1 ) ];
    p1 = [ x0( :, 2 ) y0( :, 2 ) z0( :, 2 ) ];
    p2 = [ x0( :, 3 ) y0( :, 3 ) z0( :, 3 ) ];
    
    % cross product to get the normal
    x = cross( p1 - p0, p2 - p0 );
    
    % flip normals, if necessary
    tf = x( :, 1 ) < 0;
    x( tf ) = x( tf ) * -1;
    
    % correct the [ 0 0 0 ] case (center of the lens)
    x( isnan( x( :, 1 ) ), 1 ) = 1;
    x( isnan( x( :, 2 ) ), 2 ) = 0;
    x( isnan( x( :, 3 ) ), 3 ) = 0;
end

end
