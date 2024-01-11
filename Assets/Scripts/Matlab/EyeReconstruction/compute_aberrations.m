function Aberration = compute_aberrations( original_bench, bench, varargin )
    % input parser
    parser = inputParser;
    parser.KeepUnmatched = true;
    addOptional( parser, 'Material', 'air' );
    addOptional( parser, 'Lambda', 557.7 ); 
    addOptional( parser, 'TraceSource', [ 0 0 0 ] ); 
    addOptional( parser, 'TraceDirection', [ -1 0 0 ] ); 
    addOptional( parser, 'OutDirection', [ -1 0 0 ] );
    addOptional( parser, 'OutCenter', [ 0 0 0 ] );
    addOptional( parser, 'NumRays', 500 ); 
    addOptional( parser, 'MaxDegree', 6 ); 
    addOptional( parser, 'GridShape', 'hexcircle' );
    addOptional( parser, 'GridSpread', 'approximate' );
    addOptional( parser, 'GridFitPasses', 3 );
    addOptional( parser, 'RadiusThreshold', 1.0 );
    addOptional( parser, 'ProjectionMethod', 'parallel' );
    addOptional( parser, 'CircumscribeRays', 'expected' );
    addOptional( parser, 'CircumscribeShape', 'circle' );
    addOptional( parser, 'CircumscribeExtension', 'mirror' );
    addOptional( parser, 'EllipsePrecision', 1e-4 );
    addOptional( parser, 'Centering', 'ellipse' );
    addOptional( parser, 'Stretching', 'disable' );
    addOptional( parser, 'PupilRounding', 0.05 );
    addOptional( parser, 'CaptureDistance', 1e-1 );
    addOptional( parser, 'CaptureSize', 1e6 );
    addOptional( parser, 'CaptureBins', 10 );
    addOptional( parser, 'FitMethod', 'lsq' );
    addOptional( parser, 'IgnoreMissed', true, @islogical );
    addOptional( parser, 'IgnoreBlocked', true, @islogical );
    addOptional( parser, 'IgnoreTIR', true, @islogical );
    addOptional( parser, 'PlotPupilCentroids', false, @islogical );
    addOptional( parser, 'PlotProjection2D', false, @islogical );
    addOptional( parser, 'PlotProjection3D', false, @islogical );
    addOptional( parser, 'PlotSlopes', false, @islogical );
    addOptional( parser, 'PlotCentroids', false, @islogical );
    addOptional( parser, 'PlotAberration2D', false, @islogical );
    addOptional( parser, 'PlotAberration3D', false, @islogical );
    addOptional( parser, 'PlotPSF', false, @islogical );
    addOptional( parser, 'PlotImage', false, @islogical );
    parse( parser, varargin{ : } );
    p = parser.Results;
    
    % setup the test bench
    tic;
    test_bench = setup_bench( bench, p.OutDirection, p.CaptureDistance, p.CaptureSize, p.CaptureBins );
    time_setup_bench = toc;
    
    pupil_id = test_bench.find_aperture_id( ); % find the index of the aperture element
    pupil_position = test_bench.elem{ pupil_id }.r; % extract its position
    pupil_diameter = test_bench.elem{ pupil_id }.D( 1 ); % extract the diameter of the opening hole
    
    % setup the ray grid
    tic;
    [ rays_in, spread, radius_ratio ] = setup_rays( p.NumRays, p.GridShape, p.TraceSource, p.TraceDirection, ...
        p.Material, p.Lambda, p.GridSpread, p.GridFitPasses, test_bench, p.PlotPupilCentroids );
    time_setup_rays = toc;
    
    % trace the rays through the eye
    tic;
    rays_through = test_bench.trace( rays_in, b2f(p.IgnoreMissed), b2f(p.IgnoreBlocked), b2f(p.IgnoreTIR) );
    time_trace_rays = toc;
    
    % find the valid ray ids
    valid_rays = valid_ray_ids( test_bench, rays_through, p.RadiusThreshold );
    
    % extract the corneal and actual screen coordinates
    tic;
    screen_pos = test_bench.elem{ test_bench.cnt }.r;
    screen_normal = test_bench.elem{ test_bench.cnt }.normal( );
    cornea_coords = rays_through( test_bench.cnt ).r( valid_rays, : ); % cornea coordinates
    screen_pos_act = rays_through( test_bench.cnt + 1 ).r( valid_rays, : ); % actual screen positions
    screen_pos_exp = intersect_screen( screen_pos, screen_normal, cornea_coords , p.OutDirection ); % expected screen positions
    length_exp = vecnorm( cornea_coords - screen_pos_exp, 2, 2 );
    length_act = vecnorm( cornea_coords - screen_pos_act, 2, 2 );
    time_extract_rays = toc;
    
    % project the rays to 2D
    tic;
    chief = intersect_screen( screen_pos, screen_normal, p.OutCenter, p.OutDirection );
    chief_2d = project_to_screen( p.ProjectionMethod, screen_pos, screen_normal, chief );
    screen_pos_exp_2d = project_to_screen( p.ProjectionMethod, screen_pos, screen_normal, screen_pos_exp );
    screen_pos_act_2d = project_to_screen( p.ProjectionMethod, screen_pos, screen_normal, screen_pos_act );
    
    if strcmp( p.Centering, 'chief' )
        screen_pos_exp_2d = screen_pos_exp_2d - chief_2d;
        screen_pos_act_2d = screen_pos_act_2d - chief_2d;
    end
    time_project_rays = toc;
    
    % fit an ellipse over the expected ray positions
    tic;
    [ ellipse_x0, ellipse_r, ellipse_ang, ch_points ] = fit_ellipse( p.CircumscribeRays, screen_pos_exp_2d, screen_pos_act_2d, p.CircumscribeExtension, p.CircumscribeShape, p.EllipsePrecision ); % ellipse fit
    actual_pupil_radius = p.PupilRounding * ceil( max( ellipse_r ) / p.PupilRounding ); % store the pupil radius
    if strcmp( p.Centering, 'ellipse' )
        screen_pos_exp_2d = screen_pos_exp_2d - ellipse_x0';
        screen_pos_act_2d = screen_pos_act_2d - ellipse_x0';
        ch_points = ch_points - ellipse_x0';
    end
    time_circumscribe_rays = toc;
        
    % unwrap the ellipse and transform it to a circle
    tic;
    screen_pos_exp_uw = unwrap_ellipse( screen_pos_exp_2d, ellipse_x0, ellipse_r, ellipse_ang, p.Stretching );
    screen_pos_act_uw = unwrap_ellipse( screen_pos_act_2d, ellipse_x0, ellipse_r, ellipse_ang, p.Stretching );
    time_unwrap_rays = toc;
    
    % normalized pupil coordinates and ray slopes
    tic;
    positions_norm = screen_pos_exp_uw / actual_pupil_radius; % normalized coords
    slopes = compute_ray_slopes( actual_pupil_radius, length_exp, screen_pos_exp_uw, screen_pos_act_uw ); % slopes
    time_compute_slopes = toc;
    
    % generate the results structure
    Aberration = struct;
    Aberration.TraceSource = p.TraceSource; % write out the trace vectors and angles
    Aberration.TraceDirection = p.TraceDirection;
    Aberration.TraceAngle = trace_angles( p.TraceDirection );
    Aberration.OutDirection = p.OutDirection;
    Aberration.OutCenter = p.OutCenter;
    Aberration.OutAngle = trace_angles( p.OutDirection );
    Aberration.Lambda = p.Lambda; % store the wavelength of light
    Aberration.MaxDegree = p.MaxDegree; % store the ray-related parameters
    Aberration.GridShape = p.GridShape;
    Aberration.RaysSpread = spread;
    Aberration.RaysRadiusRatio = radius_ratio;
    Aberration.NumInputRays = p.NumRays;
    Aberration.NumOriginalRays = size( rays_through( test_bench.cnt ).r, 1 );
    Aberration.NumValidRays = size( screen_pos_exp, 1 );
    Aberration.PupilCenter = ellipse_x0; % store the pupil parameters
    Aberration.PupilRadius = ellipse_r;
    Aberration.PupilAngle = ellipse_ang;
    Aberration.PupilRounded = actual_pupil_radius;
    tic;
    Aberration.Opd = fit_data( positions_norm, slopes, p.MaxDegree, p.FitMethod ); % aberration coefficients
    %{
    if p.UnstretchEllipse
        Aberration.Opd = lc_to_se( Aberration.Opd, actual_pupil_radius, ellipse_x0, ellipse_r, ellipse_ang );
    end
    %}
    time_fit_data = toc;
    Aberration.Rms = sum( Aberration.Opd( 2 : end ) );
    Aberration.Defocus = defocus_dioptres( test_bench, Aberration.Opd );
    
    Aberration.Timings = struct;
    Aberration.Timings.SetupBench = time_setup_bench;
    Aberration.Timings.SetupRays = time_setup_rays;
    Aberration.Timings.TraceRays = time_trace_rays;
    Aberration.Timings.ExtractRays = time_extract_rays;
    Aberration.Timings.ProjectRays = time_project_rays;
    Aberration.Timings.CircumscribeRays = time_circumscribe_rays;
    Aberration.Timings.UnwrapRays = time_unwrap_rays;
    Aberration.Timings.ComputeSlopes = time_compute_slopes;
    Aberration.Timings.FitData = time_fit_data;
    
    % various plots
    if p.PlotProjection2D
        Aberration.FigProjection2D = plot_projection2D( original_bench, test_bench, rays_through, cornea_coords, screen_pos_exp );
    end
    if p.PlotProjection3D
        Aberration.FigProjection = plot_projection3D( original_bench, test_bench, rays_through, cornea_coords, screen_pos_exp );
    end
    if p.PlotCentroids
        ch_points_uw = unwrap_ellipse( ch_points, ellipse_x0, ellipse_r, ellipse_ang, p.Stretching );
        Aberration.FigCentroids = plot_centroids( screen_pos_exp_uw, screen_pos_act_uw, ch_points_uw, actual_pupil_radius, ellipse_r, ellipse_ang );
    end
    if p.PlotSlopes
        Aberration.FigSlopes = plot_slopes( positions_norm, slopes );
    end
    if p.PlotImage
        Aberration.FigImage = plot_image( test_bench );
    end
    
    % plot the aberration surfaces
    if p.PlotAberration2D
        Aberration.FigAberration2D = plot_aberration( Aberration.Opd, 'Optical Path Difference', '2D' );
    end
    if p.PlotAberration3D
        Aberration.FigAberration3D = plot_aberration( Aberration.Opd, 'Optical Path Difference', '3D' );
    end
end

% --------------------------------------------------------
% converts a boolean to a flag
% --------------------------------------------------------

function f = b2f( b )
    if b, f = 1;
    else, f = 0;
    end
end

% --------------------------------------------------------
% test bench construction functions
% --------------------------------------------------------

function test_bench = setup_bench( bench, out_direction, dist, csize, nbins )
    % make a copy of the input bench
    test_bench = bench.copy( );
    
    % create the capture screen
    test_bench.cnt = test_bench.cnt + 1;
    test_bench.elem{ test_bench.cnt } = Screen( out_direction * dist * 1e3, ...
        csize, csize, nbins, nbins );
    test_bench.elem{ test_bench.cnt }.rotate( [ 0 0 1 ], deg2rad( 180 ) );
    
    % rotate the screen, if needed
    if norm( cross( out_direction, [ -1 0 0 ] ) ) > 1e-3
        screen_normal = test_bench.elem{ test_bench.cnt }.normal( );
        screen_normal = screen_normal / norm( screen_normal );
        rot_axis = cross( out_direction, screen_normal );
        rot_axis = rot_axis / norm( rot_axis );
        rot_angle = -acos( dot( out_direction, screen_normal ) );
        test_bench.elem{ test_bench.cnt }.rotate( rot_axis, rot_angle );
        assert( norm( cross( test_bench.elem{ test_bench.cnt }.normal( ), -out_direction ) ) < 1e-3 );
    end
end

% --------------------------------------------------------
% construct the test ray grid
% --------------------------------------------------------

function [ rays, spread, radius_ratio ] = setup_rays( nrays, shape, src, dir, material, lambda, spread, fit_passes, test_bench, plot_rays )
    % get the pupil properties
    pupil_id = test_bench.find_aperture_id( ); % find the index of the aperture element
    pupil_position = test_bench.elem{ pupil_id }.r; % extract its position
    pupil_diameter = test_bench.elem{ pupil_id }.D( 1 ); % extract the diameter of the opening hole
    pupil_distance = src( 1 ) - pupil_position( 1 ); % get its distance from the source location
        
    % ray-traced, accurate spread
    if isa( spread, 'char' ) && strcmp( spread, 'trace' )
        tmp_bench = test_bench.copy( );
        tmp_bench.cnt = 0;
        for i = 1:test_bench.cnt
            if ~isa( test_bench.elem{ i }, 'ZernikeLens' )
                tmp_bench.cnt = tmp_bench.cnt + 1;
                tmp_bench.elem{ tmp_bench.cnt } = tmp_bench.elem{ i };
            end
        end
        
        % fit a ray grid to the physical pupil
        [rays, spread, radius_ratio] = tmp_bench.fit_ray_grid_to_pupil( 'inside', fit_passes, nrays, 'source', src, dir, shape, material, lambda * 1e-9 );
        
    % approximate spread
    elseif isa( spread, 'char' ) && strcmp( spread, 'approximate' )
        % calculate the approximate spread
        spread = pupil_diameter / pupil_distance * ones( 4, 1 );
    
        % construct the outgoing ray grid
        rays = Rays( nrays, 'source', src, dir, spread, shape, material, lambda * 1e-9 );
        
        % zero out the radius ratio
        radius_ratio = 0.0;
    end
        
    % plot the final ray grid
    if plot_rays
        % construct a bench without all the elements following the pupil
        spread_bench = test_bench.create_pupil_capture_bench( 'inside' ); 

        % plot the spread gri
        plot_pupil_grid( spread_bench, rays, pupil_diameter );
    end
end

% --------------------------------------------------------
% truncates invalid rays
% --------------------------------------------------------

function valid_rays = valid_ray_ids( test_bench, rays_through, dist_threshold )
    valid_rays = ones( size( rays_through( test_bench.cnt + 1 ).I ) );
    
    % validate rays by intensity
    valid_rays = valid_rays & rays_through( test_bench.cnt + 1 ).I > 0;
    
    % remove infs and nans
    valid_rays = valid_rays & ~any( isinf( rays_through( test_bench.cnt + 1 ).r ), 2 );
    valid_rays = valid_rays & ~any( isnan( rays_through( test_bench.cnt + 1 ).r ), 2 );
    
    % validate by distance from the center
    for elem_id = 1:test_bench.cnt
        switch class( test_bench.elem{ elem_id } )
            case { 'Lens', 'ZernikeLens' } % intersection with a plane
                R = test_bench.elem{ elem_id }.D( 2 ) / 2;
                r = sqrt( sum( rays_through( elem_id + 1 ).r( :, 2:3 ).^2, 2 ) ); 
                valid_rays = valid_rays & r <= R * dist_threshold;
        end
    end
end

% --------------------------------------------------------
% computes the expected ray positions by intersecting the
% outgoing direction with the screen
% --------------------------------------------------------

function screen_positions = intersect_screen( screen_pos, screen_normal, ray_coords, ray_direction )
    % normalize the outgoing direction vector
    ray_direction = ray_direction / norm( ray_direction );
    
    % number of rays we have
    num_rays = size( ray_coords, 1 );
    
    % plane P_0 and normal
    p0 = repmat( screen_pos, num_rays, 1 );
    n = repmat( screen_normal, num_rays, 1 );
    
    % ray R0 and direction
    r0 = ray_coords;
    r = repmat( ray_direction, num_rays, 1 );
    
    % intersection ray parameters
    t = dot( p0 - r0, n, 2 ) ./ dot( r, n, 2 );
    
    % intersection points
    screen_positions = r0 + t .* r;
end

% --------------------------------------------------------
% projects the rays to a 2D screen
% --------------------------------------------------------

function projected_positions = project_to_screen( proj_method, screen_pos, screen_normal, positions )
    if strcmp( proj_method, 'parallel' )
        projected_positions = project_to_plane( screen_pos, screen_normal, positions, [0,0,1] );
    elseif strcmp( proj_method, 'orthogonal' )
        projected_positions = positions( :, 2:3 );
    end
end

% --------------------------------------------------------
% fits a min. volume ellipse around the input data
% --------------------------------------------------------

function [ x0, r, ang, ch_points ] = fit_ellipse( rays, positions_exp, positions_act, extension, shape, precision )
    % determine which rays to bound
    switch rays
        case 'expected', positions = positions_exp;
        case 'actual', positions = positions_act;
    end
    
    switch extension
        case 'disable'
            positions_ext = positions;
            
        % extend the ray locations with a mirrored set of points to
        % account for the unoccupied space of unknown rays
        case 'mirror'
            positions_ext = [ positions; positions * -1 ];
    end
    
    % compute the convex hull of the locations
    ch_points = positions_ext( convhull( positions_ext, 'Simplify', true ), : );
    
    % fit the shape around the CH points
    switch shape
        % circular fit
        case 'circle'
            circle = TaubinSVD( ch_points );
            circle = LM( ch_points, circle );
            x0 = circle( 1:2 )';
            r = repmat( circle( 3 ), 2, 1 );
            ang = 0;
            
        % elliptical fit
        case 'ellipse'
            ell = MinVolEllipseFit( ch_points, precision );
            x0 = ell( 1:2 )';
            r = ell( 3:4 )';
            ang = ell( 5 );
    end
end

% --------------------------------------------------------
% transforms the parameter elliptical coords to circular
% fitting approaches described in 'Population distribution of wavefront aberrations in the peripheral human eye'
% --------------------------------------------------------

function M = rot_mat_2d( th )
    M = [ cos( th ), -sin( th ); sin( th ), cos( th ) ];
end

function M = scale_mat_2d( s )
    M = [ s( 1 ), 0; 0, s( 2 ) ];
end

function circular_coords = unwrap_ellipse( elliptical_coords, ellipse_x0, ellipse_r, ellipse_angle, stretching )
    circular_coords = elliptical_coords;
    if strcmp( stretching, 'ellipse2circle' )
        circular_coords = circular_coords - ellipse_x0';
        R = rot_mat_2d( ellipse_angle );
        circular_coords = circular_coords * R;
        S = scale_mat_2d( [ ellipse_r( 2 ) / ellipse_r( 1 ), 1 ] );
        circular_coords = circular_coords * S;
        R = rot_mat_2d( -ellipse_angle );
        circular_coords = circular_coords * R;
        circular_coords = circular_coords + ellipse_x0';
    end
end

% --------------------------------------------------------
% computes the wavefront slopes
% from 'Introduction to Wavefront Sensors', Eq 2.8 (p. 18)
% --------------------------------------------------------
% T = -(R / r) * (dW / dp)     [transverse ray aberration, normalized pupil coords.]
% dW / dp = T * -(r / R)       [local wavefront tilt]
%
% with: 
%   r = exit pupil radius
%   R = distance from last element to the screen
% --------------------------------------------------------

function ray_slopes = compute_ray_slopes( pupil_radius, expected_length, expected_positions_2d, actual_positions_2d )
    transverse_aberration = actual_positions_2d - expected_positions_2d;
    ray_slopes = transverse_aberration .* ( -pupil_radius ./ expected_length ) * 1e3;
end

% --------------------------------------------------------
% performs the surface fitting
% --------------------------------------------------------

function coefficients = fit_data( positions_norm, slopes, max_degree, fit_method )
    % evaluate the zernike basis functions
    K = ZernikeNumCoeffs( max_degree );
    basis = zeros( size( positions_norm, 1 ), 2, K );
    for c = 1 : K
        basis( :, :, c ) = ZernikeRMS( c ) .* ZernikePartial( c, positions_norm( :, 1 ), positions_norm( :, 2 ) );
    end
    
    % construct the slope and Zernike-basis matrices
    V = [ squeeze( basis( :, 1, : ) ) ; squeeze( basis( :, 2, : ) ) ]; 
    S = [ slopes( :, 1 ) ; slopes( :, 2 ) ];
    
    % construct extended matrices (adds bias term)
    Vb = [ V; ones( 1, size( V, 2 ) ) ]; % extend V with a row of ones
    Sb = [ S; 0 ]; % extend S with a zero
    
    switch fit_method
        case 'lsq'
            VtV = Vb.' * Vb;
            if det( VtV ) < 1e-1
                error( "V' * V is singular!" );
            end
            coefficients = VtV^-1 * Vb.' * Sb;
        case 'lasso'
            fit_lasso = lasso( V, S, 'Alpha', 1 );
            coefficients = fit_lasso( :, 1 );
        case 'ridge'            
            k = 0:1e-5:5e-3;
            fit_ridge = ridge( S, V, k, 0 );
            coefficients = fit_ridge( 2:end, 2 );
    end
end

% --------------------------------------------------------
% converts from large circular (LC) to small elliptical (SE)
% --------------------------------------------------------

function coefficients = lc_to_se( coefficients, actual_pupil_radius, ellipse_x0, ellipse_r, ellipse_ang )
    major = max( max( ellipse_r ) );
    minor = min( min( ellipse_r ) );
    %etaE=minor/major;
    etaE = major/minor;
    thetaE = deg2rad( ellipse_ang );
    
    coefficients = ZernikeTransform( coefficients, etaE, thetaE );
end

function C2 = ZernikeTransform( C1, etaE, thetaE )
    jnm=length(C1)-1; nmax=ceil((-3+sqrt(9+8*jnm))/2); jmax=nmax*(nmax+3)/2;
    S=zeros(jmax+1,1); S(1:length(C1))=C1; C1=S; clear S
    P=zeros(jmax+1); % Matrix P transforms from standard to Campbell order
    N=zeros(jmax+1); % Matrix N contains the normalization coefficients
    R=zeros(jmax+1); % Matrix R is the coefficients of the radial polynomials
    CC1=zeros(jmax+1,1); % CC1 is a complex representation of C1
    counter=1;
    for m=-nmax:nmax % Meridional indexes
    for n=abs(m):2:nmax % Radial indexes
        jnm=(m+n*(n+2))/2;
        P(counter,jnm+1)=1;
        N(counter,counter)=sqrt(n+1);
        for s=0:(n-abs(m))/2
            R(counter-s,counter)=(-1)^s*factorial(n-s)/(factorial(s)*factorial((n+m)/2-s)*factorial((n-m)/2-s));
        end
        if m<0 CC1(jnm+1)=(C1((-m+n*(n+2))/2+1)+i*C1(jnm+1))/sqrt(2);
        elseif m==0 CC1(jnm+1)=C1(jnm+1);
        else CC1(jnm+1)=(C1(jnm+1)-i*C1((-m+n*(n+2))/2+1))/sqrt(2);
        end
        counter=counter+1;
    end,end
    ETAE=[]; % Coordinate-transfer matrces
    for m=-nmax:nmax
    for n=abs(m):2:nmax
        ETAE=[ETAE P*(transformElliptical(n,m,jmax,etaE,thetaE))];
    end,end
    ETA=ETAE;
    C=inv(P)*inv(N)*inv(R)*ETA*R*N*P;
    CC2=C*CC1;
    C2=zeros(jmax+1,1); % C2 is formed from the complex Zernike coefficients, CC2
    for m=-nmax:nmax
    for n=abs(m):2:nmax
        jnm=(m+n*(n+2))/2;
        if m<0, C2(jnm+1)=imag(CC2(jnm+1)-CC2((-m+n*(n+2))/2+1))/sqrt(2);
        elseif m==0, C2(jnm+1)=real(CC2(jnm+1));
        else C2(jnm+1)=real(CC2(jnm+1)+CC2((-m+n*(n+2))/2+1))/sqrt(2);
        end
    end,end
end

function Eta = transformElliptical(n,m,jmax,etaE,thetaE)
    % Returns coefficients for transforming a ro^n*exp(i*m*theta)-term into '-terms
    Eta=zeros(jmax+1,1);
    for p=0:((n+m)/2)
    for q=0:((n-m)/2)
        nnew=n; mnew=m-2*p+2*q;
        jnm=(mnew+nnew*(nnew+2))/2;
        Eta(floor(jnm+1))=Eta(floor(jnm+1))+...
            1/(2^n)*...
            nchoosek((n+m)/2,p)*...
            nchoosek((n-m)/2,q)*...
            (etaE+1)^(n-p-q)*...
            (etaE-1)^(p+q)*...
            exp(i*(2*(p-q)*thetaE));
    end
    end
end

% --------------------------------------------------------
% function to calculate output angles
% --------------------------------------------------------

function angles = trace_angles( trace_direction )
    [ az, el, ~ ] = cart2sph( -trace_direction( 1 ), trace_direction( 2 ), trace_direction( 3 ) );
    angles = [ az, el ];
end

% --------------------------------------------------------
% function to calculate defocus dioptres
% --------------------------------------------------------

function defocus = defocus_dioptres( test_bench, aberrations )
    % get the pupil properties
    pupil_id = test_bench.find_aperture_id( ); % find the index of the aperture element
    pupil_diameter = test_bench.elem{ pupil_id }.D( 1 ); % extract the diameter of the opening hole
    pupil_radius = pupil_diameter / 2;
    
    % compute the defocus in dioptres
    defocus = -( aberrations( ZernikeIdAnsi( 2, 0 ) ) * 4 * sqrt(3) ) / ( pupil_radius.^2 );
end

% --------------------------------------------------------
% various plot functions
% --------------------------------------------------------

function fig = plot_pupil_grid( test_bench, rays, pupil_diameter )
    rays_through = test_bench.trace( rays, 1, 1 );
    rays_screen = rays_through( test_bench.cnt + 1 );
    pupil_pos = rays_screen.r( rays_screen.I > 0, : );
    pupil_pos_2d = pupil_pos( :, 2:3 );
    ch_points = pupil_pos_2d( convhull( pupil_pos_2d ), : );
    fig = plot_centroids( pupil_pos_2d, pupil_pos_2d, ch_points, pupil_diameter / 2 );
    %test_bench.draw( rays_through, 'lines', 0.33, 1.0, '3D', [ 0, 0 ] );
end

function fig = plot_projection2D( original_bench, test_bench, rays_through, cornea_coords, screen_pos_exp )
    % draw the traced rays
    draw_bench = test_bench.copy( );
    draw_bench.elem{ draw_bench.cnt }.h = 2e1;
    draw_bench.elem{ draw_bench.cnt }.w = 2e1;
    draw_bench.cnt = draw_bench.cnt + 1;
    draw_bench.elem{ draw_bench.cnt } = original_bench.elem{ original_bench.cnt };
    for i = 1:size( rays_through, 2 )
        rays_through( i ).color = repmat( [ 0, 0.7, 0 ], [ rays_through( i ).cnt, 1 ] );
    end
    fig = draw_bench.draw( rays_through, 'lines', 0.33, 1.0, 'XZ', [ 0, 0 ] );

    % draw out the expected rays
    axvis = abs( rays_through( 1 ).n( :, 2 ) ) < 1e-3;
    x0 = cornea_coords( axvis, 1 )';
    x1 = screen_pos_exp( axvis, 1 )';
    y0 = cornea_coords( axvis, 3 )';
    y1 = screen_pos_exp( axvis, 3 )';
    z0 = cornea_coords( axvis, 2 )';
    z1 = screen_pos_exp( axvis, 2 )';
    plot3( [ x0; x1 ], [ y0; y1 ], [ z0; z1 ], '-', 'Color', 'b', 'LineWidth', 1.0 );
end

function fig = plot_projection3D( original_bench, test_bench, rays_through, cornea_coords, screen_pos_exp )
    % draw the traced rays
    draw_bench = test_bench.copy( );
    draw_bench.elem{ draw_bench.cnt }.h = 3e1;
    draw_bench.elem{ draw_bench.cnt }.w = 3e1;
    fig = draw_bench.draw( rays_through, 'lines', 0.33, 1.0, '3D', [ 0, 0 ] );

    % draw out the expected rays
    x0 = cornea_coords( :, 1 )';
    x1 = screen_pos_exp( :, 1 )';
    y0 = cornea_coords( :, 2 )';
    y1 = screen_pos_exp( :, 2 )';
    z0 = cornea_coords( :, 3 )';
    z1 = screen_pos_exp( :, 3 )';
    plot3( [ x0; x1 ], [ y0; y1 ], [ z0; z1 ], '-', 'Color', 'b' );
end

function fig = plot_centroids( screen_pos_exp, screen_pos_act, ch_points, pupil_radius, ellipse_r, ellipse_ang )
    pupil_colors = [ 0.07, 0.54, 0.77 ];
    ch_colors = [ 0.07, 0.54, 0.77 ];
    fig = figure( 'Name', 'Wave centroids', 'NumberTitle', 'Off' );
    % expected locations
    plot( screen_pos_exp( :, 1 ), screen_pos_exp( :, 2 ), 'LineStyle', 'none', 'Marker', 'o', 'MarkerFaceColor', 'g', 'MarkerEdgeColor', 'k', 'MarkerSize', 3 );
    hold on;
    % actual locations
    plot( screen_pos_act( :, 1 ), screen_pos_act( :, 2 ), 'LineStyle', 'none', 'Marker', 'o', 'MarkerFaceColor', 'r', 'MarkerEdgeColor', 'k', 'MarkerSize', 3 );
    hold on;
    % pupil circle
    t = linspace( 0, 2 * pi, 100 );
    x = pupil_radius * cos( t );
    y = pupil_radius * sin( t );
    plot( x, y, 'Color', pupil_colors, 'LineStyle', '-.' );
    % pupil ellipse
    if nargin >= 5
        t = linspace( 0, 2 * pi, 100 );
        x = ellipse_r( 1 ) * cos( t ) * cos( ellipse_ang ) - ellipse_r( 2 ) * sin( t ) * sin( ellipse_ang );
        y = ellipse_r( 2 ) * sin( t ) * cos( ellipse_ang ) + ellipse_r( 1 ) * cos( t ) * sin( ellipse_ang );
        %plot( x, y, 'Color', pupil_colors );
    end
    % convex hull points
    hold on;
    plot( ch_points( :, 1 ), ch_points( :, 2 ), 'Color', ch_colors );
    % axis settings
    ax = gca;
    % - colors
    fig.Color = 'w';
    ax.Color = 'w';
    ax.XColor = 'k';
    ax.YColor = 'k';
    ax.ZColor = 'k';
    disableDefaultInteractivity(ax)
    pbaspect( [ 1 1 1 ] );
    % - extents
    cmin = repmat( min( min( [ screen_pos_act; screen_pos_exp ] ) ), 2, 1 );
    cmax = repmat( max( max( [ screen_pos_act; screen_pos_exp ] ) ), 2, 1 );
    center = ( cmax + cmin ) * 0.5;
    extent = max( ( cmax - cmin ) * 0.5,  pupil_radius );
    margin = 0.1;
    xlim( [ center( 1 ) - extent( 1 ) * ( 1 + margin ), center( 1 ) + extent( 1 ) * ( 1 + margin ) ] );
    ylim( [ center( 2 ) - extent( 2 ) * ( 1 + margin ), center( 2 ) + extent( 2 ) * ( 1 + margin ) ] );
end

function fig = plot_slopes( normalized_positions, ray_slopes )
    fig = figure( 'Name', 'Ray slopes on the pupil', 'NumberTitle', 'Off' );
    cstr = cell( size( ray_slopes, 1 ), 1 );
    for i = 1 : size( ray_slopes, 1 )
        cstr{ i } = sprintf( 'x:%10f\ny:%10f', ray_slopes( i, 1 ), ray_slopes( i, 2 ) );
    end
    tp = text( normalized_positions( :, 1 ), normalized_positions( :, 2 ), cellstr( cstr ) );
    set( tp, 'visible', 'on', 'HorizontalAlignment', 'center', 'VerticalAlignment', 'middle' );
    box on;
    axis( [ -1 1 -1 1 ] );
    pbaspect( [ 1 1 1 ] );
end

function fig = plot_aberration( alpha, title, type )
    if nargin < 2, title = 'Wavefront Aberration'; end
    % input parameter space
    x = linspace( -1, 1, 20 );
    y = linspace( 1, -1, 20 );
    [ x, y ] = meshgrid( x, y );
    [ t, r ] = cart2pol( x, y );

    % evaluate the zernike polynoms
    z = [];
    for i = 1 : length( alpha )
        [ n, m ] = ZernikeIndices( i );
        zern_mat = alpha( i ) * elliptical_crop( Zernike( r, t, n, m ), 1 );
        if isempty( z )
            z = zern_mat;
        else
            z = z + zern_mat;
        end
    end

    % plot the aberration map
    fig = figure( 'Name', title, 'NumberTitle', 'Off' );
    if strcmp( type, '2D' )
        %surf([ -1 1 ],[ 1 -1 ], repmat( min( min( z ) ), [ 2 2 ]), z, 'facecolor', 'texture' );
        imshow( z, [ min( min( z ) ), max( max( z ) ) ], 'InitialMagnification', 'fit', 'Interpolation', "nearest" );
        colormap jet;
        colorbar;
    elseif strcmp( type, '3D' )
        surf( x, y, z );
        hold on;
        surf([ -1 1 ],[ 1 -1 ], repmat( min( min( z ) ), [ 2 2 ]), z, 'facecolor', 'texture' );
        % set the figure parameters
        colormap jet;
        colorbar;
        pbaspect( [ 1 1 1 ] );
        view(45,30);
        % setup axis labels
        xlabel( 'x' );
        ylabel( 'y' );
        zlabel( 'z' );
    end
end

function fig = plot_image( test_bench )
    fig = figure;
    imshow( test_bench.elem{ test_bench.cnt }.image );
end