function ell = MinVolEllipseFit(P, tolerance )
% ell = MinVolEllipseFit(P, tolerance)
%--------------------------------------------------------------------------
% Wrapper around MinVolEllipse. Finds the minimum volume enclsing ellipsoid 
% (MVEE) of a set of data points stored in matrix P. 
%
% P : (N x d) dimnesional matrix containing N points in R^d.
% tolerance : error in the solution with respect to the optimal value.
%
% outputs:
%---------
% ell : [ a b r R theta ] is the fitting ellipse:
%                         center (a,b), radius (r,R) and angle (theta)
%--------------------------------------------------------------------------

%[ A, c ] = MinVolEllipse( [ P( :, 1 ), P( :, 2 ) ]', tolerance );
[ A, c ] = MinVolEllipse( P( :, 1:2 )', tolerance );
x0 = c; % center of the ellipse

[ ~, Q, V ] = svd( A ); % also determine the radii and ellipse angle
r = [ 1 / sqrt( Q( 1, 1 ) ), 1 / sqrt( Q( 2, 2 ) ) ];
r = [ min( r ), max( r ) ]; % sort the radii
ang = atan2( V( 1, 2 ), V( 1, 1 ) );

% write out the results
ell = [ x0', r, ang ];