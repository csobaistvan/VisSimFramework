 classdef ZernikeLens < GeneralLens
    % ZERNIKELENS Implements a lens surface given by a rotation of a conic curve
    % with additional polynomial terms
    % R is the tangent sphere radius, k is the aspheric factor:
    % 0 < k - oblate spheroid
    % k = 0 - sphere
    % -1 < k < 0 - prolate spheroid
    % k = -1 - parabola
    % k < -1 - hyperbola
    % and a(1) ... a(n) are the Zernike coefficients
    %
    % Member functions:
    %
    % l = ZernikeLens( r, D, R, k, avec, glass ) - object constructor
    % INPUT:
    % r - 1x3 position vector
    % D - diameter
    % R - tangent sphere radius, [ Ry Rz ] vector for an astigmatic surface
    % k - conic coefficient, for astigmatic surface corresponds to the y-axis
    % avec - a vector of the Zernike coefficients terms
    % glass - 1 x 2 cell array of strings, e.g., { 'air' 'acrylic' }
    % OUTPUT:
    % l - lens surface object
    %
    % l.display() - displays the surface l information
    %
    % l.draw() - draws the surface l in the current axes
    %
    % l.rotate( rot_axis, rot_angle ) - rotate the surface l
    % INPUT:
    %   rot_axis - 1x3 vector defining the rotation axis
    %   rot_angle - rotation angle (radians)
    % 
    % Copyright: Yury Petrov, 2016
    %
    
    properties
        avec = []; % vector of Zernike coefficients
    end
    
    methods
        function self = ZernikeLens( ar, aD, aR, ak, aavec, aglass )
            if nargin == 0
                return;
            end
            if size( aD, 1 ) < size( aD, 2 )
                aD = aD';
            end
            if size( aD, 1 ) == 1
                aD = [ 0; aD ];
            end
            self.r = ar;
            self.D = aD;
            self.R = aR;
            self.k = ak;
            if size( aavec, 1 ) > size( aavec, 2 )
                aavec = aavec';
            end
            self.avec = aavec;
            self.glass = aglass;
            self.funcs = 'zernlens';
            self.funch = str2func( self.funcs ); % construct function handle from the function name string
            if length( self.R ) == 1 % no astigmatism
                self.funca = [ self.R self.R self.D(2) self.k, self.avec ];
            else
                self.funca = [ self.R(1) self.R(2) self.D(2) self.k self.avec ];
            end
        end
        
        function display( self )
            fprintf( 'Position:\t [%.3f %.3f %.3f]\n', self.r );
            fprintf( 'Orientation:\t [%.3f %.3f %.3f]\n', self.n );
            fprintf( 'Diameter:\t %.3f\n', self.D );
            fprintf( 'Curv. radius:\t %.3f\n', self.R );
            fprintf( 'Asphericity:\t %.3f\n', self.k );
            fprintf( 'Polynomial coefficients:' );
            disp( self.avec );
            fprintf( 'Material:\t %s | %s\n', self.glass{ 1 }, self.glass{ 2 } );
        end
        
        function h = draw( self, color, plot_type )
            % DISPLAY the lens surface
            if nargin < 2
                color = [ 1 1 1 .5 ];
            end
            h = self.draw@GeneralLens( color, plot_type );
        end
    end
    
 end
