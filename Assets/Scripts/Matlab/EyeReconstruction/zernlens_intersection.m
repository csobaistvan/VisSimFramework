function rinter = zernlens_intersection( r_in, e, surf )

rinter = conic_intersection( r_in, e, surf );
options = optimoptions( 'fminunc', 'Display', 'off', ...
    'Algorithm', 'quasi-newton', 'HessUpdate', 'bfgs', ...
    'OptimalityTolerance', 1e-6, 'StepTolerance', 1e-6, ...
    'FiniteDifferenceType', 'forward', ...
    'MaxFunEvals', 1000, 'MaxIter', 1000 );
for i = 1 : size( r_in, 1 )
    if ~any( isinf( r_in( i, : ) ) ) && ~any( isnan( r_in( i, : ) ) )
        x0 = sqrt( sum( ( rinter( i, : ) - r_in( i, : ) ).^2 ) );
        if isinf( x0 ) || isnan( x0 )
            rinter( i, : ) = Inf;
        else
            % minimize a measure of distance between a ray point and the surface
            [ d, fval ] = fminunc( @dist2, x0, options, r_in( i, : ), e( i, : ), surf );
            if fval > 1e-2 || d < 0 % didn't intersect with the surface
                rinter( i, : ) = Inf;
            else
                rinter( i, : ) = r_in( i, : ) + e( i, : ) * d;
            end
        end
    end
end