classdef EyeParametric < Bench
    % EYE implements human eye optics
    %   The human eye model is described in Escudero-Sanz & Navarro, 
    %   "Off-axis aberrations of a wide-angle schematic eye model", 
    %   JOSA A, 16(8), 1881-1891 (1999). Also, Dubbelman M, Van der 
    %   Heijde GL. "The shape of the aging human lens: curvature, 
    %   equivalent refractive index and the lens paradox." Vision Res. 
    %   2001;41:1867?1877 and A. V. Goncharov and C. Dainty. Wide-field 
    %   schematic eye models with gradient-index lens. J Opt Soc Am A 
    %   Opt Image Sci Vis, 24(8):2157?74, 2007. were used.
    %   
    %   Following  G. K. Von Noorden and E. C. Campos. "Binocular 
    %   vision and ocular motility: theory and management of strabismus." 
    %   Gunter K. 6th ed. St. Louis: CV Mosby, 2002, the eye center of 
    %   rotation was taken to be 13.3 mm behind the corneal apex, which 
    %   puts it 1.34 mm behind the center of the eye (total eye depth is
    %   23.93 here). High precision is immaterial since eye center
    %   of rotation actually moves by as much as 1 mm as the eye rotates,
    %   so the idea of the eye center of rotation is just an approximation.
    %   The retina shape was taken to be an oblate spheroid according to 
    %   D. A. Atchison et al. "Shape of the retinal surface in emmetropia 
    %   and myopia". Invest Ophthalmol Vis Sci, 46(8):2698?707, Aug 2005.
    %   The retinal spheroid and lens slants were ignored here.
    %
    %   Lens accomodation is modeled by its diameter variation assuming 
    %   that the lens volume is constant for all diameters. This assumption
    %   was based on Hermans et al., "Constant Volume of the Human Lens and 
    %   Decrease in Surface Area of the Capsular Bag during Accommodation: 
    %   An MRI and Scheimp?ug Study", Investigative Ophthalmology & Visual 
    %   Science 50(1), 281-289 (2009).
    %
    %   The back lens surface was modeled by a paraboloid of revolution 
    %   x = 1/(2 R) ( y^2 + z^2 ), its (constant) volume V is 
    %   given by pi/8 D^2 h, where h is the paraboloid height. Hence, 
    %   h(D) = 8 V / (pi D^2) and R(D) = pi D^4 / (64 V). The front lens 
    %   surface was modeled by a hyperboloid of revolution given by
    %   x = R/(1+k) (1 - sqrt( 1 - a( y^2 + z^2)/R^2 ) ). The volume 
    %   is pi/8 D^2 h (1 - h/(6 R/(1+k) + 3h) ), where h is the height of
    %   of the hyperboloid from it apex to the cut of diameter D. 
    %   Corresponding h(D) and R(D) formulas were obtained by solving a cubic 
    %   equation, and its closed-from solution is implemented here.
        
    properties
        Name            = ""         % name of the eye model
        Description     = ""         % additional information about the eye
        % cornea
        Cornea1Z1       = 0          % anterior cornea elevation coefficients
        Cornea1Z2       = 0
        Cornea1Z3       = 0
        Cornea1Z4       = 0
        Cornea1Z5       = 0
        Cornea1Z6       = 0
        Cornea1Z7       = 0
        Cornea1Z8       = 0
        Cornea1Z9       = 0
        Cornea1Z10      = 0
        Cornea1Z11      = 0
        Cornea1Z12      = 0
        Cornea1Z13      = 0
        Cornea1Z14      = 0
        Cornea1Z15      = 0
        Cornea1Z16      = 0
        Cornea1Z17      = 0
        Cornea1Z18      = 0
        Cornea1Z19      = 0
        Cornea1Z20      = 0
        Cornea1Z21      = 0
        Cornea1Z22      = 0
        Cornea1Z23      = 0
        Cornea1Z24      = 0
        Cornea1Z25      = 0
        Cornea1Z26      = 0
        Cornea1Z27      = 0
        Cornea1Z28      = 0
        Cornea1Z        = []
        CorneaD1        = 11.5       % anterior cornea diameter, mm
        CorneaD2        = 11.5       % posterior cornea diameter, mm
        Corneadx        = 0.0        % cornea decenter x
        Corneady        = 0.0        % cornea decenter y
        Corneaax        = 0.0        % cornea tilt x
        Corneaay        = 0.0        % cornea tilt y
        Cornea1Th       = 0.0        % cornea axial rotation
        Cornea2Th       = 0.0        % cornea axial rotation
        Cornea1k        = -0.35      % front conic constant
        Cornea2k        = -0.01      % back conic constant
        Cornea1R        = 7.71       % front surface radius of curvature
        Cornea1R1       = 0.0        % front surface radius of curvature #1
        Cornea1R2       = 0.0        % front surface radius of curvature #2
        Cornea1Ra       = 1.0        % front surface astigmatism (R2/R1)
        Cornea2R        = 0.0        % back surface radius of curvature
        Cornea2R1       = 0.0        % back surface radius of curvature #1
        Cornea2R2       = 0.0        % back surface radius of curvature #2
        Cornea2Ra       = 1.0        % back surface astigmatism (R2/R1)
        CorneaRr        = 0.830      % ratio of posterior and anterior radii of curvature (R2/R1)
        % lens
        LensD           = 9.6        % lens diameter
        LensD2          = 9.6        % lens diameter (actual)
        Lensdx          = 0.0        % lens decenter x
        Lensdy          = 0.0        % lens decenter y
        Lensax          = 0.0        % lens tilt x
        Lensay          = 0.0        % lens tilt y
        LensTh          = 0.0        % lens axial rotation
        Lens1R          = 0.0        % anterior lens radius
        Lens1R1         = 0.0        % anterior lens radius #1
        Lens1R2         = 0.0        % anterior lens radius #2
        Lens1k          = -4.0       % anterior lens conic constant
        Lens2R          = 0.         % posterior lens radius
        Lens2R1         = 0.0        % posterior lens radius #1
        Lens2R2         = 0.0        % posterior lens radius #2
        Lens2k          = -3.0       % anterior lens conic constant
        LensTr          = 0.60       % ratio between anterior and posterior thicknesses
        LensV           = 160.1      % constant volume of the entire lens, mm^3
        LensT1          = 0.0        % thickness of the anterior part
        LensT2          = 0.0        % thickness of the posterior part
        LensV1          = 0.0        % constant volume of the anterior lens part, mm^3
        LensV2          = 0.0        % constant volume of the posterior lens part, mm^3
        LensDc          = 1.0        % lens diameter compensation
        LensVc          = 1.0        % lens volume compensation
        LensTc          = 1.0        % lens thickness compensation
        % pupil
        PupilD          = 3          % 3 mm pupil diameter by default
        PupilD2         = 0          % pupil diameter used for drawing
        % thicknesses
        EyeT           = 24.743      % total eye length
        CorneaT        = 0.55        % cornea
        AqueousT       = 3.05        % aqueous humour
        LensT          = 0.0         % crystalline lens
        VitreousT      = 0.0         % vitreous chamber
        % retina
        RetinaR        = -12.0       % retina radius of curvature
        Retinak        = 0           % emmetropic retina is oblong
        % locations of the elements along the optical axis
        Corneax        = 0           % cornea
        Pupilx         = 0           % pupil
        Lensx          = 0           % lens
        Retinax        = 0           % retina
        % element counts & indices
        IdCornea1      = 1
        IdCornea2      = 2
        IdPupil        = 3
        IdLens1        = 4
        IdLens2        = 5
        IdRetina       = 6
        NumSurfaces    = 6
        % induced aberrations
        Aberrations    = 0           % structure holding all the aberration information
        Lambda         = 587.56      % wavelength at which the aberration was measured
        AngleHor       = 0           % horizontal angle (in degrees) at which the aberration was measured
        AngleVert      = 0           % vertical angle (in degrees) at which the aberration was measured
        Alpha          = []          % Zernike aberration coefficients of the induced aberrations (optical path difference)
    end

    methods(Static)
        
        function D = MaxSurfaceDiameter( R, k )
            D = 1.99 / sqrt( ( 1 + k ) / ( R * R ) );
        end
        
        function d = CorneaDepth( D, R1, R2, k )
            d = max( abs( surface_sag( D, R1, k ) ), abs( surface_sag( D, R2, k ) ) );
        end
        
        function D = FixSurfaceDiameter( D, R1, R2, k )
            if 1 + k > 0
                D = min( [ D, EyeParametric.MaxSurfaceDiameter( R1, k ), EyeParametric.MaxSurfaceDiameter( R2, k ) ] );
            end
        end
        
        function [LensR, hf] = LensRadius( D, V, k, sgn )
            if nargin < 4, sgn = 1.0; end
            
            if k == -1
                LensR = sgn * pi * D^4 / ( 64 * V );
                hf = 8 * V / ( pi * D^2 );
            else
                a = 1 + k;
                A = 5 * D^6 * pi^2 - 1024 * a * V^2;
                d = a^3 * D^6 * ( 5 * D^12 * pi^4 + 2112 * a * D^6 * pi^2 * V^2 + 1572864 * a^2 * V^4 );
                B = ( -360 * a^2 * D^6 * pi^2 * V - 32768 * a^3 * V^3 + 5 * pi * sqrt( d ) )^(1/3);
                C = 10 * pi * D^2;
                
                % below are the 3 real roots of the cubic equation for the surface height, the 3d root gives the height
                %h1 = ( 32 * V + A / B - B / a  ) / C;  
                %h2 = ( 64 * V + ( 1 + 1i * sqrt(3) ) * (-A) / B + ( 1 - 1i * sqrt(3) ) * B / a ) / ( 2 * C );
                h3 = ( 64 * V + 1i * ( 1i + sqrt(3) ) * A / B + ( 1 + 1i * sqrt(3) ) * B / a ) / ( 2 * C );
                hf = real( h3 );  % remove zero imaginary part
                LensR = sgn * ( a * hf / 2 + D^2 / ( 8 * hf ) );
            end
        end
        
        function V = ConeVolume( t, R, r )
            if nargin <= 2, r = R; end
            V = (1/3) * pi * t * (r.^2 + r * R + R.^2);
        end
        
        function V = LensVolume( D, R, k )
            if k == -1
                h = abs( D^2 / ( 8 * R ) );
                V = pi / 8 * D^2 * h;
            else
                x = R / ( 1 + k );
                h = x + sqrt( x^2 - D^2 / ( 4 * ( 1 + k ) ) );
                V = pi * D^2 / 8 * h * ( 1 - h / ( 6 * R / ( 1 + k ) + 3 * h ) );
            end
        end
    end
    
    methods
        
        function self = EyeParametric( )
        end
        
        function DeleteSelf( self )
            delete( self );
        end

        function self = SetEyeParameter( self, name, value )
            self.( name ) = value;
        end
        
        function self = SetEyeParameters( self, names, values )
            for i = 1:length( values )
                if isprop( self, names{ i } )
                    self.( names{ i } ) = values{ i };
                end
            end
        end
        
        function value = GetEyeParameter( self, name )
            value = self.( name );
        end
        
        % correct anterior cornea diameter
        function D = ComputeCornea1Diameter( self, max_cornea_diameter )
            if nargin < 2, max_cornea_diameter = 12.4; end
            
            % ensure that the diameter is not larger than physically possible
            D = EyeParametric.FixSurfaceDiameter( max_cornea_diameter, self.Cornea1R1, self.Cornea1R2, self.Cornea1k );
        end
        
        % correct posterior cornea diameter
        function D = ComputeCornea2Diameter( self )
            % start from the anterior cornea's diameter
            D = EyeParametric.FixSurfaceDiameter( self.CorneaD1, self.Cornea2R1, self.Cornea2R2, self.Cornea2k );
        
            % modify the diameter such that the distance between the
            % anterior and posterior cornea is equal to CorneaT
            %d = EyeParametric.CorneaDepth( self.CorneaD1, self.Cornea1R1, self.Cornea1R2, self.Cornea1k ) - self.CorneaT;
            d = EyeParametric.CorneaDepth( self.CorneaD1, self.Cornea1R1, self.Cornea1R2, self.Cornea1k );
            while D > 0 && EyeParametric.CorneaDepth( D, self.Cornea2R1, self.Cornea2R2, self.Cornea2k ) > d
                D = D - 0.01;
            end
        end
        
        % cornea parameters
        function self = DeriveCorneaParameters( self )
            % copy over the cornea radii of curvature
            if self.Cornea1R ~= 0 && self.Cornea1R1 == 0 && self.Cornea1R2 == 0
                self.Cornea1R1 = self.Cornea1R;
                self.Cornea1R2 = self.Cornea1R * self.Cornea1Ra;
            end
            
            if self.CorneaRr ~= 0 && self.Cornea2R == 0 && self.Cornea2R1 == 0 && self.Cornea2R2 == 0
                self.Cornea2R = self.CorneaRr * self.Cornea1R;
                self.Cornea2R1 = self.Cornea2R;
                self.Cornea2R2 = self.Cornea2R * self.Cornea2Ra;
            end
            
            if self.Cornea2R ~= 0 && self.Cornea2R1 == 0 && self.Cornea2R2 == 0
                self.Cornea2R1 = self.Cornea2R;
                self.Cornea2R2 = self.Cornea2R * self.Cornea2Ra;
            end
            
            % cornea diameters
            self.CorneaD1 = self.ComputeCornea1Diameter( );
            self.CorneaD2 = self.ComputeCornea2Diameter( );
            
            % cornea zernike coeffs
            self.Cornea1Z = ...
            [...
                self.Cornea1Z1, self.Cornea1Z2, self.Cornea1Z3, ...
                self.Cornea1Z4, self.Cornea1Z5, self.Cornea1Z6, ...
                self.Cornea1Z7, self.Cornea1Z8, self.Cornea1Z9, ...
                self.Cornea1Z10, self.Cornea1Z11, self.Cornea1Z12, ...
                self.Cornea1Z12, self.Cornea1Z4, self.Cornea1Z15, ...
                self.Cornea1Z16, self.Cornea1Z17, self.Cornea1Z18, ...
                self.Cornea1Z19, self.Cornea1Z20, self.Cornea1Z21, ...
                self.Cornea1Z22, self.Cornea1Z23, self.Cornea1Z24, ...
                self.Cornea1Z25, self.Cornea1Z26, self.Cornea1Z27, ...
                self.Cornea1Z28
            ];
            self.Cornea1Z = self.Cornea1Z( 1 : find( self.Cornea1Z, 1, 'last' ) );
        end
        
        % lens parameters
        function self = DeriveLensParameters( self )            
            % set the various lens compensation terms
            self.LensTc = 10 / (self.LensD - 4);
            self.LensVc = self.LensTc * 0.3;
            
            % determine the lens parameters
            step_size = 10.0;
            batches_left = 5;
            
            self.LensV1 = 0.0;
            keep_looping = true;
            while keep_looping
                % compute the new lens volumes
                self.LensV1 = self.LensV1 + step_size;
                self.LensV2 = self.LensV - self.LensV1;
                
                % lens thickness and curvature                
                [self.Lens1R, self.LensT1] = self.LensRadius( self.LensD * self.LensDc, self.LensV1 * self.LensVc, self.Lens1k, 1.0 );
                [self.Lens2R, self.LensT2] = self.LensRadius( self.LensD * self.LensDc, self.LensV2 * self.LensVc, self.Lens2k, -1.0 );
                
                % calculate the ratio and evaluate the exit condition
                ratio = self.LensT1 / self.LensT2;
                if ratio >= self.LensTr
                    if batches_left > 1
                        self.LensV1 = self.LensV1 - step_size;
                        step_size = step_size * 0.1;
                        batches_left = batches_left - 1;
                    else
                        keep_looping = false;
                    end
                end
            end
            
            % copy over the curvatures
            self.Lens1R1 = self.Lens1R;
            self.Lens1R2 = self.Lens1R;
            self.Lens2R1 = self.Lens2R;
            self.Lens2R2 = self.Lens2R;
            
            % apply compensation and calculate the total lens thickness
            t1_comp = ( self.LensT1 / ( self.LensT1 + self.LensT2 )) * self.LensTc;
            t2_comp = ( self.LensT2 / ( self.LensT1 + self.LensT2 )) * self.LensTc;
            self.LensT1 = self.LensT1 + t1_comp;
            self.LensT2 = self.LensT2 + t2_comp;
            self.LensT = self.LensT1 + self.LensT2;
            
            % calculate the actual lens diameter
            self.LensD2 = ( surface_diam( self.LensT1, self.Lens1R, self.Lens1k ) + surface_diam( self.LensT2, self.Lens2R, self.Lens2k ) ) / 2;
            
            %fprintf( 'V1: %f - %f, T1: %f\n', self.LensV1, self.LensVolume( self.LensD, self.Lens1R, self.Lens1k ), self.LensT1 );
            %fprintf( 'V2: %f - %f, T2: %f\n', self.LensV2, self.LensVolume( self.LensD, abs( self.Lens2R ), self.Lens2k ), self.LensT2 );
        end
        
        % pupil parameters
        function self = DerivePupilParameters( self )
            self.PupilD2 = max( self.CorneaD1, self.PupilD * 1.3 );
        end
            
        % vitreous chamber parameters
        function self = DeriveVitreousParameters( self )
            self.VitreousT = self.EyeT - self.LensT - self.AqueousT - self.CorneaT;
        end
        
        % retina parameters
        function self = DeriveRetinaParameters( self )
            self.RetinaR = -0.5 * self.EyeT;
        end
        
        % derives all the eye parameters
        function self = DeriveParameters( self )
            self.DeriveCorneaParameters( );
            self.DerivePupilParameters( );
            self.DeriveLensParameters( );
            self.DeriveVitreousParameters( );
            self.DeriveRetinaParameters( );
        end
        
        function cornea1 = MakeCornea1( self )
            self.Corneax = 0;
            if isempty( self.Cornea1Z )
                cornea1 = Lens( [ self.Corneax self.Corneadx self.Corneady ], self.CorneaD1, ...
                    [ self.Cornea1R1, self.Cornea1R2 ], self.Cornea1k, { 'air' 'cornea' } );
            else
                cornea1 = ZernikeLens( [ self.Corneax self.Corneadx self.Corneady ], self.CorneaD1, ...
                    [ self.Cornea1R1, self.Cornea1R2 ], self.Cornea1k, self.Cornea1Z, { 'air' 'cornea' } );
            end
            cornea1.rotate_euler( deg2rad( [ self.Cornea1Th, self.Corneaay, self.Corneaax ] ) );
        end
        
        function cornea2 = MakeCornea2( self )
            self.Corneax = 0;
            offset = self.CorneaT * self.elem{ self.IdCornea1 }.n;
            cornea2 = Lens( [ self.Corneax self.Corneadx self.Corneady ] + offset, self.CorneaD2, ...
                [ self.Cornea2R1, self.Cornea2R2 ], self.Cornea2k, { 'cornea' 'aqueous' } );
            cornea2.rotate_euler( deg2rad( [ self.Cornea2Th, self.Corneaay, self.Corneaax ] ) );
        end
        
        function pupil = MakePupil( self )
            self.Pupilx = self.CorneaT + self.AqueousT;
            pupil = Aperture( [ self.Pupilx 0 0 ], [ self.PupilD self.PupilD2 ] );    
        end
        
        function lens1 = MakeLens1( self )
            self.Lensx = self.CorneaT + self.AqueousT;
            lens1 = Lens( [ self.Lensx self.Lensdx self.Lensdy ], self.LensD2, ...
                [ self.Lens1R1, self.Lens1R2 ], self.Lens1k, { 'aqueous' 'lens' } );
            lens1.rotate_euler( deg2rad( [ self.LensTh, self.Lensay, self.Lensax ] ) );
        end
        
        function lens2 = MakeLens2( self )
            self.Lensx = self.CorneaT + self.AqueousT;
            offset = self.LensT * self.elem{ self.IdLens1 }.n;
            lens2 = Lens( [ self.Lensx self.Lensdx self.Lensdy ] + offset, self.LensD2, ...
                [ self.Lens2R1, self.Lens2R2 ], self.Lens2k, { 'lens' 'vitreous' } );
            lens2.rotate_euler( deg2rad( [ self.LensTh, self.Lensay, self.Lensax ] ) );
        end
        
        function retina = MakeRetina( self )
            self.Retinax = self.CorneaT + self.AqueousT + self.LensT + self.VitreousT;
            retina = Retina( [ self.Retinax 0 0 ], self.RetinaR, self.Retinak, 0.55 * pi / 2 );
        end
        
        function self = MakeElementsOnly( self )
            self.elem = {};
            self.elem{ self.IdCornea1 } = self.MakeCornea1( );
            self.elem{ self.IdCornea2 } = self.MakeCornea2( );
            self.elem{ self.IdPupil } = self.MakePupil( );
            self.elem{ self.IdLens1 } = self.MakeLens1( );
            self.elem{ self.IdLens2 } = self.MakeLens2( );
            self.elem{ self.IdRetina } = self.MakeRetina( );
            self.cnt = self.NumSurfaces;
        end
        
        function self = MakeElements( self )
            % compute the necessary eye parameters
            self.DeriveParameters( );
            
            % Construct the elements
            self.MakeElementsOnly( );
        end
        
        function self = SetLensDiameter( self, lens_diameter )
            % modify the lens diameter
            prev_lens_t1 = self.LensT1;
            self.LensD = lens_diameter;
            self.DeriveLensParameters( );
            self.DeriveVitreousParameters( );
            
            % update the Aqueous and Vitreous thicknesses
            offset = (self.LensT1 - prev_lens_t1);
            self.AqueousT = self.AqueousT - offset;
            self.VitreousT = self.VitreousT + offset;
                       
            % re-make the pupil and the lenses
            self.elem{ self.IdPupil } = self.MakePupil( );
            self.elem{ self.IdLens1 } = self.MakeLens1( );
            self.elem{ self.IdLens2 } = self.MakeLens2( );
        end
        
        function [ result_eye, lens_diam, coc, dv, ld ] = FocusAt( self, fdist, lambda, npasses, nrays, ndiams, ldiams )
            if nargin < 3 || isempty( lambda ), lambda = 557.7; end
            if nargin < 4 || isempty( npasses ), npasses = 4; end
            if nargin < 5 || isempty( nrays ), nrays = 50; end
            if nargin < 6 || isempty( ndiams ), ndiams = 10; end
            if nargin < 7 || isempty( ldiams ), ldiams = [ self.LensD * 0.9, self.LensD ]; end
            
            % get the necessary aperture parameters
            pupil_id = self.find_aperture_id( ); % find the index of the aperture element
            
            % create the test ray bundle
            test_eye = self.copy( );
            test_eye.elem{ test_eye.cnt } = Screen( [ self.elem{ self.cnt }.r( 1 ) 0 0 ], 1e6, 1e6, 500, 500 );
            
            % trace through the initial grid
            test_eye.SetLensDiameter( min( ldiams ) );
            rays_in = test_eye.fit_ray_grid_to_pupil( 'outside', 1, nrays, 'source', [ -fdist * 1e3 0 0 ], [ 1 0 0 ], ...
                'hexcircle', 'air', lambda * 1e-9 );
            rays_through = test_eye.trace( rays_in );
            
            % diameters to test
            for p = 1 : npasses
                ld = linspace( ldiams( 1 ), ldiams( 2 ), ndiams );
                
                dv = zeros( ndiams, 1 );
                for i = 1 : ndiams
                    % apply the lens diameter
                    test_eye.SetLensDiameter( ld( i ) );

                    % loop through the optical elements after the pupil
                    for e = pupil_id - 1 : test_eye.cnt 
                        rays_through( e + 1 ) = rays_through( e ).interaction( test_eye.elem{ e }, 1, 1, 1 );
                    end

                    % store the stats for later
                    [ ~, dv( i ) ] = rays_through( end ).stat_sph( [0, 0, 0] );
                end
                
                % find the lens diameter with the tightest focus on the retina
                [ coc, mi ] = min( dv );
                lens_diam = ld( mi );

                % update the diameters for the next pass
                ldiams( 1 ) = ld( max( mi - 1, 1 ) );
                ldiams( 2 ) = ld( min( mi + 1, ndiams ) );
            end
            
            % create the resulting eye
            result_eye = self.copy( );
            result_eye.MakeElements( );
            result_eye.SetLensDiameter( lens_diam );
            
            %{
            for e = pupil_id : test_eye.cnt 
                rays_through( e + 1 ) = rays_through( e ).interaction( result_eye.elem{ e }, 1, 1, 1 );
            end
            result_eye.draw( rays_through, 'lines', .33, 1.0, '3D', [ 0, 0 ] );
            axis( [ -10 self.EyeT + 1 -10 10 ] );
            axis equal vis3d off;
            ax = gca;
            ax.Clipping = 'on';
            %}
        end
        
        function [ dist, coc, dv, dd ] = FocusDistance( self, lambda, npasses, nrays, ndists, fdists )
            if nargin < 2 || isempty( lambda ), lambda = 557.7; end
            if nargin < 3 || isempty( npasses ), npasses = 4; end
            if nargin < 4 || isempty( nrays ), nrays = 50; end
            if nargin < 5 || isempty( ndists ), ndists = 10; end
            if nargin < 6 || isempty( fdists ), fdists = [ 0.01, 8.0 ]; end
            
            % create the test ray bundle
            test_eye = self.copy( );
            test_eye.elem{ test_eye.cnt } = Screen( [ self.elem{ self.cnt }.r( 1 ) 0 0 ], 20, 20, 20, 20 );
            
            % diameters to test
            for p = 1 : npasses
                dd = linspace( fdists( 1 ), fdists( 2 ), ndists );
                
                dv = zeros( ndists, 1 );
                for i = 1 : ndists
                    % construct the ray grid
                    rays_in = test_eye.fit_ray_grid_to_pupil( 'outside', 1, nrays, 'source', [ -dd( i ) * 1e3 0 0 ], [ 1 0 0 ], ...
                        'hexcircle', 'air', lambda * 1e-9 );
                    % trace it through the system
                    %rays_in = Rays( nrays, 'source', [ -dd( i ) * 1e3 0 0 ], [ 1 0 0 ], 2 * self.PupilD / ( dd( i ) * 1e3 ), 'hexcircle', 'air', lambda * 1e-9 );
                    rays_through = test_eye.trace( rays_in );

                    % store the stats for later
                    [ ~, dv( i ) ] = rays_through( end ).stat_sph( [0, 0, 0] );
                end
                
                % find the distance with the tightest focus on the retina
                [ coc, mi ] = min( dv );
                dist = dd( mi );

                % update the distances for the next pass
                fdists( 1 ) = dd( max( mi - 1, 1 ) );
                fdists( 2 ) = dd( min( mi + 1, ndists ) );
            end
        end

        function d = PupilRetinaDistance( self )
            d = self.EyeT - self.CorneaT - self.AqueousT;
        end
        
        function self = DrawDemo( self, fd, nrays, source, color, plot_type )
            if nargin < 2, fd = 0.02; end
            if nargin < 3, nrays = 50; end
            if nargin < 4, source = 'collimated'; end
            if nargin < 5, color = [ 0 1 0 ]; end
            if nargin < 6, plot_type = '3D'; end
            
            % construct the ray bundle
            if strcmp( source, 'collimated' )
                rays_in = Rays( nrays, 'collimated', [ -fd * 1e3 0 0 ], [ 1 0 0 ], self.PupilD, 'hexcircle', 'air', 557.7 * 1e-9, color );
            elseif strcmp( source, 'point' )
                rays_in = Rays( nrays, 'source', [ -fd * 1e3 0 0 ], [ 1 0 0 ], self.PupilD / ( fd * 1e3 ), 'hexcircle', 'air', 557.7 * 1e-9, color );
            end

            % trace the rays through the eye
            if ~isempty( source )
                rays_through = self.trace( rays_in );
            else
                rays_through = [];
            end
            
            % draw the rays
            switch plot_type
                case '3D'
                    self.draw( rays_through, 'lines', 0.33, 1.0, plot_type, [ 0, 0 ] );
                    axis( [ -10 self.EyeT + 1 -10 10 ] );
                    axis equal vis3d off;
                    ax = gca;
                    ax.Clipping = 'on';
                case 'XY'
                    self.draw( rays_through, 'lines', 1.0, 1.0, plot_type, [ 0, 90 ] );
                    axis( [ -10 self.EyeT + 1 -10 10 ] );
                    axis equal vis3d off;
                    ax = gca;
                    ax.Clipping = 'on';
                case 'XZ'
                    self.draw( rays_through, 'lines', 1.0, 1.0, plot_type, [ 0, 0 ] );
                    axis( [ -10 self.EyeT + 1 -10 10 ] );
                    axis equal vis3d off;
                    ax = gca;
                    ax.Clipping = 'on';
            end
        end
        
        function [ retinal_pos, retinal_dir, trace_source, trace_dir, dst ] = ComputeAberrationTraceVectors( self, ah, av, l, method, test_eye_method, nrays, tol )
            if nargin < 3, ah = 0; end
            if nargin < 4, av = 0; end
            if nargin < 5, l = 557.5; end
            
            % construct the test eye
            if strcmp( test_eye_method, 'healthy' )
                test_eye = EyeParametric( );
                test_eye.EyeT = self.EyeT;
                test_eye.MakeElements( );
            elseif strcmp( test_eye_method, 'input' )
                test_eye = self;
            end
            
            % simple method - simply trace a ray from the source, in the
            % given direction
            if strcmp( method, 'simple' )
                % determine the start position (outside)
                [ x, y, z ] = sph2cart( -ah, -av, -1e1  );
                trace_source = [ x y z ];
                trace_dir = -trace_source / norm( trace_source );
            
                % determine the retinal start position
                rays_in = Rays( 2, 'collimated', trace_source, trace_dir, 0.0, 'linear', 'air', l * 1e-9 );
                rays_through = test_eye.trace( rays_in, 1, 1, 1 );
                %test_eye.draw( rays_through, 'lines', 0.33, 1.0, '3D', [ 0, 0 ] );
                central_ray = ceil( rays_in.cnt / 2 );
                retinal_pos = rays_through( end ).r( central_ray, : );
                retinal_dir = rays_through( end - 1 ).r( central_ray, : ) - rays_through( end ).r( central_ray, : );
                retinal_dir = retinal_dir / norm( retinal_dir );
                
                % zero out the not implemented properties
                dst = 0.0;
                
            % chief ray method - find the chief ray
            elseif strcmp( method, 'chief' )
                % find the location of the chief ray
                [trace_source, axis_dst, trace_dir] = test_eye.find_chief_ray_position( ah, av, l, nrays, tol );

                % construct the ray grid
                rays_in = Rays( 2, 'collimated', trace_source, trace_dir, 0, 'linear', 'air', l * 1e-9 );
                rays_through = test_eye.trace( rays_in, 1, 1, 1 );
                %test_eye.draw( rays_through, 'lines', 0.33, 1.0, '3D', [ 0, 0 ] );

                % compute the results using the central ray's retinal location
                % and incoming direction
                central_ray = ceil( rays_in.cnt / 2 );
                retinal_pos = rays_through( end ).r( central_ray, : );
                retinal_dir = rays_through( end - 1 ).r( central_ray, : ) - rays_through( end ).r( central_ray, : );
                retinal_dir = retinal_dir / norm( retinal_dir );
                
                % write out the distance from the optical axis
                dst = axis_dst;
            end
        end

        function Opd = ComputeAberrationsParametric( self, pd, ah, av, l, varargin )
            parser = inputParser;
            parser.KeepUnmatched = true;
            addOptional( parser, 'TraceVectors', 'chief' );
            addOptional( parser, 'TraceVectorsEye', 'input' );
            addOptional( parser, 'TraceVectorsRays', 100 );
            addOptional( parser, 'TraceVectorsTol', 5e-4 );
            parse( parser, varargin{ : } );
            p = parser.Results;
    
            time_total = tic;
            
            % trace through a ray to find there to start the reverse ray-tracing
            time_chief_ray = tic;
            [ start_pos, start_dir, trace_source, trace_dir, axis_dst ] = self.ComputeAberrationTraceVectors( ...
                deg2rad( ah ), deg2rad( av ), l, p.TraceVectors, p.TraceVectorsEye, p.TraceVectorsRays, p.TraceVectorsTol );
            time_chief_ray = toc(time_chief_ray);
            
            % setup the capture screen
            test_eye = self.copy( );
            test_eye.PupilD = pd;
            test_eye.MakeElements( );
            test_eye.remove( test_eye.cnt );
            test_eye.reverse( );
            
            % compute the optical path differences
            time_aberrations = tic;
            aberrations = compute_aberrations( self, test_eye, ...
                'Material', 'vitreous', ...
                'TraceSource', start_pos, ...
                'TraceDirection', start_dir, ...
                'OutDirection', -trace_dir, ...
                'OutCenter', trace_source, ...
                'Lambda', l, ...
                varargin{:} );
            time_aberrations = toc(time_aberrations);
            
            time_total = toc(time_total);
            
            % Store the aberrations in the local 'Aberrations' field
            self.Aberrations = aberrations;
            self.Aberrations.Lambda = l;
            self.Aberrations.AngleHor = ah;
            self.Aberrations.AngleVert = av;
            self.Aberrations.ChiefAxisDistance = axis_dst;
            self.Aberrations.Timings.TimeChiefRay = time_chief_ray;
            self.Aberrations.Timings.TimeAberrations = time_aberrations;
            self.Aberrations.Timings.TimeTotal = time_total;
            
            % Also store the aberration info in the object itself
            self.Alpha = self.Aberrations.Opd;
            self.Lambda = self.Aberrations.Lambda;
            self.AngleHor = self.Aberrations.AngleHor;
            self.AngleVert = self.Aberrations.AngleVert;
            
            % return the OPD
            Opd = self.Alpha;
        end
        
        function Opd = ComputeAberrations( self, varargin )
            Opd = self.ComputeAberrationsParametric( self.PupilD, self.AngleHor, self.AngleVert, self.Lambda, varargin{:} );
        end
        
        function Opd = GetAberrations( self )
            Opd = self.Alpha;
        end

        function self = ValidateAberrations( self, aberrations )
            if size( self.Alpha ) == size( aberrations' )
                aberrations = aberrations';
            end
            if ~isequal( self.Alpha, aberrations )
                error( 'The input aberrations do not match the true aberrations; input: ' + mat2str(aberrations) + ', true: ' + mat2str(self.Alpha) )
            end
        end

        function self = PrintAberrations( self, aberrations, descriptions )
            if nargin < 2, aberrations = []; end
            if nargin < 2, descriptions = {}; end
            
            fprintf( '**************************************************\n' );
            disp( self.Aberrations.Timings );
            fprintf( '**************************************************\n' );
            fprintf( '[Input]: axis( %f, %f ) deg\n', ...
                self.AngleHor, self.AngleVert );
            fprintf( '[Input]: lambda( %f ) mum\n', self.Lambda * 1e-3 );
            fprintf( '**************************************************\n' );
            fprintf( '[Source]: chief-axis distance( %f )\n', ...
                self.Aberrations.ChiefAxisDistance );
            fprintf( '[Source]: position( %f, %f, %f )\n', ...
                self.Aberrations.TraceSource( 1 ), self.Aberrations.TraceSource( 2 ), self.Aberrations.TraceSource( 3 ) );
            fprintf( '[Source]: direction( %f, %f, %f )\n', ...
                self.Aberrations.TraceDirection( 1 ), self.Aberrations.TraceDirection( 2 ), self.Aberrations.TraceDirection( 3 ) );
            fprintf( '[Source]: angle( %f, %f ) deg\n', ...
                rad2deg( self.Aberrations.TraceAngle( 1 ) ), rad2deg( self.Aberrations.TraceAngle( 2 ) ) );
            fprintf( '**************************************************\n' );
            fprintf( '[Output]: location( %f, %f, %f )\n', ...
                self.Aberrations.OutCenter( 1 ), self.Aberrations.OutCenter( 2 ), self.Aberrations.OutCenter( 3 ) );
            fprintf( '[Output]: direction( %f, %f, %f )\n', ...
                self.Aberrations.OutDirection( 1 ), self.Aberrations.OutDirection( 2 ), self.Aberrations.OutDirection( 3 ) );
            fprintf( '[Output]: angle( %f, %f ) deg\n', ...
                rad2deg( self.Aberrations.OutAngle( 1 ) ), rad2deg( self.Aberrations.OutAngle( 2 ) ) );
            fprintf( '**************************************************\n' );
            fprintf( '[Rays]: pupil coverage( %f )\n', ...
                self.Aberrations.RaysRadiusRatio );
            fprintf( '[Rays]: reqested( %d ), input( %d ), valid( %d )\n', ...
                self.Aberrations.NumInputRays, self.Aberrations.NumOriginalRays, self.Aberrations.NumValidRays );
            fprintf( '[Rays]: spread( %f, %f, %f, %f )\n', ...
                self.Aberrations.RaysSpread( 1 ), self.Aberrations.RaysSpread( 2 ), ...
                self.Aberrations.RaysSpread( 3 ), self.Aberrations.RaysSpread( 4 ) );
            fprintf( '**************************************************\n' );
            fprintf( '[Pupil]: center( %f, %f ) mm\n', ...
                self.Aberrations.PupilCenter( 1 ), self.Aberrations.PupilCenter( 2 ) );
            fprintf( '[Pupil]: radius( %f, %f ) mm\n', ...
                self.Aberrations.PupilRadius( 1 ), self.Aberrations.PupilRadius( 2 ) );
            fprintf( '[Pupil]: rounded radius( %f ) mm\n', ...
                self.Aberrations.PupilRounded );
            fprintf( '[Pupil]: axis( %f deg )\n', rad2deg( self.Aberrations.PupilAngle ) );
            fprintf( '**************************************************\n' );
            fprintf( '[Defocus]: dioptres( %f ) D\n', self.Aberrations.Defocus  );
            fprintf( '[RMS]: %f mum\n', self.Aberrations.Rms );
            fprintf( '**************************************************\n' );
            aberrations_combined = [ self.Alpha, aberrations ];
            descriptions_combined = [ 'Path Diff.', descriptions ];
            print_aberrations( aberrations_combined, descriptions_combined, 18 );
            fprintf( '**************************************************\n' );
        end
    end
end