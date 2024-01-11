classdef AberrationDatabase < handle
    properties
        AberrationMap = containers.Map( );
        
        FileName = 'aberrations.mat'
    end
    
    methods
        function self = AberrationDatabase( )
            % find the database folder
            [ root_folder, ~, ~ ] = fileparts( mfilename( 'fullpath' ) );
            root_folder = sprintf( '%s/%s/', root_folder, 'databases' );
            
            % make sure it exists; create it otherwise
            if exist( root_folder, 'file' ) ~= 7
                mkdir( root_folder );
            end
            
            % append the root folder path to the file name
            self.FileName = strcat( root_folder, self.FileName );
            
            % load the aberration database, if it exists
            if exist( self.FileName, 'file' ) == 2
                load( self.FileName, 'aberration_db' );
                self.AberrationMap = aberration_db;
            end
        end

        function [ aberration_entry ] = Get( ...
                self, eye_entry, nrays, max_deg, angles_hor, angles_vert, ...
                lambdas, pupil_diams, focus_distances, varargin )

            % input parser
            p = inputParser;
            addOptional( p, 'ShutdownPool', true, @islogical );
            parse( p, varargin{ : } );
            params = p.Results;
            
            % resulting structure
            aberration_entry = struct( );
            
            % DB entry names
            aberration_name_params = [ ...
                angles_hor, angles_vert, lambdas, ...
                pupil_diams, focus_distances ];
            aberration_name_par_suffix = sprintf( '_%f', aberration_name_params );
            aberration_entry.entry_name = strcat( eye_entry.entry_name, aberration_name_par_suffix );

            % load the precomputed aberrations
            recompute_aberrations = eye_entry.recomputed;
            try
                aberration_entry = self.AberrationMap( aberration_entry.entry_name );
            catch
                recompute_aberrations = true;
            end

            % early out
            if recompute_aberrations == false
                return;
            end
            
            % number of parameters to trace
            num_lambdas = length( lambdas );
            num_angles_hor = length( angles_hor );
            num_angles_vert = length( angles_vert );
            num_pupil_diams = length( pupil_diams );
            num_focus_distances = length( focus_distances );
            num_coeffs = ZernikeNumCoeffs( max_deg );
            num_focus_indices = num_focus_distances * num_pupil_diams;
            num_aberr_indices = num_focus_distances * num_pupil_diams * num_lambdas * num_angles_hor * num_angles_vert;

            % calculate indices for the parallel loops
            pfindices = zeros( num_focus_indices, 2 );
            aberr_indices = zeros( num_aberr_indices, 6 );
            idx_pfi = 1;
            idx_ab = 1;
            for f = 1 : num_focus_distances
            for p = 1 : num_pupil_diams
                pfindices( idx_pfi, 1 ) = p;
                pfindices( idx_pfi, 2 ) = f;
                for l = 1 : num_lambdas
                for h = 1 : num_angles_hor
                for v = 1 : num_angles_vert
                    aberr_indices( idx_ab, 1 ) = p;
                    aberr_indices( idx_ab, 2 ) = f;
                    aberr_indices( idx_ab, 3 ) = l;
                    aberr_indices( idx_ab, 4 ) = h;
                    aberr_indices( idx_ab, 5 ) = v;
                    aberr_indices( idx_ab, 6 ) = idx_pfi;
                    idx_ab = idx_ab + 1;
                end,end,end
                idx_pfi = idx_pfi + 1;
            end, end

            % start the parallel pool
            parpool;

            % calculate the refocused eyes
            fprintf( 'Calculating refocused eyes: %d\n', num_focus_indices );

            refocused_eyes = cell( num_focus_indices, 1 );
            pupil_retina_distances_flat = zeros( num_focus_indices, 1 );
            focus_timer = tic;
            parfor pfi = 1 : num_focus_indices
                % extract the computation indices
                idx = num2cell( pfindices( pfi, : ) );
                [ p, f ] = deal( idx{:} );

                % focus distance
                fdist = 1.0 / focus_distances( f );

                % lambda at which to focus
                %refocus_lambda = mean( lambdas );
                refocus_lambda = 587.56;

                % refocus the eye
                test_eye = eye_entry.eye.copy( );
                test_eye.PupilD = pupil_diams( p );
                [ test_eye, ~, refocus_coc ] = eye_entry.eye.FocusAt( ...
                    fdist, refocus_lambda, 5, 100, 10, [ eye_entry.eye.LensD * 0.91, eye_entry.eye.LensD ] );

                % compute the pupil-retina distance
                pupil_retina_distance = test_eye.PupilRetinaDistance( );
    
                % write out the pupil-retina distance
                pupil_retina_distances_flat( pfi ) = pupil_retina_distance;

                % store the refocused eye
                refocused_eyes{ pfi, 1 } = test_eye;
            end
            total_focus_time = toc( focus_timer );
            fprintf( 'Total time spent on refocusing eyes: %f\n', total_focus_time );

            % Calculate the corresponding aberrations
            fprintf( 'Calculating aberrations: %d\n', num_aberr_indices );

            aberrations = zeros( num_aberr_indices, num_coeffs );
            aberration_timer = tic;
            parfor abi = 1 : num_aberr_indices
                % extract the computation indices
                idx = num2cell( aberr_indices( abi, : ) );
                [ p, f, l, h, v, pfi ] = deal( idx{:} );

                % extract the proper refocused eye structure
                test_eye = refocused_eyes{ pfi, 1 };

                % compute the aberrations
                test_eye.ComputeAberrationsParametric( ...
                    pupil_diams( p ), ...
                    angles_hor( h ), ...
                    angles_vert( v ), ...
                    lambdas( l ), ...
                    'MaxDegree', max_deg, ...
                    'NumRays', nrays, ...
                    'TraceVectors', 'chief', ...
                    'TraceVectorsEye', 'input', ...
                    'TraceVectorsRays', 100, ...
                    'TraceVectorsTol', 1e-6, ...
                    'IgnoreMissed', true, ...
                    'IgnoreBlocked', true, ...
                    'IgnoreTIR', true, ...
                    'GridShape', 'hexcircle', ...
                    'GridSpread', 'trace', ...
                    'GridFitPasses', 3, ...
                    'CaptureDistance', 1e-2, ...
                    'CaptureSize', 1e6, ...
                    'RadiusThreshold', 1.0, ...
                    'ProjectionMethod', 'parallel', ...
                    'CircumscribeRays', 'expected', ...
                    'CircumscribeShape', 'ellipse', ...
                    'CircumscribeExtension', 'mirror', ...
                    'EllipsePrecision', 2e-4, ...
                    'Centering', 'chief', ...
                    'Stretching', 'ellipse2circle', ...
                    'PupilRounding', 0.001, ...
                    'FitMethod', 'lsq' );

                % store it in the database
                aberrations_flat( abi, : ) = test_eye.Alpha;
            end
            total_aberration_time = toc(aberration_timer);

            fprintf( 'Aberration computation finished.\n' );
            fprintf( 'Total time spent on aberration calculations: %f\n', total_aberration_time );

            % output matrices            
            aberrations = zeros( num_angles_hor, num_angles_vert, num_lambdas, num_pupil_diams, num_focus_distances, num_coeffs );
            pupil_retina_distances = zeros( num_pupil_diams, num_focus_distances );

            idx_pfi = 1;
            idx_ab = 1;
            for f = 1 : num_focus_distances
            for p = 1 : num_pupil_diams
                pupil_retina_distances( p, f ) = pupil_retina_distances_flat( idx_pfi );
                for l = 1 : num_lambdas
                for h = 1 : num_angles_hor
                for v = 1 : num_angles_vert
                    aberrations( h, v, l, p, f, : ) = aberrations_flat( idx_ab, : );
                    idx_ab = idx_ab + 1;
                end,end,end
                idx_pfi = idx_pfi + 1;
            end, end

            % forcibly shut down the parallel pool
            if params.ShutdownPool
                poolobj = gcp( 'nocreate' );
                delete( poolobj );
            end
                
            % store the results in the output entry
            aberration_entry.opd = aberrations;
            aberration_entry.pupil_retina_distances = pupil_retina_distances;

            % store the computed aberrations in the database
            self.AberrationMap( aberration_entry.entry_name ) = aberration_entry;
                
            % store the databases
            aberration_db = self.AberrationMap;
            save( self.FileName, 'aberration_db' );
        end
    end
end