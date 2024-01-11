classdef EyeDatabase < handle
    properties
        EyeMap = containers.Map( );
        
        RootFolder = ''
        FileName = 'eyes.mat'
    end
    
    methods
        function self = EyeDatabase( )
            % find the database folder
            [ self.RootFolder, ~, ~ ] = fileparts( mfilename( 'fullpath' ) );
            self.RootFolder = sprintf( '%s/%s/', self.RootFolder, 'databases' );
            
            % make sure it exists; create it otherwise
            if exist( self.RootFolder, 'file' ) ~= 7
                mkdir( self.RootFolder );
            end
            
            % append the root folder path to the file name
            self.FileName = strcat( self.RootFolder, self.FileName );
            
            % load the eye database, if it exists
            if exist( self.FileName, 'file' ) == 2
                load( self.FileName, 'eye_db' );
                self.EyeMap = eye_db;
            end
        end

        function ShowEyeAberrations( self, reconstruction, eye_entry )
            % show the aberrations
            eye_entry.eye.PrintAberrations( );

            % show the mean absolute error
            target_coeffs = reconstruction.optimization_params.alpha_padded( 2:end );
            actual_coeffs = eye_entry.eye.Aberrations.Opd( 2:end );
            errors = target_coeffs - actual_coeffs;
            aberrations_combined = [ target_coeffs, actual_coeffs, errors ];
            descriptions_combined = [ 'Target', 'Actual', 'Error' ];
            print_aberrations( aberrations_combined, descriptions_combined, 18 );

            % calculate the errors and show them
            reconstruction_mae = mean( abs( errors ) );
            fprintf( 'Reconstruction error (MAE): %f\n', reconstruction_mae );
        end

        function [ eye_entry ] = GetReconstructed( ...
                self, name, pd, ml, nrays, max_deg, max_time, ...
                anat_boundary, anat_average, functional_specified, functional_unspecified, ...
                alpha, optimizer, solver )
            
            % resulting structure
            eye_entry = struct( );
            
            % construct the unique eye ID
            if isempty( name )
                eye_name_params_number = [ ...
                    pd, ml, nrays, max_deg, max_time, ...
                    anat_boundary, anat_average, ...
                    functional_specified, functional_unspecified, ...
                    reshape( alpha, 1, [] ) ];
                eye_name_params_str = [ optimizer, solver ];
                eye_name_par_suffix = strcat( ...
                    sprintf( '_%f', eye_name_params_number ), ...
                    sprintf( '_%s', eye_name_params_str ) );
                eye_entry.entry_name = strcat( 'eye', eye_name_par_suffix );
            else
                eye_entry.entry_name = name;
            end

            % eye reconstruction object
            reconstruction = EyeReconstruction( ...
                pd, ml, nrays, max_deg, alpha, max_time, ...
                anat_boundary, anat_average, ...
                functional_specified, functional_unspecified, ...
                optimizer, solver );

            % try to load it
            full_path = sprintf( '%s/%s.mat', self.RootFolder, name );
            if isempty( name )
                try
                    eye_entry.eye = self.EyeMap( eye_entry.entry_name );
                    eye_entry.recomputed = false;
                catch
                    eye_entry.recomputed = true;
                end
            else
                % try to load it
                try
                    load( full_path, 'eye' );
                    eye_entry.eye = eye;
                    eye_entry.recomputed = false;
                catch
                    eye_entry.recomputed = true;
                end
            end
            
            % recompute, if necessary
            if eye_entry.recomputed
                fprintf( 'Reconstructing eye...\n' );
            
                % reconstruct the eye
                tic;
                reconstruction.Reconstruct( );
                eye_entry.eye = reconstruction.eye;
                reconstruction_time = toc;
                
                fprintf( 'Reconstruction finished in %f seconds\n', reconstruction_time );
                
                % Print out the eye losses
                reconstruction.PrintLossesFromEye( eye_entry.eye );
                
                % store it in the database
                self.EyeMap( eye_entry.entry_name ) = eye_entry.eye;
    
                % update the eye database
                if isempty( name )
                    eye_db = self.EyeMap;
                    save( self.FileName, 'eye_db' );
                else
                    eye = eye_entry.eye;
                    save( full_path, 'eye' )
                end
            end

            % Display the resulting eye
            % disp( eye_entry.eye );
            
            % Print out the eye losses
            reconstruction.PrintLossesFromEye( eye_entry.eye );

            % Also show the aberrations
            fprintf( 'Eye aberrations using reconstruction estimator:\n' );

            eye_entry.eye.ComputeAberrationsParametric( ...
                reconstruction.optimization_params.pupil_diameter, ...
                deg2rad( 0.0 ), deg2rad( 0.0 ), ...
                reconstruction.optimization_params.measurement_lambda, ...
                reconstruction.compute_aberrations_params{:} );
            self.ShowEyeAberrations( reconstruction, eye_entry );

            fprintf( 'Eye aberrations using proper estimation:\n' );
            eye_entry.eye.ComputeAberrationsParametric( ...
                reconstruction.optimization_params.pupil_diameter, ...
                deg2rad( 0.0 ), deg2rad( 0.0 ), ...
                reconstruction.optimization_params.measurement_lambda, ...
                'MaxDegree', max_deg, ...
                'NumRays', nrays, ...        
                'TraceVectors', 'chief',  ...
                'TraceVectorsEye', 'input',  ...
                'TraceVectorsRays', 50, ...
                'TraceVectorsTol', 1e-6, ...
                'IgnoreMissed', true,  ...
                'IgnoreBlocked', true,  ...
                'IgnoreTIR', true, ...
                'GridShape', 'hexcircle', ...
                'GridSpread', 'trace',  ...
                'GridFitPasses', 3, ...
                'CaptureDistance', 1e-1, ...
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
            self.ShowEyeAberrations( reconstruction, eye_entry );

            %{
            [ bfl, efl, bfl_coc, bfl_dv, bfl_x ] = eye_entry.eye.BackFocalLength( 4, 50, 10, [ 0.01, 40.0 ], false );
            %[ bfl_x; bfl_dv ]'
            fprintf( 'Back focal length: %f mm (CoC: %f)\n', bfl, bfl_coc );
            fprintf( 'Effective focal length: %f mm (CoC: %f)\n', efl, bfl_coc );
            
            retinal_pos = eye_entry.eye.ComputeCenterRayRetinalPos( );
            [ ffl, ffl_coc, ffl_dv, ffl_x ] = eye_entry.eye.FrontFocusPoint( 4, 50, 10, retinal_pos( 1 ) * 1e-3, [ 0.01, 8.0 * 1e3 ], false );
            %[ ffl_x; ffl_dv ]'
            fprintf( 'Front focus point: %f mm (CoC: %f)\n', ffl, ffl_coc );
            %}
        end

        function [ eye_entry ] = GetRefocused( self, eye_entry, focus_distance )
            
        end
    end
end