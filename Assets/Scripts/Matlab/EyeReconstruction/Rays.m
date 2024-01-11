classdef Rays
    % RAYS Implements a ray bundle
    % Note that for easy copying Rays doesn't inherit from handle
    %
    % Member functions:
    %
    % r = Rays( n, geometry, r, dir, D, pattern, glass, wavelength, color, diopter ) - object constructor
    % INPUT:
    %   n - number of rays in the bundle
    %   geometry - For geometry 'collimated', r defines rays origins while dir - 
    % their direction. For geometry 'source', r defines position 
    % of the point source, and dir - direction along which rays propagate. For geometry
    % 'vergent' r defines rays orignis, dir - their average direction, while diopter 
    % defines the convergence/divergence of the rays in diopters.
    %   r - 1x3 bundle source position vector
    %   dir - 1x3 bundle direction vector
    %   D - diameter of the ray bundle (at distance 1 if geometry = 'source' )
    %   pattern - (optional) pattern of rays within the bundle: 'linear' = 'linearY', 'linearZ', 'hexagonal'
    %   'square', 'unicircle', 'hexcircle', or 'random', hexagonal by default
    %   glass - (optional) material through which rays propagate, 'air' by
    % default
    %   wavelength - (optional) wavelength of the ray bundle, meters, 557.7
    % nm by default
    %   color - (optional) 1 x 3 vector defining the color with which to draw
    % the ray, [ 0 1 0 ] (green) by default
    %
    % OUTPUT:
    %   r - ray bundle object
    %
    % r.draw( scale ) - draws the ray bundle r in the current axes as arrows
    % INPUT:
    %   scale - (optional) the arrow length, 1 by default
    %
    % [ rays_out, nrms ] = r.intersection( surf ) - finds intersection
    % of the ray bundle r with a surface
    % INPUT:
    %   surf - the surface
    % OUTPUT:
    %   rays_out - rays_out.r has the intersection points, the remaining
    % structure parameters might be wrong
    %   nrms - 1x3 normal vectors to the surface at the intersection points
    %
    % rays_out = r.interaction( surf ) - finishes forming the outcoming
    % ray bundle, calculates correct directions and intensities
    % INPUT:
    %   surf - surface
    % OUTPUT:
    %   rays_out - outcoming ray bundle
    %
    % r = r.append( r1 ) - appends to bundle r bundle r1
    % INPUT:
    %   r1 - the appended ray bundle
    % OUTPUT:
    %   r - the resulting ray bundle
    % 
    % sr = r.subset( inices ) - subset of rays in bundle r
    % INPUT:
    %   indices - subset's indices in the bundle r
    % OUTPUT:
    %   sr - the resulting new ray bundle
    % 
    % r = r.truncate() - truncate all rays with zero intensity from the bundle r.
    %
    % [ av, dv, nrays ] = r.stat() - return statistics on the bundle r
    % OUTPUT:
    %   av - 1x3 vector of the mean bundle position
    %   dv - standard deviation of the ray positions in the bundle
    %   nrays - number of rays with non-zero intensity in the bundle
    %
    % [ x0, cv, ax, ang, nrays ] = r.stat_ellipse() - fit a circumscribing ellipse
    % to the bundle r in the YZ plane
    % OUTPUT:
    %   x0 - 1x3 vector of the ellipse center
    %   cv - bundle covariance matrix
    %   ax - 1x2 vector of the ellipse half-axes lengths
    %   ang - angle of rotation of the ellipse from the longer axis being
    %   oriented along the Y axis.
    %   nrays - number of rays with non-zero intensity in the bundle
    %
    % r2 = dist2rays( p ) - returns squared distances from point p to all rays
    % INPUT:
    %   p - 1x3 vector
    % OUTPUT:
    %   r2 - nrays x 1 vector of squared distances
    %
    % [ f, ff ] = r.focal_point() - find a focal point of the bundle. The focal
    % point f is defined as the mean convergence distance of the bundle
    % rays. ff gives the residual bundle crossection (intensity weighted std).
    % OUTPUT:
    %    f - 1x3 vector for the focal point 
    %
    % Copyright: Yury Petrov, 2016
    %
    
    properties
        r = []; % a matrix of ray starting positions
        n = []; % a matrix of ray directions
        w = [];  % a vector of ray wavelengths
        I = [];         % a vector of ray intensities
        nrefr = [];    % a vector of current refractive indices
        att = [];       % a vector of ray attenuations
        color = [];   % color to draw the bundle rays
        cnt = 0;      % number of rays in the bundle
    end
    
    methods            
        function self = Rays( cnt, geometry, pos, dir, diameter, rflag, glass, wavelength, acolor, adiopter ) % constructor of ray bundles
            % Constructs a ray bundle comprising 'cnt' rays. For geometry 
            % 'collimated', 'pos' defines rays origins, while 'dir' - 
            % their direction. For geometry 'source', 'pos' defines position 
            % of the point source, 'dir' - direction along which rays form a 
            % linear, hexagonal, square, or random pattern (specified by 'rflag') of 
            % the size specified by 'diameter' at distance 1. For geometry
            % 'vergent' 'pos' defines rays orignis, 'dir' - their average
            % direction, while 'adiopter' defines the convergence/divergence
            % of the rays in diopters.
            
            if nargin == 0 % used to allocate arrays of Rays
                return;
            end
            if nargin < 10 || isempty( adiopter )
               diopter = 0;
            else
               diopter = adiopter;
            end
            if nargin < 9 || isempty( acolor )
                self.color = [ 0 1 0 ];
            else
                self.color = acolor;
            end
            if nargin < 8 || isempty( wavelength )
                self.w = 5300e-10; % green by default
            else
                self.w = wavelength;
            end
            if nargin < 7 || isempty( glass )
                glass = 'air';
            end
            if nargin < 6 || isempty( rflag )
                rflag = 'hexagonal'; % hexagonal lattice of rays
            end
            if nargin < 5 || isempty( diameter )
                diameter = 1;
            end
            if nargin < 4 || isempty( dir )
                dir = [ 1 0 0 ];
            end
            if nargin < 3 || isempty( pos )
                pos = [ 0 0 0 ];
            end
            if nargin < 2 || isempty( geometry )
                geometry = 'collimated';
            end
                    
            % normalize direction and rotate positions to the plane
            % orthogonal to their direction
            dir = dir ./ norm( dir );
            ex = [ 1 0 0 ];
            ax = cross( ex, dir );
            ang = asin( norm( ax ) );
            if dir( 1 ) < 0
                ax = -ax;
                ang = pi + ang;
            end

            if strcmp( rflag, 'linear' ) || strcmp( rflag, 'linearY' ) % extend along y-axis
                p( :, 1 ) = linspace( -diameter/2, diameter/2, cnt ); % all rays starting from the center
                p( :, 2 ) = 0;
            elseif strcmp( rflag, 'linearZ' ) % extend along z-axis
                p( :, 1 ) = zeros( 1, cnt );
                p( :, 2 ) = linspace( -diameter/2, diameter/2, cnt ); % all rays starting from the center
            elseif strcmp( rflag, 'random' )
                cnt1 = round( cnt * 4 / pi );
                p( :, 1 ) = diameter * ( rand( cnt1, 1 ) - 0.5 ); % horizontal positions
                p( :, 2 ) = diameter * ( rand( cnt1, 1 ) - 0.5 ); % vertical positions
                p( p( :, 1 ).^2 + p( :, 2 ).^2 > diameter^2 / 4, : ) = []; % leave rays only within the diameter
            elseif strcmp( rflag, 'hexagonal' ) || strcmp ( rflag, 'hexcircle' )
                % find the closest hexagonal number to cnt
                cn = find_hexagonal_number( cnt );
                % generate hexagonal grid
                p = [];
                for i = cn : -1 : -cn % loop over rows starting from the top
                    nr = 2 * cn + 1 - abs( i ); % number in a row
                    hn = floor( nr / 2 );
                    if rem( nr, 2 ) == 1
                        x = ( -hn : hn )';
                    else
                        x = ( -hn : hn - 1 )' + 1/2;
                    end
                    p = [ p; [ x, i * sqrt( 3 ) / 2 * ones( nr, 1 ) ] ]; % add new pin locations
                end
                if strcmp ( rflag, 'hexcircle' ) % remove corners
                    %p( p( :, 1 ).^2 + p( :, 2 ).^2 > ( cn * sqrt( 3 ) / 2 )^2, : ) = [];
                    p( p( :, 1 ).^2 + p( :, 2 ).^2 > ( cn * sqrt( 3 ) / 2 )^2, : ) = [];
                end
                if isscalar( diameter ) % extend the diameter to all directions
                    diameter = diameter * ones( 4, 1 );
                end
                if cn > 0 % scale the rays
                    scale = diameter / 2 / cn * 2 / sqrt( 3 );
                    
                    % circubscribe the hexagon by an inward circle
                    p( p(:,1) < 0, 1 ) = p( p(:,1) < 0, 1 ) * scale( 1 ); % left
                    p( p(:,1) > 0, 1 ) = p( p(:,1) > 0, 1 ) * scale( 2 ); % right
                    p( p(:,2) < 0, 2 ) = p( p(:,2) < 0, 2 ) * scale( 3 ); % bottom
                    p( p(:,2) > 0, 2 ) = p( p(:,2) > 0, 2 ) * scale( 4 ); % top
                end
            elseif strcmp( rflag, 'square' ) || strcmp( rflag, 'unicircle' )
                 % increase the number of rays for circular shapes
                if strcmp( rflag, 'unicircle' )
                    cnt = ceil( cnt * 4 / pi );
                end
                % find the closest square number to cnt
                cnt = ceil( sqrt( cnt ) )^2;
                % extend the diameter to all directions
                if isscalar( diameter )
                    diameter = diameter * ones( 4, 1 );
                end
                % area per ray and number of rays in each direction and 
                per = sqrt( 4 / cnt ); nr = ceil( 1 / per ); 
                % generate the grid
                [ x, y ] = meshgrid( -nr * per : per : nr * per, -nr * per : per : nr * per );
                p( :, 1 ) = y( : );
                p( :, 2 ) = x( : );
                % remove corners
                if strcmp( rflag, 'unicircle' )
                    p( p( :, 1 ).^2 + p( :, 2 ).^2 > 1, : ) = []; 
                end
                % scale the rays
                rad = diameter ./ 2;
                p( p(:,1) < 0, 1 ) = p( p(:,1) < 0, 1 ) * rad( 1 ); % left
                p( p(:,1) > 0, 1 ) = p( p(:,1) > 0, 1 ) * rad( 2 ); % right
                p( p(:,2) < 0, 2 ) = p( p(:,2) < 0, 2 ) * rad( 3 ); % bottom
                p( p(:,2) > 0, 2 ) = p( p(:,2) > 0, 2 ) * rad( 4 ); % top
            else
                error( [ 'Ray arrangement flag ' rflag ' is not defined!' ] );
            end

            self.cnt = size( p, 1 );            
            p = [ zeros( self.cnt, 1 ) p ]; % add x-positions
            pos = repmat( pos, self.cnt, 1 );
            if norm( ax ) ~= 0
                p = rodrigues_rot( p, ax, ang );
            end
            if strcmp( geometry, 'collimated' ) % parallel rays
                % distribute over the area
                self.r = pos + p;
                dir = repmat( dir, self.cnt, 1 );
                self.n = dir;
            elseif strcmp( geometry, 'source' ) || strcmp( geometry, 'source-Lambert' ) % assume p array at dir, source at pos.
                self.r = pos;
                self.n = p + repmat( dir, self.cnt, 1 );
            elseif strcmp( geometry, 'vergent' ) %
                % distribute over the area
                self.r = pos + p;
                if diopter == 0 % the same as collimated
                    dir = repmat( dir, self.cnt, 1 );
                    self.n = dir;
                else
                    self.n = p + repmat( 1000 * dir / diopter, self.cnt, 1 );
                end
                if diopter < 0
                    self.n = -self.n; % make ray normal point forward, as usual
                end
            else
                error( [ 'Source geometry' source ' is not defined!' ] );
            end
            % normalize directions
            self.n = self.n ./ repmat( sqrt( sum( self.n.^2, 2 ) ), 1, 3 );
                
            self.w = repmat( self.w, self.cnt, 1 );
            self.color = repmat( self.color, self.cnt, 1 );
            self.nrefr = refrindx( self.w, glass );
            self.I = ones( self.cnt, 1 );
            if strcmp( geometry, 'source-Lambert' )
                self.I = self.I .* self.n( :, 1 ); % Lambertian source: I proportional to cos wrt source surface normal assumed to be [ 1 0 0 ]
            end
            
            self.att = ones( self.cnt, 1 );
        end
        
            
        function draw( self, scale )
            if nargin == 0 || isempty( scale )
                scale = 1;
            end
            vis = self.I ~= 0;
            [ unique_colors, ~, ic ] = unique( self.color, 'rows' );
            nrms = scale * self.n;
            for i = 1 : size( unique_colors, 1 )
                cvis = vis & ( ic == i );
                quiver3( self.r( cvis, 1 ), self.r( cvis, 2 ), self.r( cvis, 3 ), ...
                         nrms( cvis, 1 ),   nrms( cvis, 2 ),   nrms( cvis, 3 ), ...
                         0, 'Color', unique_colors( i, : ), 'ShowArrowHead', 'off' );
            end
        end
         
        
        function [ rays_out, nrms ] = intersection( self, surf )
            % instantiate Rays object
            rays_out = self; % copy incoming rays
            
            switch class( surf )
                
                case { 'Aperture', 'Plane', 'Screen' } % intersection with a plane
                    % distance to the plane along the ray
                    d = dot( repmat( surf.n, self.cnt, 1 ), repmat( surf.r, self.cnt, 1 ) - self.r, 2 ) ./ ...
                        dot( self.n, repmat( surf.n, self.cnt, 1 ), 2 );
                    
                    % calculate intersection vectors and normals
                    rinter = self.r + repmat( d, 1, 3 ) .* self.n;
                    nrms = repmat( surf.n, self.cnt, 1 );
                    
                    % bring surface to the default position
                    rtr = rinter - repmat( surf.r, self.cnt, 1 );
                    if surf.rotang ~= 0
                        rtr = rodrigues_rot( rtr, surf.rotax, -surf.rotang ); % rotate rays to the default plane orientation
                    end
                    
                    rays_out.r = rinter;
                    rinter = rtr;
                    if isa( surf, 'Screen' ) % calculate retinal image
                        %{
                        wrong_dir = dot( nrms * sign( surf.R(1) ), self.n, 2 ) < 0;
                        self.I( wrong_dir ) = 0; % zero for the rays that point away from the screen for the image formation
                        rays_out.r( wrong_dir, : ) = Inf * rays_out.r( wrong_dir, : );
                        
                        surf.image = hist2( rtr( :, 2 ), rtr( :, 3 ), self.I, ...
                            linspace( -surf.w/2, surf.w/2, surf.wbins ), ...
                            linspace( -surf.h/2, surf.h/2, surf.hbins ) );
                        surf.image = flipud( surf.image ); % to get from matrix to image form
                        surf.image = fliplr( surf.image ); % because y-axis points to the left
                        %}
                    end
                    
                    % handle rays that miss the element
                    out = [];
                    if isprop( surf, 'w' ) && ~isempty( surf.w ) && isprop( surf, 'h' ) && ~isempty( surf.h )
                        out =  rinter( :, 2 ) < -surf.w/2 | rinter( :, 2 ) > surf.w/2 | ...
                            rinter( :, 3 ) < -surf.h/2 | rinter( :, 3 ) > surf.h/2;
                    elseif isprop( surf, 'D' ) && ~isempty( surf.D )
                        if length( surf.D ) == 1
                            out = sum( rinter( :, 2:3 ).^2, 2 ) - 1e-12 > ( surf.D / 2 )^2;
                        elseif length( surf.D ) == 2
                            r2 = sum( rinter( :, 2:3 ).^2, 2 );
                            out = ( r2 + 1e-12 < ( surf.D(1) / 2 )^2 ) | ( r2 - 1e-12 > ( surf.D(2) / 2 )^2 );
                        elseif length( surf.D ) == 4
                            out =  rinter( :, 2 ) > -surf.D(1)/2 & rinter( :, 2 ) < surf.D(1)/2 & ...
                                   rinter( :, 3 ) > -surf.D(2)/2 & rinter( :, 3 ) < surf.D(2)/2 | ...
                                   rinter( :, 2 ) < -surf.D(3)/2 | rinter( :, 2 ) > surf.D(3)/2 | ...
                                   rinter( :, 3 ) < -surf.D(4)/2 | rinter( :, 3 ) > surf.D(4)/2;
                        end
                    end
                    rays_out.I( out ) = -1 * rays_out.I( out ); % mark for processing in the interaction function
                    
                    if isa( surf, 'Screen' )  % do not draw rays that missed the screen
                        rays_out.I( out ) = 0;
                        %rays_out.I( wrong_dir ) = 0; 
                        rays_out.r( out, : ) = Inf;
                    elseif isa( surf, 'Aperture' )
                        rays_out.I( ~out ) = 0; % block the rays
                    end
                    
                case { 'ZernikeLens', 'Lens' 'Retina' } % intersection with a conical surface of rotation
                    % intersection between rays and the surface, also returns surface normals at the intersections
                    
                    % transform rays into the lens surface RF
                    r_in = self.r - repmat( surf.r, self.cnt, 1 ); % shift to RF with surface origin at [ 0 0 ]
                    
                    if surf.rotang ~= 0 % rotate so that the surface axis is along [1 0 0]
                        r_in = rodrigues_rot( r_in, surf.rotax, -surf.rotang ); % rotate rays to the default surface orientation
                        e = rodrigues_rot( self.n, surf.rotax, -surf.rotang );
                    else
                        e = self.n;
                    end
                    
                    if ~isa( surf, 'ZernikeLens' ) && size( surf.R, 2 ) > 1 % asymmetric quadric, scale z-dimension to make the surface symmetric
                        sc = surf.R( 1 ) / surf.R( 2 );
                        r_in( :, 3 ) = r_in( :, 3 ) * sc;
                        e( :, 3 ) = e( :, 3 ) * sc;
                    end
                    if isa( surf, 'ZernikeLens' )
                        rinter = zernlens_intersection( r_in, e, surf );
                        
                        % get surface normals at the intersection points
                        en = surf.funch( rinter( :, 2 ), rinter( :, 3 ), surf.funca, 1 );
                    else    
                        rinter = conic_intersection( r_in, e, surf );
                        
                        % find normals
                        r2yz = ( rinter( :, 2 ).^2 + rinter( :, 3 ).^2 ) / surf.R(1)^2; % distance to the lens center along the lens plane in units of lens R
                        if surf.k == -1 % parabola, special case
                            c = 1 ./ sqrt( 1 + r2yz );
                            s = sqrt( 1 - c.^2 );
                        else
                            s = sqrt( r2yz ) ./ sqrt( 1 - surf.k * r2yz );
                            c = sqrt( 1 - s.^2 );
                        end
                        s = -sign( surf.R(1) ) * s; % sign of the transverse component to the ray determined by the lens curvature
                        th = atan2( rinter( :, 3 ), rinter( :, 2 ) ); % rotation angle to bring r into XZ plane
                        en = [ c, s .* cos( th ), s .* sin( th ) ]; % make normal sign positive wrt ray

                        if isa( surf, 'Retina' ) % calculate retinal image
                            wrong_dir = dot( en, e, 2 ) < 0;
                            self.I( wrong_dir ) = 0; % zero for the rays that point away from the screen for the image formation
                            rinter( wrong_dir, : ) = NaN;
                            rinter( self.I == 0, : ) = NaN;
                            rinter( sqrt( sum( rinter.^2, 2 ) ) > realmax / 2, : ) = NaN;
                        end
                    end
                                        
                    % handle rays that miss the element
                    out = [];
                    if isprop( surf, 'w' ) && ~isempty( surf.w ) && isprop( surf, 'h' ) && ~isempty( surf.h )
                        out =  rinter( :, 2 ) < -surf.w/2 | rinter( :, 2 ) > surf.w/2 | ...
                            rinter( :, 3 ) < -surf.h/2 | rinter( :, 3 ) > surf.h/2;
                    elseif isprop( surf, 'D' ) && ~isempty( surf.D )
                        if length( surf.D ) == 1
                            out = sum( rinter( :, 2:3 ).^2, 2 ) - 1e-12 > ( surf.D / 2 )^2;
                        else
                            r2 = sum( rinter( :, 2:3 ).^2, 2 );
                            out = isnan( r2 ) | ( r2 + 1e-12 < ( surf.D(1) / 2 )^2 ) | ( r2 - 1e-12 > ( surf.D(2) / 2 )^2 );
                        end
                    end
                    
                    if isa( surf, 'Retina' )  % do not draw rays that missed the screen
                        rays_out.I( out ) = 0;
                        rays_out.r( out, : ) = Inf;
                    else
                        rays_out.I( out ) = -1 * rays_out.I( out ); % mark for processing in the interaction function
                    end
                    
                    % return to the original RF
                    if ~isa( surf, 'ZernikeLens' ) && size( surf.R, 2 ) > 1 % asymmetric quadric, unscale the z-dimension
                        sc = surf.R( 1 ) / surf.R( 2 );
                        rinter( :, 3 ) = rinter( :, 3 ) / sc;
                        en( :, 3 ) = en( :, 3 ) * sc; % normals transform as one-forms rather than vectors. Hence, divide by the scaling factor
                    elseif isa( surf, 'ZernikeLens' ) && size( surf.R, 2 ) > 1
                        sc = surf.R( 1 ) / surf.R( 2 );
                        %rinter( :, 3 ) = rinter( :, 3 ) / sc;
                        en( :, 3 ) = en( :, 3 ) * sc; % normals transform as one-forms rather than vectors. Hence, divide by the scaling factor
                    end
                    if surf.rotang ~= 0 % needs rotation
                        rays_out.r = rodrigues_rot( rinter, surf.rotax, surf.rotang );
                        nrms = rodrigues_rot( en, surf.rotax, surf.rotang );
                    else
                        rays_out.r = rinter;
                        nrms = en;
                    end
                    nrms = nrms ./ repmat( sqrt( sum( nrms.^2, 2 ) ), 1, 3 );
                    rays_out.r = rays_out.r + repmat( surf.r, self.cnt, 1 );
                    
                otherwise
                    error( [ 'Surface ' class( surf ) ' is not defined!' ] );
            end
        end
        
        function rays_out = interaction( self, surf, out_fl, block_fl, tot_fl )
            % INTERACTION calculates rays properties after interacting with
            % a Surface
            
            % find intersections and set outcoming rays starting points
            [ rays_out, nrms ] = self.intersection( surf );
            
            miss = rays_out.I < 0; % indices of the rays 
            med1 = surf.glass{1};
            med2 = surf.glass{2};
            
            % determine refractive indices before and after the surface 
            cs1 = dot( nrms, self.n, 2 ); % cosine between the ray direction and the surface direction
            opp_rays = cs1 < 0; %self.nrefr == refrindx( self.w, med2 ); %cs1 < 0; % rays hitting the surface from the opposite direction 
            old_refr( ~opp_rays ) = refrindx( self.w( ~opp_rays ), med1 ); % refractive index before the surface
            old_refr(  opp_rays ) = refrindx( self.w(  opp_rays ), med2 ); % refractive index before the surface
            %rays_out.color( opp_rays ) = 1 - rays_out.color( opp_rays );
            if strcmp( med2, 'mirror' )
                new_refr = refrindx( self.w, med1 ); % refractive index after the surface
            elseif  strcmp( med1, 'mirror' )
                new_refr = refrindx( self.w, med2 ); % refractive index after the surface
            else
                new_refr( ~opp_rays ) = refrindx( self.w( ~opp_rays ), med2 ); % refractive index after the surface
                new_refr(  opp_rays ) = refrindx( self.w(  opp_rays ), med1 ); % refractive index after the surface
            end
            old_refr = old_refr';
            if size( new_refr, 1 ) < size( new_refr, 2 )
                new_refr = new_refr';
            end

            % calculate refraction
            switch( class( surf ) )
                case { 'Aperture', 'Screen', 'Retina' } 
                case { 'ZernikeLens' 'Plane' 'Lens' }
                    % calculate refraction (Snell's law)
                    inside_already = ( ~miss ) & ( abs( rays_out.nrefr - old_refr ) > 1e-12 ); % rays that are already inside the surface (entered it previously)
                    rays_out.nrefr( ~miss ) = new_refr( ~miss ); % change refractive index of the rays that crossed the surface
                    if sum( inside_already ) ~= 0 % second intersections in a cylinder
                        rays_out.nrefr( inside_already ) = old_refr( inside_already ); % use old refractive index for those rays that are crossing the second surface
                    end
                    
                    if strcmp( med1, 'mirror' ) || strcmp( med2, 'mirror' ) % if a mirror
                        rays_out.n = self.n - 2 * repmat( cs1, 1, 3 ) .* nrms; % Snell's law of reflection
                        %rays_out.nrefr = refrindx( self.w, med1 ); % refractive index before the surface
                        if strcmp( med1, 'mirror' ) && strcmp( med2, 'air' ) % mirror facing away
                            rays_out.I( cs1 > 0 & ~miss ) = 0; % zero rays hitting such mirror from the back
                        elseif strcmp( med1, 'air' ) && strcmp( med2, 'mirror' ) % mirror facing toward me
                            rays_out.I( cs1 < 0 & ~miss ) = 0; % zero rays hitting such mirror from the back
                        end
                    elseif strcmp( med1, 'soot' ) || strcmp( med2, 'soot' ) % opaque black
                        rays_out.I( ~miss ) = 0; % zero rays that hit the element
                    else % transparent surface
                        rn = self.nrefr ./ rays_out.nrefr; % ratio of in and out refractive indices
                        cs2 = sqrt( 1 - rn.^2 .* ( 1 - cs1.^2 ) );
                        rays_out.n = repmat( rn, 1, 3 ) .* self.n - repmat( rn .* cs1 - sign( cs1 ) .* cs2, 1, 3 ) .* nrms; % refracted direction
                        tmp = cs1;
                        cs1( opp_rays ) = -cs1( opp_rays );
                        
                        % calculate transmitted intensity (Fresnel formulas)
                        rs = ( rn .* cs1 - cs2 ) ./ ( rn .* cs1 + cs2 );
                        rp = ( cs1 - rn .* cs2 ) ./ ( cs1 + rn .* cs2 );
                        refraction_loss = ( abs( rs ).^2 + abs( rp ).^2 ) / 2;
                        
                        % handle total internal reflection
                        tot = imag( cs2 ) ~= 0;
                        % rays_out.n( tot, : ) = 0; % zero direction for such rays
                        rays_out.n( tot, : ) = self.n( tot, : ) - 2 * repmat( tmp( tot ), 1, 3 ) .* nrms( tot, : ); % Snell's law of reflection
                        refraction_loss( tot ) = 0; %1;
                        rays_out.nrefr( tot ) = refrindx( self.w( tot ), med1 ); % refractive index before the surface
                        if tot_fl == 1 % exclude such rays
                            rays_out.I( tot ) = 0;
                            rays_out.r( tot, : ) = Inf;
                        end
                        rays_out.I( ~miss ) = ( 1 - refraction_loss( ~miss ) ) .* rays_out.I( ~miss ); % intensity of the outcoming rays
                        % color rays with total internal reflection
                        % differently
                        rays_out.color( tot, 1 ) = 1;
                        rays_out.color( tot, 2 ) = 0;
                        rays_out.color( tot, 3 ) = 0;
                    end                     
                otherwise
                    error( [ 'Surface ' class( surf ) ' is not defined!' ] );
            end
            
            % process rays that missed the element
            if out_fl == 0 || strcmp( med1, 'soot' ) || strcmp( med2, 'soot' ) % if tracing rays missing elements or for apertures
                % use the original rays here
                rays_out.I( miss ) = self.I( miss );
                rays_out.r( miss, : ) = self.r( miss, : );
                rays_out.n( miss, : ) = self.n( miss, : );
                if ~strcmp( med1, 'soot' ) && ~strcmp( med2, 'soot' )
                    rays_out.color( miss, 1 ) = 1;
                    rays_out.color( miss, 2 ) = 0;
                    rays_out.color( miss, 3 ) = 0;
                end
            else
                % default, exclude such rays
                rays_out.I( miss ) = 0;
                rays_out.r( miss, : ) = Inf;
            end
            rays_out.I( isnan( rays_out.I ) ) = 0;
            rays_out.I( any( isinf( rays_out.r ) ) ) = 0;
            rays_out.I( any( isnan( rays_out.r ) ) ) = 0;
            rays_out.I( rays_out.n( :, 1 ) .* self.n( :, 1 ) < 0 ) = 0; % zero rays that point back to the source
            
            % process rays that are blocked
            if block_fl == 1 
                blocked = rays_out.I <= 0; % indices of blocked rays
                rays_out.I( blocked ) = 0;
                rays_out.r( blocked, : ) = Inf;
            end
        end
        
        
        function rc = copy( self )
            rc = Rays;  % initialize the class instance
            rc.r = self.r; % a matrix of ray starting positions
            rc.n = self.n; % a matrix of ray directions
            rc.w = self.w;  % a vector of ray wavelengths
            rc.I = self.I;         % a vector of ray intensities
            rc.nrefr = self.nrefr;    % a vector of current refractive indices
            rc.att = self.att;       % a vector of ray attenuations
            rc.color = self.color;   % color to draw the bundle rays
            rc.cnt = self.cnt;      % number of rays in the bundle
        end
        
        
        function self = append( self, rays )
            % append rays to the current bundle
            self.r = [ self.r; rays.r ];
            self.n = [ self.n; rays.n ];
            self.w = [ self.w; rays.w ];
            self.I = [ self.I; rays.I ];
            self.nrefr = [ self.nrefr; rays.nrefr ];
            self.att = [ self.att; rays.att ];
            self.color = [ self.color; rays.color ];
            self.cnt = self.cnt + rays.cnt;                
        end
        
        function rays = subset( self, inds )
            % pick a subset of rays defined by inds in the current bundle
            rays = Rays; % allocate an instance of rays
            rays.r = self.r( inds, : );
            rays.n = self.n( inds, : );
            rays.w = self.w( inds, : );
            rays.I = self.I( inds, : );
            rays.nrefr = self.nrefr( inds, : );
            rays.att = self.att( inds, : );
            rays.color = self.color( inds, : );
            rays.cnt = length( inds );
        end
        
        function self = truncate( self, ind )
            if nargin < 2
                % default to rays with zero intensity
                ind = self.I == 0;
            end
            self.r( ind, : ) = [];
            self.n( ind, : ) = [];
            self.w( ind, : ) = [];
            self.I( ind, : ) = [];
            self.color( ind, : ) = [];
            self.nrefr( ind, : ) = [];
            self.att( ind, : ) = [];
            self.cnt = self.cnt - sum( ind );
        end
                            
        function [ av, dv, nrays ] = stat( self )
            % calculate mean and standard deviation of the rays startingpoints
            vis = self.I ~= 0; % visible rays
            norm = sum( self.I( vis ) );
            av = sum( repmat( self.I( vis ), 1, 3 ) .* self.r( vis, : ) ) ./ norm;
            dv = sqrt( sum( self.I( vis ) .* sum( ( self.r( vis, : ) - repmat( av, sum( vis ), 1 ) ).^2, 2 ) ) ./ norm );
            nrays = sum( vis );
        end
        
        function [ av, dv ] = stat_sph( self, sph_pos )
            % calculate mean and standard deviation of the rays
            % startingpoints in spherical coordinates (e.g., on a retina)
            % returns average and std for [ azimuth, elevation, radius ]
            rs = self.r - sph_pos; % coordinates wrt sphere center
            [ az, el, rad ] = cart2sph( rs( :, 1 ), rs( :, 2 ), rs( :, 3 ) ); % [ az el r ]
            sph = [ az el rad ];
            vis = self.I ~= 0; % visible rays
            norm = sum( self.I( vis ) );
            av = sum( repmat( self.I( vis ), 1, 2 ) .* sph( vis, 1:2 ) ) ./ norm;
            dv = sqrt( sum( self.I( vis ) .* sum( ( sph( vis, 1:2 ) - repmat( av, sum( vis ), 1 ) ).^2, 2 ) ) ./ norm );
        end
        
        function r2 = dist2rays( self, p )
            t = self.r - repmat( p, self.cnt, 1 ); % vectors from the point p to the rays origins
            proj = t * self.n'; % projections of the vectors t onto the ray
            r2 = sum( ( t - repmat( proj, 1, 3 ) .* self.n ).^2 );
        end
        
        function [ f, ff ] = focal_point( self, flag )
            if nargin < 2
                flag = 0;
            end
            ind = self.I ~= 0;
            sn = self.n( ind, : );
            sr = self.r( ind, : );
            si = self.I( ind, : );
            repI = repmat( si , 1, 3 );
            nav = sum( sn .* repI, 1 ); % average bundle direction
            nav = nav / sqrt( sum( nav.^2, 2 ) ); % normalize the average direction vector
            tmp = repmat( nav, size( sr, 1 ), 1 );
            osr = tmp .* repmat( dot( sr, tmp, 2 ), 1, 3 );
            sr = sr - osr; % leave only the r component orthogonal to the average bundle direction.
            osr = sum( osr .* repI ) / sum( self.I ); % bundle origin along the bundle direction
            rav = sum( sr .* repI ) / sum( self.I ); % average bundle origin 

            if flag == 2 % search for the plane with the smallest cross-section
                scnt = sum( ind );
                options = optimoptions( 'fminunc', 'Algorithm', 'quasi-newton', 'Display', 'off', 'Diagnostics', 'off' );
                plr = rav + nav * fminunc( @scatterRayPlaneIntersect, 0, options, sr, sn, si, rav, nav ); % optimal plane position
                d = dot( repmat( nav, scnt, 1 ), repmat( plr, scnt, 1 ) - sr, 2 ) ./ ...
                    dot( sn, repmat( nav, scnt, 1 ), 2 );
                % calculate intersection vectors
                rinter = sr + repmat( d, 1, 3 ) .* sn; % intersection vectors
                f = mean( rinter ); % assign focus to the mean of the intersection points
                ff = mean( sqrt( sum( ( rinter - repmat( f, size( rinter, 1 ), 1 ) ).^2, 2 ) ) );
            else % use average cosine
                dr = sr - repmat( rav, size( sr, 1 ), 1 );
                ndr = sqrt( sum( dr.^2, 2 ) );
                drn = dr ./ repmat( ndr, 1, 3 ); % normalize the difference of origins vector
                dp = dot( sn, drn, 2 );
                if flag == 1  % only consider rays in the bundle which are diverging, the ones an eye can focus
                    ind = sign( dp ) > 0;
                    if sum( ind ) == 0
                        ind = ~ind; % consider the other half then
                    end
                else
                    ind = ones( size( si ) );
                end
                ssi = sum( si( ind ) ); % normalization factor for the weighted average
                cs = -sum( dp( ind ) .* si( ind ) ) / ssi; % mean cosine of the convergence angles
                d = sum( ndr( ind ) .* si( ind ) ) / ssi * sqrt( 1 - cs^2 ) / cs; % distance to the focus along the mean direction
                f = osr + d * nav; % focal point
                % calculate ray positions at the focal plane
                rf = dr + repmat( d ./ sqrt( 1 - dp.^2 ), 1, 3 ) .* sn;
                arf = mean( rf ); % average vector
                ff = sum( si .* sqrt( sum( ( rf - repmat( arf, size( rf, 1 ), 1 ) ).^2, 2 ) ) ) / ssi; % weighted average bundle size on the focal plane
            end               
        end
        
        function [ mtf, fr ] = MTF( self, dist, fr )
            if nargin < 3
                fr = linspace( 0, 50, 100 ); % frequencies to calculate MTF at
            end           
            ind = self.I ~= 0;
            sn = self.n( ind, : );
            sr = self.r( ind, : );
            si = self.I( ind, : );
            repI = repmat( si , 1, 3 );
            nav = sum( sn .* repI, 1 ); % average bundle direction
            nav = nav / sqrt( sum( nav.^2, 2 ) ); % normalize the average direction vector
            tmp = repmat( nav, size( sr, 1 ), 1 );
            osr = tmp .* repmat( dot( sr, tmp, 2 ), 1, 3 );
            sr = sr - osr; % leave only the r component orthogonal to the average bundle direction.
            osr = sum( osr .* repI ) / sum( self.I ); % bundle origin along the bundle direction
            rav = sum( sr .* repI ) / sum( self.I ); % average bundle origin
            
            [ ~, rinter ] = scatterRayPlaneIntersect( dist, sr, sn, si, rav, nav );
            % rotate the intersection vectors for the average to face in the x-direction
            rv = cross( nav, [ 1 0 0 ] );
            ra = asin( norm( rv ) );
            rinrot = rodrigues_rot( rinter, rv, ra );
            yz = rinrot( :, 2:3 );
            lsfY = 180 / pi * atan( ( yz( :, 1 ) - mean( yz( :, 1 ) ) ) / dist ); % convert to degrees
            lsfZ = 180 / pi * atan( ( yz( :, 2 ) - mean( yz( :, 2 ) ) ) / dist ); % convert to degrees
            %figure, scatter( lsfY, lsfZ, '*' ), axis equal;
            mtf = sum( cos( 2 * pi * lsfZ * fr ) );
            mtf = mtf / mtf( 1 );
        end
   end
end


