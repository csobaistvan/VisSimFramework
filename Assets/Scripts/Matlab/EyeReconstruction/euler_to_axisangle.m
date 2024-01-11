function [ axis, angle ] = euler_to_axisangle( rot_angles )
    % Zero rotation
    if norm( rot_angles ) < 1e-3
        axis = [ 1, 0, 0 ];
        angle = 0;
        return;
    end
    
    % https://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToAngle/
    % sin and cosine of the angles
    c = cos( rot_angles ./ 2 );
    s = sin( rot_angles ./ 2 );
    % angle of rotation
    angle = 2 * acos( prod( c ) - prod( s ) );
    % axis of rotation
    axis = [ 0 0 0 ];
    axis( 1 ) = s( 1 ) * c( 2 ) * c( 3 ) + c( 1 ) * s( 2 ) * s( 3 );
    axis( 2 ) = c( 1 ) * s( 2 ) * c( 3 ) + s( 1 ) * c( 2 ) * s( 3 );
    axis( 3 ) = s( 1 ) * s( 2 ) * c( 3 ) + c( 1 ) * c( 2 ) * s( 3 );
    % normalize the axis
    axis = axis / norm( axis );
end