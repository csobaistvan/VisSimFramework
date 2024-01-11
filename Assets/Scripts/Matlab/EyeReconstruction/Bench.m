classdef Bench < handle
    % Bench class implements a system of optical elements
    % A complex optical system can be stored and manipulated as a whole by
    % making it a Bench instance.
    %
    % Member functions:
    %
    % b = Bench( obj )  - constructor function
    % INPUT:
    %   obj - an optical element, cell array of elements, or another bench
    % OUTPUT:
    %   b - bench object
    %
    % b.display() - displays bench b's information
    %
    % b.draw( rays, draw_fl, alpha, scale, new_figure_fl ) - draws bench b in the current axes
    % INPUT:
    %   rays - array of rays objects comprising full or partial light path
    %   draw_fl - display rays as 'arrows' (default), 'lines', or 'rays'
    %   alpha - opacity of optical surfaces from 0 to 1, default .33
    %   scale - scale of the arrow heads for the 'arrows' draw_fl
    %   new_figure_fl - 0, do not open, or 1, open (default)
    % 
    % a = b.copy() - copies bench b to bench a
    %
    % b.append( a, n ) - appends element a to bench b n times. n > 1
    % corresponds to multiple possible interactions (internal reflections
    % and such).
    %
    % b.prepend( a, n ) - prepends element a to bench b n times
    %
    % b.replace( ind, a ) - replaces an element with index ind on bench b with element a
    %
    % b.remove( inds ) - removes elements located at inds on bench b
    %
    % b.rotate( rot_axis, rot_angle, rot_fl ) - rotate the bench b with all its elements
    % INPUT:
    %   rot_axis - 1x3 vector defining the rotation axis
    %   rot_angle - rotation angle (radians)
    %   rot_fl - (0, default) rotation of the bench elements wrt to the
    %   global origin, (1) rotation wrt to the bench geometric center
    %   
    % b.translate( tr_vec ) - translate the bench b with all its elements
    % INPUT:
    %   tr_vec - 1x3 translation vector
    %
    % rays_through = b.trace( rays_in, out_fl ) - trace rays through optical elements
    % on the bench b
    % INPUT:
    %   rays_in - incoming rays, e.g., created by the Rays() function
    %   out_fl  - 0 include even rays that missed some elements on the
    %   bench,  - 1 (default) exlude such rays
    % OUTPUT:
    %   rays_through - a cell array of refracted/reflected rays of the same
    %   length as the number of optical elements on the bench.
    %
    % Copyright: Yury Petrov, 2016
    %
  
    properties
        elem = {};     % cell array of optical elements
        cnt = 0;       % counter of elements in the system
    end
    
    methods
        function self = Bench( obj )
            % b = Bench( obj )  - constructor function
            % INPUT:
            %   obj - an optical element, cell array of elements, or another bench
            % OUTPUT:
            %   b - bench object
            if nargin == 0
                return;
            end
            
            if isa( obj, 'Bench' ) % if another Bench
                obj = obj.elem;    % extract elements
            end
            
            % append object(s) to the optical system
            nobj = length( obj );
            for i = 1 : nobj
                self.cnt = self.cnt + 1;
                if nobj == 1
                    self.elem{ self.cnt } = obj;
                elseif iscell( obj )   % other benches or cell arrays of Surfaces
                    self.elem{ self.cnt } = obj{ i };
                elseif isvector( obj ) % Rays
                    self.elem{ self.cnt } = obj( i );
                end
            end
        end
        
        function b = copy( self )
            % a = b.copy() - copies bench b to bench a
            b = feval( class( self ) );
            p = properties( self );
            for i = 1:length( p )
                b.( p{ i } ) = self.( p{ i } );
            end
            for i = 1 : length( self.elem )
                b.elem{ i } = self.elem{ i }.copy;
            end
        end
         
        function display( self )
            % b.display() - displays bench b's information
            for i = 1 : self.cnt
                obj = self.elem{ i };
                fprintf( '\n%s:\n', class( obj ) );
                obj.display;
            end
         end
        
        function fig = draw( self, rays, draw_fl, alpha, scale, plot_type, cam_rot, new_figure_fl )
            % b.draw( rays, draw_fl, alpha, scale, new_figure_fl ) - draws bench b in the current axes
            % INPUT:
            %   rays - array of rays objects comprising full or partial light path
            %   draw_fl - display rays as 'arrows' (default), 'lines', or 'rays'
            %   alpha - opacity of optical surfaces from 0 to 1, default .33
            %   scale - scale of the arrow heads for the 'arrows' draw_fl
            %   plot_type - type of plot ('2D' or {'3D'})
            %   cam_rot - camera rotation angles (default [ -54, 54 ])
            %   new_figure_fl - 0, do not open, or 1, open (default)
            if nargin < 6 || isempty( plot_type )
                plot_type = '3D';
            end
            if nargin < 7 || isempty( cam_rot )
                cam_rot = [ -54, 54 ];
            end
            if nargin < 8 || isempty( new_figure_fl )
                new_figure_fl = 1; % open a new figure by default
            end
            if nargin < 5 || isempty( scale )
                if nargin > 1
                    scale = ones( 1, length( rays ) );
                else
                    scale = 1;
                end
            else
                if length( scale ) == 1
                    scale = repmat( scale, 1, length( rays ) ); % make all ones
                elseif length( scale ) < length( rays )
                    if size( scale, 1 ) > size( scale, 2 )
                        scale = scale';
                    end
                    scale = [ scale ones( 1, length( rays ) - length( scale ) ) ]; % append ones 
                end
            end
            if nargin < 4 || isempty( alpha )
                alpha = 0.33;
            end
            if nargin < 3 || isempty( draw_fl )
                draw_fl = 'arrows';
            end
            if nargin < 2 || isempty( rays )
                rays = [];
            end
            
            if new_figure_fl == 1
                fname = dbstack;  % get debugging info
                [ ~, fname ] = fname.name; % get the second (original) call function name
                fig = figure( 'Name', [ 'OPTOMETRIKA: ' fname ], 'NumberTitle', 'Off', ...
                    'Position', [ 0 0 1024 1024 ], ...
                    'Color', 'w' );
            end
            hold on;
            for i = 1 : self.cnt
                obj = self.elem{ i };
                if isprop( obj, 'glass' ) && ( strcmp( obj.glass{1}, 'soot' ) || strcmp( obj.glass{2}, 'soot' ) )
                    color = [ .25 .25 .25 1 ];
                    obj.draw( color, plot_type );
                elseif strcmp( class( obj ), 'Retina' )
                    color = [ 0.2, 0.2, 0.2, alpha ];
                    obj.draw( color, plot_type );
                else
                    color = [ 1 1 1 alpha ];
                    obj.draw( color, plot_type );
                end
            end
            
            if ~isempty( rays )
                if strcmp( draw_fl, 'lines' ) || strcmp( draw_fl, 'clines' ) || strcmp( draw_fl, 'rays' ) % draw ray bundles as lines
                    if strcmp( draw_fl, 'lines' )
                        sym = '-';
                    else
                        sym = '*:';
                    end
                    switch plot_type
                        case '3D'
                            axvis = ones( size( rays( 1 ) ) );
                            ax = [ 1 2 3 ];
                        case 'wireframe'
                            axvis = ones( size( rays( 1 ) ) );
                            ax = [ 1 2 3 ];
                        case 'XY'
                            axvis = abs( rays( 1 ).n( :, 3 ) ) < 1e-3;
                            ax = [ 1 2 3 ];
                        case 'XZ'
                            axvis = abs( rays( 1 ).n( :, 2 ) ) < 1e-3;
                            ax = [ 1 3 2 ];
                    end
                    for i = 1 : length( rays ) - 1
                        vis = ( rays( i ).I ~= 0 ) & ...
                                isfinite( sum( rays( i ).r.^2, 2 ) ) & ...
                                isfinite( sum( rays( i + 1 ).r.^2, 2 ) );  % visible rays
                        %fprintf('number of rays: %d\n', nnz( vis ) );
                        real = dot( rays( i + 1 ).r - rays( i ).r, rays( i ).n, 2 ) > 0; % real rays (vs. virtual for virtual image)
                        [ unique_colors, ~, ic ] = unique( rays( i ).color, 'rows' );
                        switch plot_type
                            case '3D'
                                linewidth = 0.5;
                            case 'wireframe'
                                linewidth = 0.5;
                            case 'XY'
                                linewidth = 1.0;
                            case 'XZ'
                                linewidth = 1.0;
                        end
                        for j = 1 : size( unique_colors, 1 )
                            cvis = axvis & vis & real & ( ic == j );
                            plot3( [ rays( i ).r( cvis, ax( 1 ) )';  rays( i + 1 ).r( cvis, ax( 1 ) )' ], ...
                                   [ rays( i ).r( cvis, ax( 2 ) )';  rays( i + 1 ).r( cvis, ax( 2 ) )' ], ...
                                   [ rays( i ).r( cvis, ax( 3 ) )';  rays( i + 1 ).r( cvis, ax( 3 ) )' ], ...
                                   sym, 'LineWidth', linewidth, 'Color', unique_colors( j, : ) );
                        end
                   end
                elseif strcmp( draw_fl, 'arrows' )
                    for i = 1 : length( rays )
                        rays( i ).draw( scale( i ) );
                    end
                end
            end
            
            if new_figure_fl == 1
                axis equal vis3d on;
                ax = gca;
                ax.Clipping = 'off';
                ax.YDir = 'reverse';
                camlight( 'left' );
                camlight( 'right' );
                camlight( 'headlight' );
                lighting phong;
                %grid on;
                
                switch plot_type
                    case '3D'
                        rotate3d on;
                        xlabel( 'x' );
                        ylabel( 'y' );
                        zlabel( 'z' );
                        view( cam_rot );
                    case 'wireframe'
                        rotate3d on;
                        xlabel( 'x' );
                        ylabel( 'y' );
                        zlabel( 'z' );
                        view( cam_rot );
                    case 'XY'
                        rotate3d off;
                        xlabel( 'x' );
                        ylabel( 'y' );
                        zlabel( 'z' );
                        view( cam_rot );
                    case 'XZ'
                        rotate3d off;
                        xlabel( 'x' );
                        ylabel( 'z' );
                        zlabel( 'y' );
                        view( cam_rot + [ 0 -90 ] );
                end
            end
        end
        
        function append( self, obj, mult )
            % b.append( a, n ) - appends element a to bench b n times. n > 1
            % corresponds to multiple possible interactions (internal reflections
            % and such).
            if nargin < 3
                mult = 1;
            end
            if isa( obj, 'Bench' ) % if another Bench
                obj = obj.elem;    % extract elements
            end           
            % append object(s) to the optical system
            nobj = length( obj );
            for m = 1 : mult
                for i = 1 : nobj
                    self.cnt = self.cnt + 1;
                    if nobj == 1
                        self.elem{ self.cnt } = obj;
                    elseif iscell( obj )   % other benches or cell arrays of Surfaces
                        self.elem{ self.cnt } = obj{ i };
                    elseif isvector( obj ) % Rays
                        self.elem{ self.cnt } = obj( i );
                    end
                end
            end
        end
        
        function prepend( self, obj, mult )
            % b.prepend( a, n ) - prepends element a to bench b n times
            if nargin < 3
                mult = 1;
            end
            if isa( obj, 'Bench' ) % if another Bench
                obj = obj.elem;    % extract elements
            end         
            self.elem = fliplr( self.elem ); % reverse element direction temporarily
            % prepend object(s) to the optical system
            nobj = length( obj );
            for m = 1 : mult
                for i = nobj : -1 : 1 % append in the opposite order
                    self.cnt = self.cnt + 1;
                    if nobj == 1
                        self.elem{ self.cnt } = obj;
                    elseif iscell( obj )   % other benches or cell arrays of Surfaces
                        self.elem{ self.cnt } = obj{ i };
                    elseif isvector( obj ) % Rays
                        self.elem{ self.cnt } = obj( i );
                    end
                end
            end
            self.elem = fliplr( self.elem ); % restitute the original order
        end
        
        function replace( self, ind, obj )
            % b.replace( ind, a ) - replaces an element with index ind on bench b with element a
            self.elem{ ind } = obj;
        end
        
        function remove( self, inds )
             % b.remove( inds ) - removes elements located at inds on bench b
             if self.cnt == 0
                 error( 'The bench is already empty!' );
             else
                self.elem( inds ) = [];
                self.cnt = self.cnt - length( inds );
             end
         end
         
        function rotate( self, rot_axis, rot_angle, rot_fl )
             % b.rotate( rot_axis, rot_angle, rot_fl ) - rotate the bench b with all its elements
             % INPUT:
             %   rot_axis - 1x3 vector defining the rotation axis
             %   rot_angle - rotation angle (radians)
             %   rot_fl - (0, default) rotation of the bench elements wrt to the
             %   global origin, (1) rotation wrt to the bench geometric center
             if nargin < 4
                 rot_fl = 0;
             end
             cntr = [ 0 0 0 ];
             if rot_fl == 1 % rotate around the geometric center of the bench
                 for i = 1 : self.cnt % loop through the optic system
                     cntr = cntr + self.elem{ i }.r;
                 end
                 cntr = cntr / self.cnt;
             end
            % rotate bench elements
            for i = 1 : self.cnt % loop through the optic system
                self.elem{ i }.rotate( rot_axis, rot_angle ); % rotate normal
                self.elem{ i }.r = cntr + rodrigues_rot( self.elem{ i }.r - cntr, rot_axis, rot_angle ); % rotate position
            end
            if abs( rot_angle ) > pi/2 % reverse order in which the elements are encountered by rays
                self.elem = fliplr( self.elem );
            end
        end

        function reverse( self )
            % b.reverse( ) - reverses the order of elements in the bench
            self.elem = fliplr( self.elem );
        end
        
        function translate( self, tr_vec )
            % b.translate( tr_vec ) - translate the bench b with all its elements
            % INPUT:
            %   tr_vec - 1x3 translation vector
            for i = 1 : self.cnt % loop through the optic system
                self.elem{ i }.r = self.elem{ i }.r + tr_vec; % translate position
            end
        end
        
        function rays = trace( self, rays_in, out_fl, block_fl, tot_fl )
            % rays_through = b.trace( rays_in, out_fl ) - trace rays through optical elements
            % on the bench b
            % INPUT:
            %   rays_in  - incoming rays, e.g., created by the Rays() function
            %   out_fl   - 0 include even rays that missed some elements on the
            %   bench,   - 1 (default) exlude such rays
            %   block_fl - 0 include even rays that were blocked by some elements
            %            - 1 (default) exlude such rays
            %   tot_fl   - 0 include rays that suffered total internal reflection
            %            - 1 (default) exlude such rays
            % OUTPUT:
            %   rays_through - a cell array of refracted/reflected rays of the same
            %   length as the number of optical elements on the bench.
            
            if nargin < 3, out_fl = 1; end
            if nargin < 4, block_fl = 1; end
            if nargin < 5, tot_fl = 1; end
            
            rays( 1, self.cnt + 1 ) = Rays;
            rays( 1 ) = rays_in;
            for i = 1 : self.cnt
                rays( i + 1 ) = rays( i ).interaction( self.elem{ i }, out_fl, block_fl, tot_fl );
            end
        end
        
        function aperture_id = find_aperture_id( self )
            % b.find_aperture_id( ) - computes the index of the (first)
            % aperture element
            %
            % OUTPUT:
            %   aperture_id - index of the aperture element, if any
            for i = 1 : self.cnt
                if isa( self.elem{ i }, 'Aperture' )
                    aperture_id = i;
                    return;
                end
            end
            
            % throw an error
            error( 'This bench has no aperture element!' );
        end
        
        function capture_bench = create_capture_bench( self, trace_dir, last_elem_id )
            % capture_bench = b.create_capture_bench( rays_in, out_fl ) - 
            %   constructs a bench suitable for capturing rays after an
            %   arbitrary element
            % INPUT:
            %   trace_dir - 'inside' rays are coming from inside the bench
            %             - 'outside' rays are coming from outside the bench
            %
            % OUTPUT:
            %   capture_bench - resulting modified copy of the input bench
            
            % setup the capture bench
            capture_bench = self.copy( );
            capture_bench.elem{ self.find_aperture_id( ) }.D( 2 ) = 1e6; % enlarge the pupil to make sure no stray rays can pass through
            capture_bench.cnt = last_elem_id + 1;
            capture_bench.elem{ last_elem_id + 1 } = Screen( [ self.elem{ last_elem_id }.r( 1 ), 0, 0 ], 20, 20, 20, 20 );
            if strcmp( trace_dir, 'inside' ) % rotate the screen if we are capturing from inside
                capture_bench.elem{ last_elem_id + 1 }.rotate( [ 0 0 1 ], deg2rad( 180 ) );
            elseif strcmp( trace_dir, 'outside' )
                % do nothing
            else
                error( 'Unknown trace direction: %s', trace_dir );
            end
        end
        
        function capture_bench = create_pupil_capture_bench( self, trace_dir )
            capture_bench = self.create_capture_bench( trace_dir, self.find_aperture_id( ) );
        end
        
        function [P, d, n] = find_chief_ray_position( self, ah, av, l, nr, tol )
            % [P, d, p, n] = b.find_chief_ray_position( ah, av, l ) - 
            %   finds the proper starting position for the chief ray
            % INPUT:
            %   ah  - horizontal input angle (radians)
            %   av  - vertical input angle (radians)
            %   l   - wavelength to trace at (nanometers)
            %   nr  - number of rays to test with (default: 50)
            %   tol - stop tolerance (default: 5e-3)
            %
            % OUTPUT:
            %   P  - chief ray input position
            %   d  - distance of the ray from the pupil
            %   n  - chief ray direction
            
            % set defaults
            if nargin < 5, nr = 100; end
            if nargin < 6, tol = 5e-3; end
            
            % parameters of the pupil
            pupil_id = self.find_aperture_id( ); % find the index of the aperture element
            pupil_diameter = self.elem{ pupil_id }.D( 1 ); % extract its diameter
            first_elem_diameter = self.elem{ 1 }.D( 2 ); % extract the diameter of the first element
            
            % construct a test bench
            test_bench = self.create_pupil_capture_bench( 'outside' );
        
            % determine the trace parameters
            [ x, y, z ] = sph2cart( -ah, -av, -1e3  );
            trace_source = [ x y z ];
            trace_dir = -trace_source / norm( trace_source );

            % create the input rays and trace them
            keep_looping = true;
            source = trace_source;
            diameter = first_elem_diameter / 2;
            while keep_looping
                % trace through a test grid
                nr_factor = max( min( power( diameter, 1 / 8 ), 1.0 ), 0.1 );
                num_rays = max( nr * nr_factor, 15 );
                rays_in = Rays( num_rays, 'collimated', source, trace_dir, diameter, 'square', 'air', l * 1e-9 );
                rays_through = test_bench.trace( rays_in, 1, 1, 1 );
                %test_bench.elem{ test_bench.find_aperture_id( ) }.D( 2 ) = 1e1;
                %test_bench.draw( rays_through, 'lines', 0.33, 1.0, '3D', [ 0, 0 ] );
                %test_bench.elem{ test_bench.find_aperture_id( ) }.D( 2 ) = 1e6;

                % determine the valid input and pupil positions
                valid_rays = rays_through( end ).I > 0;
                input_pos = rays_through( 1 ).r( valid_rays, : );
                pupil_pos = rays_through( end ).r( valid_rays, 2:3 );

                % determine the distance of each ray from the optical axis
                valid_ray_ids = find( valid_rays );
                num_valid_rays = length( valid_ray_ids );
                
                % update the best position
                if num_valid_rays > 0                
                    [best_dist, best_id] = min( vecnorm( pupil_pos, 2, 2 ) );
                    trace_source = input_pos( best_id, : );
                    trace_source( 2:3 ) = trace_source( 2:3 ) - pupil_pos( best_id, : );
                else
                    best_dist = 100.0;
                end
                
                %{
                fprintf( 'best: %f, #valid: %d, source: (%f, %f, %f), diameter: %f\n', ...
                    best_dist, num_valid_rays, source( 1 ), source( 2 ), source( 3 ), diameter );
                %}

                % we can finish if we are close enough to the axis, or if
                % we didn't find anything
                keep_looping = ( best_dist > tol ) && diameter > tol;

                % update the trace parameters if we need to continue
                if keep_looping
                    % how much to contract the diameter
                    contraction_factor = 0.25;
                    
                    % update the trace source and diameter
                    source = trace_source;
                    diameter = diameter * contraction_factor;
                end
            end
            
            %test_bench.elem{ test_bench.find_aperture_id( ) }.D( 2 ) = 1e1;
            %test_bench.draw( rays_through, 'lines', 0.33, 1.0, 'XZ', [ 0, 0 ] );
            
            % write out the result
            P = trace_source;
            d = best_dist;
            n = trace_dir;
        end
        
        function [rays, diameter, radius_ratio] = fit_ray_grid_to_pupil( self, trace_dir, dsteps, nrays, geometry, src, dir, pattern, material, lambda )
            % [rays, spread] = b.fit_ray_grid_to_pupil( nrays, geometry, src, dir, pattern, material, lambda ) - 
            %   fits a ray grid with the input parameters to the aperture
            % INPUT:
            %   trace_dir - direction in which the rays should be going
            %   dsteps    - number of diameter fit steps to perform
            %   nrays     - number of rays to pass through the pupil
            %   geometry  - ray grid geometry
            %   src       - ray source location
            %   dir       - ray grid direction
            %   pattern   - ray pattern
            %   material  - material in which the rays start
            %   lambda    - ray grid wavelength
            %
            % OUTPUT:
            %   rays         - resulting, optimized ray grid
            %   diameter     - diameter with which the result was achieved
            %   radius_ratio - ratio of pupil and input grid radii
            
            % get the necessary aperture parameters
            pupil_id = self.find_aperture_id( ); % find the index of the aperture element
            pupil_position = self.elem{ pupil_id }.r; % extract its position
            pupil_diameter = self.elem{ pupil_id }.D( 1 ); % extract the diameter of the opening hole
            pupil_distance = abs( src( 1 ) - pupil_position( 1 ) ); % get its distance from the source location
    
            % construct a bench without all the elements following the pupil
            test_bench = self.create_pupil_capture_bench( trace_dir );

            % calculate the diameter for the grid
            if strcmp( geometry, 'collimated' )
                diameter = 2 * pupil_diameter;
            elseif strcmp( geometry, 'source' )
                diameter = ( 2 * pupil_diameter / pupil_distance );
            end

            % keep track of the ray count and number of valid rays
            num_valid_rays = 0;
            radius_ratio = 0.0;
            test_nrays = nrays;

            % keep increasing the grid size until the number of valid rays
            % reaches the desired amount
            while num_valid_rays < nrays
                % construct the test rays
                rays_in = Rays( test_nrays, geometry, src, dir, diameter, pattern, material, lambda );

                % trace the rays through the test optical system
                rays_through = test_bench.trace( rays_in, 1, 1 );

                % extract the valid rays
                valid_rays = rays_through( test_bench.cnt + 1 ).I > 0;
                valid_ray_ids = find( valid_rays );
                num_valid_rays = length( valid_ray_ids );

                % fit an ellipse around the points
                pupil_positions = rays_through( test_bench.cnt + 1 ).r( valid_rays, : );
                ch_points = pupil_positions( convhull( pupil_positions( :, 2:3 ), 'Simplify', true ), 2:3 );
                %circle = TaubinSVD( ch_points );
                %r = circle( 3 );
                ell = MinVolEllipseFit( ch_points, 5e-4 );
                r = min( ell( 3:4 ) );
                radius_ratio = ( 2 * r / pupil_diameter );

                %{
                fprintf( 'rays: %d, valid rays: %d, radius ratio: %f\n', ...
                    test_nrays, num_valid_rays, radius_ratio );
                %}

                % increase the ray count if we are still below
                if num_valid_rays < nrays
                    test_nrays = ceil( test_nrays * ( nrays / num_valid_rays ) );

                % otherwise just stop looping
                else
                    rays = rays_in.subset( valid_ray_ids );
                end
            end
            
            %test_bench.elem{ test_bench.find_aperture_id( ) }.D( 2 ) = 1e1;
            %test_bench.draw( rays_through, 'lines', 0.33, 1.0, '3D', [ 0, 0 ] );

            %{
            fprintf( 'diameter: %f, target: %d, #valid: %d, radius: %f, ratio: %f\n', ...
                diameter, nrays, num_valid_rays, max( r ), radius_ratio );
            %}

            % parameters for increasing the diameter
            batch_mul = 0.1;
            step_size = min( diameter * 0.1, 0.01 );
            if dsteps > 1, step_size = step_size * 10; end
            batches_left = dsteps;

            % now try to increase the diameter
            keep_looping = batches_left > 0;
            while keep_looping
                % create the test diameter
                test_diameter = diameter + step_size;

                % construct the test rays
                rays_in = Rays( test_nrays, geometry, src, dir, test_diameter, pattern, material, lambda );

                % trace the rays through the test optical system
                rays_through = test_bench.trace( rays_in, 1, 1 );

                % extract the valid rays
                valid_rays = rays_through( test_bench.cnt + 1 ).I > 0;
                valid_ray_ids = find( valid_rays );
                num_valid_rays_test = length( valid_ray_ids );

                % fit an ellipse around the points
                try
                    pupil_positions = rays_through( test_bench.cnt + 1 ).r( valid_rays, : );
                    ch_points = pupil_positions( convhull( pupil_positions( :, 2:3 ), 'Simplify', true ), 2:3 );
                    %circle = TaubinSVD( ch_points );
                    %r = circle( 3 );
                    ell = MinVolEllipseFit( ch_points, 5e-4 );
                    r = min( ell( 3:4 ) );
                    radius_ratio_test = ( 2 * r / pupil_diameter );
                catch ME
                    radius_ratio_test = 0.0;
                end

                %{
                fprintf( 'diameter(%d): %f, target: %d, #valid: %d, radius: %f, ratio: %f\n', ...
                    batches_left, test_diameter, nrays, num_valid_rays_test, max( r ), radius_ratio_test );
                %}

                % keep doing this only if we didn't lose any rays
                can_keep_looping = num_valid_rays_test >= nrays * 0.8 && radius_ratio_test > radius_ratio;

                % update the diameter if we can keep looping
                if can_keep_looping
                    diameter = test_diameter;
                    radius_ratio = radius_ratio_test;
                    rays = rays_in.subset( valid_ray_ids );

                % jump to the next batch, if we have any
                elseif batches_left > 1
                    step_size = step_size * batch_mul;
                    batches_left = batches_left - 1;

                % exit otherwise
                else
                    keep_looping = false;
                end
            end
            
            %test_bench.elem{ test_bench.find_aperture_id( ) }.D( 2 ) = 1e1;
            %test_bench.draw( rays_through, 'lines', 0.33, 1.0, '3D', [ 0, 0 ] );
            
            % construct a bench with all the elements to make sure the
            % proper number of rays makes it out
            test_bench = self.create_capture_bench( trace_dir, self.cnt - 1 );

            % keep track of the ray count and number of valid rays
            num_valid_rays = 0;

            % keep increasing the grid size until the number of valid rays
            % reaches the desired amount
            %%{
            while num_valid_rays < nrays
                % construct the test rays
                rays_in = Rays( test_nrays, geometry, src, dir, diameter, pattern, material, lambda );

                % trace the rays through the test optical system
                rays_through = test_bench.trace( rays_in, 1, 1 );

                % extract the valid rays
                valid_rays = rays_through( test_bench.cnt + 1 ).I > 0;
                valid_ray_ids = find( valid_rays );
                num_valid_rays = length( valid_ray_ids );

                %{
                fprintf( 'rays: %d, valid rays: %d\n', ...
                    test_nrays, num_valid_rays );
                %}

                % increase the ray count if we are still below
                if num_valid_rays < nrays
                    test_nrays = ceil( test_nrays * ( nrays / num_valid_rays ) );

                % otherwise just stop looping
                else
                    rays = rays_in.subset( valid_ray_ids );
                end
            end
            %}
            
            %test_bench.elem{ test_bench.find_aperture_id( ) }.D( 2 ) = 1e1;
            %test_bench.draw( rays_through, 'lines', 0.33, 1.0, '3D', [ 0, 0 ] );

            % extend the diameter to all 4 sides
            diameter = diameter * ones( 4, 1 );
        end
 
        function [bfl, efl, coc, dv, dst_x] = back_focal_length( self, npasses, nrays, npos, lambda, dist_limits )
            if nargin < 2 || isempty( npasses ), npasses = 2; end
            if nargin < 3 || isempty( nrays ), nrays = 100; end
            if nargin < 4 || isempty( npos ), npos = 10; end
            if nargin < 5 || isempty( lambda ), lambda = 557.7; end
            if nargin < 6 || isempty( dist_limits ), dist_limits = [ 0.01, 40.0 ]; 
            elseif isscalar( dist_limits ), dist_limits = [ dist_limits dist_limits ]; end
            
            % get the necessary aperture parameters
            pupil_id = self.find_aperture_id( ); % find the index of the aperture element
            
            % make a temporary test copy of ourselves
            test_bench = self.copy( );
            test_bench.elem{ test_bench.cnt } = Screen( zeros( 1, 3 ), 1e6, 1e6, 100, 100 );
            
            % test ray bundle
            rays_in = self.fit_ray_grid_to_pupil( 'outside', 2, nrays, 'collimated', [ -10 0 0 ], [ 1 0 0 ], ...
                'hexcircle', 'air', lambda * 1e-9 );
            
            % distance along the optical axis for the aperture and 
            % the last element
            last_elem_x = self.elem{ self.cnt - 1 }.r( 1 );
            pupil_x = self.elem{ pupil_id }.r( 1 );
            
            % update the distance limits
            dist_limits( 1 ) = last_elem_x + dist_limits( 1 );
            dist_limits( 2 ) = last_elem_x + dist_limits( 2 );

            % trace through the test ray bundle
            rays_through = test_bench.trace( rays_in );
             
            for p = 1 : npasses
                % the target distances
                dst_x = linspace( dist_limits( 1 ), dist_limits( 2 ), npos );
                
                % loop over different screen distances
                dv = zeros( 1, npos );
                for i = 1 : npos
                    % move the capture screen
                    test_bench.elem{ test_bench.cnt }.r( 1 ) = dst_x( i );

                    % trace through the test rays and store the std. dev.
                    rays_last = rays_through( end - 1 ).interaction( test_bench.elem{ test_bench.cnt }, 1, 1, 1 );
                    [ ~, dv( i ) ] = rays_last( end ).stat;
                end
                
                % update the dist limits for the next pass
                [ coc, mi ] = min( dv );
                dist_limits( 1 ) = dst_x( max( mi - 1, 1 ) );
                dist_limits( 2 ) = dst_x( min( mi + 1, npos ) );
            end

            % find the result with the smallest circle of confusion
            bfl = abs( dst_x( mi ) - last_elem_x );
            efl = abs( dst_x( mi ) - pupil_x );
            
            %{
            % Display the optimal distance
            test_bench.elem{ test_bench.cnt }.r( 1 ) = dst_x( mi );
            rays_through = test_bench.trace( rays_in );
            test_bench.draw( rays_through, 'lines', .33, 1.0, '3D', [ 0, 0 ] );
            %}
        end
        
        function [ffl, efl, coc, dv, dst_x] = front_focal_length( self, npasses, nrays, npos, lambda, dist_limits )
            if nargin < 2 || isempty( npasses ), npasses = 2; end
            if nargin < 3 || isempty( nrays ), nrays = 100; end
            if nargin < 4 || isempty( npos ), npos = 10; end
            if nargin < 5 || isempty( lambda ), lambda = 557.7; end
            if nargin < 6 || isempty( dist_limits ), dist_limits = [ 0.01 * 1e3, 8.0 * 1e3 ]; 
            elseif isscalar( dist_limits ), dist_limits = [ dist_limits dist_limits ]; end
            
            % get the necessary aperture parameters
            pupil_id = self.find_aperture_id( ); % find the index of the aperture element
            
            % setup the test eye
            test_bench = self.copy( );
            test_bench.remove( test_bench.cnt );
            test_bench.reverse( );
            test_bench.cnt = test_bench.cnt + 1;
            test_bench.elem{ test_bench.cnt } = Screen( zeros( 1, 3 ), 1e6, 1e6, 10, 10 );
            test_bench.elem{ test_bench.cnt }.rotate( [ 0 0 1 ], deg2rad( 180 ) );
            
            % test ray bundle
            ray_material = self.elem{ self.cnt - 1 }.glass{ 2 };
            rays_in = test_bench.fit_ray_grid_to_pupil( 'inside', 3, nrays, 'collimated', [ 10 0 0 ], [ -1 0 0 ], 'hexcircle', ray_material, lambda * 1e-9 );
            
            % location of the anterior lens
            first_elem_x = self.elem{ 1 }.r( 1 );
            pupil_x = self.elem{ pupil_id }.r( 1 );
            
            % update the distance limits
            dist_limits( 1 ) = first_elem_x - dist_limits( 1 );
            dist_limits( 2 ) = first_elem_x - dist_limits( 2 );

            % trace through the test ray bundle
            rays_through = test_bench.trace( rays_in );
            
            for p = 1 : npasses
                % the target distances
                dst_x = linspace( dist_limits( 1 ), dist_limits( 2 ), npos );
            
                % loop over different screen distances
                dv = zeros( 1, npos );
                for i = 1 : npos
                    % move the capture screen
                    test_bench.elem{ test_bench.cnt }.r( 1 ) = dst_x( i );

                    % trace through the test rays and store the std. dev.
                    rays_last = rays_through( end - 1 ).interaction( test_bench.elem{ test_bench.cnt }, 1, 1, 1 );
                    [ ~, dv( i ) ] = rays_last( end ).stat;
                end
                %dv
                
                % update the dist limits for the next pass
                [ coc, mi ] = min( dv );
                dist_limits( 1 ) = dst_x( max( mi - 1, 1 ) );
                dist_limits( 2 ) = dst_x( min( mi + 1, npos ) );
            end
            
            % find the result with the smallest circle of confusion
            ffl = abs( dst_x( mi ) + first_elem_x );
            efl = abs( dst_x( mi ) + pupil_x );
            
            %{
            % Display the optimal distance
            test_bench.elem{ test_bench.cnt }.r( 1 ) = dst_x( mi );
            rays_through = test_bench.trace( rays_in );
            test_bench.draw( rays_through, 'lines', .33, 1.0, '3D', [ 0, 0 ] );
            %}
        end
    end
    
end