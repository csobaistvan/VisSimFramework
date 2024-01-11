 classdef EyeReconstruction < handle
    properties
        % optimization parameters
        optimization_params
        compute_aberrations_params
        
        % variables to optimize
        variables_anatomical = []
        num_variables_anatomical = 0
        
        % variables to echo (show merely for debugging)
        variables_echo = []
        num_variables_echo = 0
        
        % functional targets
        variables_functional = []
        num_variables_functional = 0
        
        % initial and optimized eye parameters
        initial_params
        optimized_params
        lower_bounds
        upper_bounds
        param_extents
        
        % same, but scaled (0..1) versions
        initial_params_scaled
        optimized_params_scaled
        lower_bounds_scaled
        upper_bounds_scaled
        
        % optimization state
        is_optimizing
        is_optimized
        
        % optimized eye
        eye
    end
    methods(Static)
        function stop = SurrogateOptPlot( X, optimValues, state )
            stop = false;
            if ~strcmp( state, 'iter' ) || optimValues.fval < 1e8
                stop = surrogateoptplot( X, optimValues, state );
            end
        end
    end
    
    methods
        function self = EyeReconstruction( pd, ml, nrays, max_deg, alpha, max_time, anat_boundary, anat_average, ...
                func_weight_spec, func_weight_unspec, optimizer, solver )
            
            if nargin <= 6, anat_boundary = 1.0; end
            if nargin <= 7, anat_average = 1.0; end
            if nargin <= 8, func_weight_spec = 10.0; end
            if nargin <= 9, func_weight_unspec = 50.0; end
            if nargin <= 10, optimizer = 'multistart'; end
            if nargin <= 11, solver = 'fmincon-sqp'; end
            
            if isstruct( pd )
                % store the optimization parameters
                self.optimization_params = pd;
            else
                self.optimization_params = struct;
                self.optimization_params.pupil_diameter = pd;
                self.optimization_params.measurement_lambda = ml;
                self.optimization_params.nrays = nrays;
                self.optimization_params.max_time = max_time * 60;
                self.optimization_params.anatomical_weight_average = anat_average;
                self.optimization_params.anatomical_weight_boundary = anat_boundary;
                self.optimization_params.functional_weight_specified = func_weight_spec;
                self.optimization_params.functional_weight_unspecified = func_weight_unspec;
                self.optimization_params.functional_weight_not_same_sign = 10.0;
                self.optimization_params.tolerance_scale = max([ anat_average, anat_boundary, func_weight_spec, func_weight_unspec ]).^2;
                self.optimization_params.loss_method = 'quadratic'; % linear, quadratic, L2
                self.optimization_params.loss_reduction = 'total'; % total, average;
                self.optimization_params.num_phases = 1;
                self.optimization_params.trace = true;
                self.optimization_params.plot = true;
                self.optimization_params.optimizer = optimizer;
                self.optimization_params.solver = solver;
                self.optimization_params.parallel = true;
                self.optimization_params.max_deg = max_deg;
                self.optimization_params.alpha = alpha;
                if size(self.optimization_params.alpha, 1 ) == 2 && size(self.optimization_params.alpha, 2 ) ~= 2
                    self.optimization_params.alpha = self.optimization_params.alpha.';
                end
                
                % aberration computaton parameters
                self.compute_aberrations_params = struct;
                self.compute_aberrations_params.NumRays = self.optimization_params.nrays;
                self.compute_aberrations_params.MaxDegree = self.optimization_params.max_deg;
                %self.compute_aberrations_params.TraceVectors = 'chief';
                self.compute_aberrations_params.TraceVectors = 'simple';
                self.compute_aberrations_params.TraceVectorsEye = 'input';
                self.compute_aberrations_params.TraceVectorsRays = 100;
                self.compute_aberrations_params.TraceVectorsTol = 1e-6;
                self.compute_aberrations_params.IgnoreMissed = true;
                self.compute_aberrations_params.IgnoreBlocked = true;
                self.compute_aberrations_params.IgnoreTIR = true;
                self.compute_aberrations_params.GridShape = 'hexcircle';
                %self.compute_aberrations_params.GridSpread = 'trace';
                self.compute_aberrations_params.GridSpread = 'approximate';
                self.compute_aberrations_params.GridFitPasses = 3;
                self.compute_aberrations_params.CaptureDistance = 1e-2;
                self.compute_aberrations_params.CaptureSize = 1e6;
                self.compute_aberrations_params.RadiusThreshold = 1.0;
                self.compute_aberrations_params.ProjectionMethod = 'parallel';
                self.compute_aberrations_params.CircumscribeRays = 'expected';
                self.compute_aberrations_params.CircumscribeShape = 'ellipse';
                self.compute_aberrations_params.CircumscribeExtension = 'mirror';
                self.compute_aberrations_params.EllipsePrecision = 2e-4;
                self.compute_aberrations_params.Centering = 'chief';
                self.compute_aberrations_params.Stretching = 'ellipse2circle';
                self.compute_aberrations_params.PupilRounding = 0.001;
                self.compute_aberrations_params.FitMethod = 'lsq';
                self.compute_aberrations_params = struct2pairs( self.compute_aberrations_params );
            end
            
            % construct the padded coefficient array
            self.optimization_params.alpha_padded = zeros( ZernikeNumCoeffs( self.optimization_params.max_deg ), 1 );
            for i = 1 : size( self.optimization_params.alpha, 1 )
                self.optimization_params.alpha_padded( self.optimization_params.alpha( i, 1 ) ) = self.optimization_params.alpha( i , 2 );
            end
            
            % list of all the variables
            [ self.variables_anatomical, self.variables_echo ] = self.VariablesAnatomical( );
            if self.optimization_params.num_phases == 1
                combined_vars = cell( 1, 1 );
                combined_vars{ 1 } = { };
                for p = 1 : length( self.variables_anatomical )
                    combined_vars{ 1 } = [ combined_vars{ 1 }, self.variables_anatomical{ p } ];
                end
                self.variables_anatomical = combined_vars;
                self.num_variables_anatomical = { length( self.variables_anatomical{ 1 } ) };
            else
                self.num_variables_anatomical = { length( self.variables_anatomical{ 1 } ), length( self.variables_anatomical{ 2 } ) };
            end
            self.num_variables_echo = length( self.variables_echo );
            % list of target coefficients
            self.variables_functional = self.VariablesFunctional( );
            self.num_variables_functional = length( self.variables_functional );
            
            % initialize the internal state
            self.is_optimizing = false;
            self.is_optimized = false; 
            % construct the initial parameters
            
            for p = 1 : self.optimization_params.num_phases
                self.initial_params{ p } = zeros( 1, self.num_variables_anatomical{ p } );
                for i = 1 : self.num_variables_anatomical{ p }
                    var = self.variables_anatomical{ p }{ i };
                    self.initial_params{ p }( i ) = var.mean;
                end
                % construct the lower and upper bounds
                self.lower_bounds{ p } = zeros( 1, self.num_variables_anatomical{ p } );
                self.upper_bounds{ p } = zeros( 1, self.num_variables_anatomical{ p } );
                for i = 1 : self.num_variables_anatomical{ p }
                    self.lower_bounds{ p }( i ) = max( self.variables_anatomical{ p }{ i }.mean - self.variables_anatomical{ p }{ i }.md, self.variables_anatomical{ p }{ i }.min );
                    self.upper_bounds{ p }( i ) = min( self.variables_anatomical{ p }{ i }.mean + self.variables_anatomical{ p }{ i }.md, self.variables_anatomical{ p }{ i }.max );
                end
                self.param_extents{ p } = self.upper_bounds{ p } - self.lower_bounds{ p };
                % scaled parameters
                self.lower_bounds_scaled{ p } = zeros( size( self.lower_bounds{ p } ) );
                self.upper_bounds_scaled{ p } = ones( size( self.lower_bounds{ p } ) );
                self.initial_params_scaled { p }= self.ScaleEyeParameters( p, self.initial_params{ p } );
            end
        end

        function Reconstruct( self )
            fprintf( 'Reconstructing eye; Parameters:\n' );
            fprintf( '%20s: %f\n', 'Number of rays:', self.optimization_params.nrays );
            fprintf( '%20s: %f\n', 'Pd', self.optimization_params.pupil_diameter );
            fprintf( '%20s: %f\n', 'Lambda', self.optimization_params.measurement_lambda );
            fprintf( '%20s: %s\n', 'Optimizer', self.optimization_params.optimizer );
            fprintf( '%20s: %s\n', 'Solver', self.optimization_params.solver );
            fprintf( '%20s: %f (%f)\n', 'Anatomical weight:', self.optimization_params.anatomical_weight_average, self.optimization_params.anatomical_weight_boundary );
            fprintf( '%20s: %f (%f)\n', 'Functional weight:', self.optimization_params.functional_weight_unspecified, self.optimization_params.functional_weight_unspecified );
            fprintf( '%20s: %d\n', 'Number of targets', size( self.optimization_params.alpha, 1 ) );
            for i = 1 : size( self.optimization_params.alpha, 1 )
                [ n, m ] = ZernikeIndices( self.optimization_params.alpha( i, 1 ) );
                fprintf( '%Z[%3d, %3d]: %f\n', n, m, self.optimization_params.alpha( i, 2 ) );
            end
            tic;
            % run the optimizer
            self.RunOptimizer( );
            % construct the resulting eye object
            self.eye = EyeParametric();
            self.eye.PupilD = self.optimization_params.pupil_diameter;
            for p = 1 : self.optimization_params.num_phases
                self.CopyEyeParameters( self.eye, p, self.optimized_params{ p } );
            end
            self.eye.MakeElements( );
            % mark that it has been successfully optimized
            self.is_optimized = true;
            toc;
        end
        
        function RunOptimizer( self )
            % mark that optimization has started
            self.is_optimizing = true;
            
            for p = 1 : self.optimization_params.num_phases
                switch self.optimization_params.solver
                    case 'GPS_2N'
                        options = optimoptions( 'patternsearch', 'PollMethod', 'GPSPositiveBasis2N' );
                    case 'GPS_NP1'
                        options = optimoptions( 'patternsearch', 'PollMethod', 'GPSPositiveBasisNp1' );
                    case 'GSS_2N'
                        options = optimoptions( 'patternsearch', 'PollMethod', 'GSSPositiveBasis2N' );
                    case 'GSS_NP1'
                        options = optimoptions( 'patternsearch', 'PollMethod', 'GSSPositiveBasisNp1' );
                    case 'MADS_2N'
                        options = optimoptions( 'patternsearch', 'PollMethod', 'MADSPositiveBasis2N' );
                    case 'MADS_NP1'
                        options = optimoptions( 'patternsearch', 'PollMethod', 'MADSPositiveBasisNp1' );
                    case 'GPS_2N_MADS_2N'
                        options = optimoptions( 'patternsearch', 'PollMethod', 'GPSPositiveBasis2N' );
                        options = optimoptions( options, 'SearchFcn', 'MADSPositiveBasis2N' );
                    case 'GPS_NP1_MADS_2N'
                        options = optimoptions( 'patternsearch', 'PollMethod', 'GPSPositiveBasisNp1' );
                        options = optimoptions( options, 'SearchFcn', 'MADSPositiveBasis2N' );
                    case 'GSS_2N_MADS_2N'
                        options = optimoptions( 'patternsearch', 'PollMethod', 'GSSPositiveBasis2N' );
                        options = optimoptions( options, 'SearchFcn', 'MADSPositiveBasis2N' );
                    case 'GSS_NP1_MADS_2N'
                        options = optimoptions( 'patternsearch', 'PollMethod', 'GSSPositiveBasisNp1' );
                        options = optimoptions( options, 'SearchFcn', 'MADSPositiveBasis2N' );
                    case 'MADS_2N_GPS_2N'
                        options = optimoptions( 'patternsearch', 'PollMethod', 'MADSPositiveBasis2N' );
                        options = optimoptions( options, 'SearchFcn', 'GSSPositiveBasis2N' );
                    case 'MADS_NP1_GPS_2N'
                        options = optimoptions( 'patternsearch', 'PollMethod', 'MADSPositiveBasisNp1' );
                        options = optimoptions( options, 'SearchFcn', 'GSSPositiveBasis2N' );
                end

                if strcmp( self.optimization_params.solver, 'MADS_2N' ) || strcmp( self.optimization_params.solver, 'MADS_NP1' )
                    options = optimoptions( options, 'ScaleMesh', true );
                else
                    options = optimoptions( options, 'AccelerateMesh', false );
                    options = optimoptions( options, 'InitialMeshSize', 0.005  );
                    options = optimoptions( options, 'MaxMeshSize', 0.005 );
                    options = optimoptions( options, 'MeshContractionFactor', 0.5 );
                    options = optimoptions( options, 'MeshExpansionFactor', 2.0 );
                    options = optimoptions( options, 'ScaleMesh', 'off' );
                end
                if p == 1 && self.optimization_params.num_phases > 1
                    options = optimoptions( options, 'MaxFunEvals', 500000 * self.num_variables_anatomical{ p } );
                    options = optimoptions( options, 'MaxIter',  30 * self.num_variables_anatomical{ p } );
                else
                    options = optimoptions( options, 'MaxFunEvals', 5000000 * self.num_variables_anatomical{ p } );
                    options = optimoptions( options, 'MaxIter',  50000 * self.num_variables_anatomical{ p } );
                    options = optimoptions( options, 'MaxTime', self.optimization_params.max_time );
                end
                %options = optimoptions( options, 'SearchFcn', 'searchlhs' );
                options = optimoptions( options, 'UseCompletePoll', true );
                options = optimoptions( options, 'Cache', 'on' );
                options = optimoptions( options, 'CacheTol', eps );
                options = optimoptions( options, 'CacheSize', 1e6 );
                options = optimoptions( options, 'StepTolerance', 5e-4 );
                options = optimoptions( options, 'MeshTolerance', 5e-4 );
                options = optimoptions( options, 'FunctionTolerance', 1e-7 * self.optimization_params.tolerance_scale );
                options = optimoptions( options, 'UseParallel', self.optimization_params.parallel );
                options = optimoptions( options, 'PlotFcn', { @psplotbestf, @psplotmeshsize } );
                options = optimoptions( options, 'PlotInterval', 1 );
                options = optimoptions( options, 'Display', 'iter' );

                % objective function
                objective = @( x ) self.LossScaled( p, x );
       
                % run the optimizer
                [ x, ~ ] = patternsearch( objective, self.initial_params_scaled{ p }, [], [], [], [], ...
                    self.lower_bounds_scaled{ p }, self.upper_bounds_scaled{ p }, [], options );
                self.optimized_params_scaled{ p } = x;
                self.optimized_params{ p } = self.UnscaleEyeParameters( p, self.optimized_params_scaled{ p } );
            end
        end
                
        function scaled_parameters = ScaleEyeParameters( self, phase, eye_params )
            scaled_parameters = ( eye_params - self.lower_bounds{ phase } ) ./ self.param_extents{ phase };
        end
        
        function original_parameters = UnscaleEyeParameters( self, phase, eye_params )
            original_parameters = self.lower_bounds{ phase } + eye_params .* self.param_extents{ phase };
        end
        
        function CopyEyeParameters( self, eye, phase, eye_params )
            for p = 1 : phase - 1
                for i = 1 : self.num_variables_anatomical{ p }
                    var = self.variables_anatomical{ p }{ i };
                    eye.( var.name ) = self.optimized_params{ p }( i );
                end
            end
            for i = 1 : self.num_variables_anatomical{ phase }
                var = self.variables_anatomical{ phase }{ i };
                eye.( var.name ) = eye_params( i );
            end
            for p = phase + 1 : self.optimization_params.num_phases
                for i = 1 : self.num_variables_anatomical{ p }
                    var = self.variables_anatomical{ p }{ i };
                    eye.( var.name ) = self.initial_params{ p }( i );
                end
            end
            if self.optimization_params.measurement_lambda > 0
                eye.Lambda = self.optimization_params.measurement_lambda;
            end
            if self.optimization_params.pupil_diameter > 0
                eye.PupilD = self.optimization_params.pupil_diameter;
            end
        end
        
        function eye_params = ExtractEyeParameters( self, phase, eye )
            eye_params_anatomical = zeros( 1, size( self.variables_anatomical{ phase }, 1 ) );
            for i = 1 : length( self.variables_anatomical{ phase } )
                var = self.variables_anatomical{ phase }{ i };
                eye_params_anatomical( i ) = eye.( var.name );
            end
            
            eye_params_echo = zeros( 1, size( self.variables_echo, 1 ) );
            for i = 1 : length( self.variables_echo )
                var = self.variables_echo{ i };
                eye_params_echo( i ) = eye.( var.name );
            end
            
            eye_params = [ eye_params_anatomical, eye_params_echo ];
        end

        function [ total_loss, losses ] = LossesEye( self, eye, phase, loss_method, full_info )
            eye_valid = true;
            try
                % make a local copy
                test_eye = eye.copy( );
                
                % compute the aberrations
                test_eye.ComputeAberrations( self.compute_aberrations_params{:} );
            catch
                eye_valid = false;
            end
            
            % detailed loss info header
            headers = { 'Source' 'Loss', 'Current', 'Average', 'Std. Dev.', 'Difference', 'Max. Diff.', 'Norm. Val.', 'Weight' };
            
            % make sure the eye is valid
            if not( eye_valid ) && not( full_info )
                total_loss = 1e10;
                if full_info
                    losses = cell( 2, length( headers ) );
                    losses( 1, : ) = headers;
                    losses( 2, : ) = { 'Singular' 1e6, 0, 0, 0, 0, 0, 0, 0 };
                end
                return;
            end
            
            if phase > 0
                % extract the actual eye parameters
                eye_params = self.ExtractEyeParameters( phase, eye );

                % compute the anatomical loss parameters
                [ total_losses_anatomical, losses_anatomical, ~, losses_echo ] = self.LossesAnatomical( phase, eye_params, loss_method, full_info );
                total_loss = total_losses_anatomical;
                num_vars_total = size( losses_anatomical, 1 ) + size( losses_echo, 1 ) + 3;
            else
                total_loss = 0;
                num_vars_total = 0;
                losses_anatomical = {};
                
                for p = 1 : self.optimization_params.num_phases
                    % extract the actual eye parameters
                    eye_params = self.ExtractEyeParameters( p, eye );

                    % compute the anatomical loss parameters
                    [ loss, losses, ~, losses_echo ] = self.LossesAnatomical( p, eye_params, loss_method, full_info );
                    total_loss = total_loss + loss;
                    if p == 1
                        num_vars_total = num_vars_total + size( losses_echo, 1 ) + 3;
                        losses_anatomical = losses;
                    else
                        losses_anatomical = [ losses_anatomical; losses ];
                    end
                    num_vars_total = num_vars_total + size( losses, 1 );
                end
            end
            
            % compute the functional loss parameters
            if eye_valid
                [ total_losses_functional, losses_functional ] = self.LossesFunctional( test_eye, loss_method, full_info );
            
                total_loss = total_loss + total_losses_functional;
                num_vars_total = num_vars_total + size ( losses_functional, 1 ) + 1;
            end
            
            % skip the rest if we don't need it
            if not( full_info )
                if strcmp( loss_method, 'L2' )
                    total_loss = sqrt( total_loss );
                end                
                return; 
            end
            
            % construct the resulting loss structure
            separators = { '----' '----', '----', '----', '----', '----', '----', '----', '----' };
            losses = cell( num_vars_total, length( headers ) );
            
            losses( 1, : ) = headers;
            loss_id = 2;
            
            % append the losses
            for i = 1 : size( losses_anatomical, 1 )
                losses( loss_id, : ) = losses_anatomical( i, : );
                loss_id = loss_id + 1;
            end
            
            losses( loss_id, : ) = separators;
            loss_id = loss_id + 1;
            
            for i = 1 : size( losses_echo, 1 )
                losses( loss_id, : ) = losses_echo( i, : );
                loss_id = loss_id + 1;
            end
            
            if eye_valid
                losses( loss_id, : ) = separators;
                loss_id = loss_id + 1;

                % functional targets
                for i = 1 : size( losses_functional, 1 )
                    losses( loss_id, : ) = losses_functional( i, : );
                    loss_id = loss_id + 1;
                end
            end
            if strcmp( loss_method, 'L2' )
                total_loss = sqrt( total_loss );
            end
            if not( eye_valid )
                total_loss = total_loss + 1e10;
            end
        end
            
        function [ total_loss, losses ] = LossesParams( self, phase, eye_params, full_info )
            % construct the eye
            test_eye = EyeParametric( );
            self.CopyEyeParameters( test_eye, phase, eye_params );
            test_eye.MakeElements( );
                
            % compute the loss
            if ~full_info
                total_loss = self.LossesEye( test_eye, phase, self.optimization_params.loss_method, full_info );
            else
                [ total_loss, losses ] = self.LossesEye( test_eye, phase, self.optimization_params.loss_method, full_info );
            end
        end
        
        function loss = LossScaled( self, phase, scaled_eye_params )
            eye_params = self.UnscaleEyeParameters( phase, scaled_eye_params );
            loss = self.LossesParams( phase, eye_params, false );
        end
        
        function PrintLossStructure( self, total_loss, losses )
        end
        
        function PrintLossesFromEye( self, eye )
            [ total_loss, losses ] = self.LossesEye( eye, 0, self.optimization_params.loss_method, true );
            [ total_loss_linear, ~ ] = self.LossesEye( eye, 0, 'linear', true );
            
            fprintf( 'Anatomical weight: %f (boundary), %f (average)\n', ...
                self.optimization_params.anatomical_weight_boundary, ...
                self.optimization_params.anatomical_weight_average );
            fprintf( 'Functional weight: %f (specified), %f (unspecified)\n', ...
                self.optimization_params.functional_weight_specified, ...
                self.optimization_params.functional_weight_unspecified );
            fprintf( 'Total loss: %f (linear: %f)\n', total_loss, total_loss_linear );
            fprintf( 'Individual losses: \n' );
            for i = 1 : size( losses, 2 )
                fprintf( '%10s\t', losses{ 1, i } );
            end
            fprintf( '\n' );
            for i = 1 : size( losses, 2 )
                underline = '---------------------------------------------';
                fprintf( '%10s\t', underline( 1 : strlength( losses{ 1, i } ) ) );
            end
            fprintf( '\n' );
            for i = 2 : size( losses, 1 )
                if strcmp( losses{ i, 1 }, '----' )
                    for j = 1 : size( losses, 2 )
                        underline = '---------------------------------------------';
                        fprintf( '%10s\t', underline( 1 : strlength( losses{ 1, j } ) ) );
                    end
                    fprintf( '\n' );
                else
                    fprintf( '%10s', losses{ i, 1 } );
                    for j = 2 : size( losses, 2 )
                        fprintf( '\t%10f', losses{ i, j } );
                    end
                    fprintf( '\n' );
                end
            end
        end

        function [ total_loss_anatomical, losses_anatomical, total_loss_echo, losses_echo ] = LossesAnatomical( self, phase, eye_params, loss_method, full_info )
            if full_info, losses_anatomical = cell( self.num_variables_anatomical{ phase }, 8 );
            else        , losses_anatomical = cell( 0, 8 ); end
            
            total_loss_anatomical = 0;
            for i = 1 : self.num_variables_anatomical{ phase }
                var = self.variables_anatomical{ phase }{ i };
                var_value = eye_params( i );
                var_loss = 0;
                var_weight = var.weight;
                if strcmp( var.wmethod, 'boundary' )
                    var_weight = var_weight * self.optimization_params.anatomical_weight_boundary;
                    var_loss = var_weight * max( abs( var.mean - var_value ) - var.sd, 0 );
                elseif strcmp( var.wmethod, 'average' )
                    var_weight = var_weight * self.optimization_params.anatomical_weight_average;
                    var_loss = var_weight * abs( var.mean - var_value );
                end
                switch loss_method
                    case 'linear'
                        total_loss_anatomical = total_loss_anatomical + var_loss;
                    case 'quadratic'
                        total_loss_anatomical = total_loss_anatomical + var_loss .^ 2;
                    case 'L2'
                        total_loss_anatomical = total_loss_anatomical + var_loss .^ 2;
                end
                if full_info
                    losses_anatomical{ i, 1 } = var.name;
                    losses_anatomical{ i, 2 } = var_loss;
                    losses_anatomical{ i, 3 } = var_value;
                    losses_anatomical{ i, 4 } = var.mean;
                    losses_anatomical{ i, 5 } = var.sd;
                    losses_anatomical{ i, 6 } = losses_anatomical{ i, 3 } - losses_anatomical{ i, 4 };
                    losses_anatomical{ i, 7 } = var.md;
                    losses_anatomical{ i, 8 } = ( var_value - self.lower_bounds{ phase }( i ) ) / self.param_extents{ phase }( i );
                    losses_anatomical{ i, 9 } = var_weight;
                end
            end
            
            if full_info, losses_echo = cell( self.num_variables_echo, 8 );
            else        , losses_echo = cell( 0, 8 ); end
            
            if length( eye_params ) == self.num_variables_anatomical{ phase }
                total_loss_echo = 0;
                return;
            end

            total_loss_echo = 0;
            for i = 1 : self.num_variables_echo
                var = self.variables_echo{ i };
                var_value = eye_params( self.num_variables_anatomical{ phase } + i );
                var_loss = 0;
                var_weight = var.weight;
                if strcmp( var.wmethod, 'boundary' )
                    var_weight = var_weight * self.optimization_params.anatomical_weight_boundary;
                    var_loss = var_weight * max( abs( var.mean - var_value ) - var.sd, 0 );
                elseif strcmp( var.wmethod, 'average' )
                    var_weight = var_weight * self.optimization_params.anatomical_weight_average;
                    var_loss = var_weight * abs( var.mean - var_value );
                end
                
                switch loss_method
                    case 'linear'
                        total_loss_echo = total_loss_echo + var_loss;
                    case 'quadratic'
                        total_loss_echo = total_loss_echo + var_loss .^ 2;
                    case 'L2'
                        total_loss_echo = total_loss_echo + var_loss .^ 2;
                end
                
                if full_info
                    losses_echo{ i, 1 } = var.name;
                    losses_echo{ i, 2 } = var_loss;
                    losses_echo{ i, 3 } = var_value;
                    losses_echo{ i, 4 } = var.mean;
                    losses_echo{ i, 5 } = var.sd;
                    losses_echo{ i, 6 } = losses_echo{ i, 3 } - losses_echo{ i, 4 };
                    losses_echo{ i, 7 } = var.md;
                    losses_echo{ i, 8 } = 0;
                    losses_echo{ i, 9 } = var_weight;
                end
            end
        end
        
        function [ total_loss, losses ] = LossesFunctional( self, test_eye, loss_method, full_info )
            if full_info, losses = cell( self.num_variables_functional, 8 );
            else        , losses = cell( 0, 8 ); end
                
            total_loss = 0;
            for i = 1 : self.num_variables_functional
               [ n, m ] = ZernikeIndices( i );
               alpha = test_eye.Alpha;
               
               current = alpha( i );
               target = self.optimization_params.alpha_padded( i );
               weight = self.variables_functional( i );
               if ( current < -1e-5 && target > 1e-5 ) || ( current > 1e-5 && target < -1e-5 )
                   weight = weight * self.optimization_params.functional_weight_not_same_sign;
               end
               coeff_loss = weight * abs( current - target );
               
               switch loss_method
                   case 'linear'
                       total_loss = total_loss + coeff_loss;
                   case 'quadratic'
                       total_loss = total_loss + coeff_loss .^ 2;
                   case 'L2'
                       total_loss = total_loss + coeff_loss .^ 2;
               end
               
               if full_info
                   losses{ i, 1 } = sprintf( 'Z[%2d, %2d]', n, m );
                   losses{ i, 2 } = coeff_loss;
                   losses{ i, 3 } = alpha( i );
                   losses{ i, 4 } = self.optimization_params.alpha_padded( i );
                   losses{ i, 5 } = 0;
                   losses{ i, 6 } = losses{ i, 3 } - losses{ i, 4 };
                   losses{ i, 7 } = 0;
                   losses{ i, 8 } = 0;
                   losses{ i, 9 } = self.variables_functional( i );
               end
            end           
        end
  
        function var = MakeVariableAnatomical( self, name, mean, sd, md, weight, wmethod, min, max )
            var = struct;
            var.name = name;
            var.mean = mean;
            var.sd = sd;
            var.md = md;
            var.weight = weight;
            var.wmethod = wmethod;
            var.min = min;
            var.max = max;
        end
        
        function [ avars, evars ] = VariablesAnatomical( self )
            % optimization variables
            avars = { { }, {} };
            evars = { };
            
            % cornea Zernike coeff names
            cznames = cell( 1, ZernikeNumCoeffs( 6 ) );
            for i = 1 : ZernikeNumCoeffs( 6 )
                cznames{ i } = sprintf( 'Cornea1Z%d', i );
            end
            
            %                                                     name        mean     sd       mdiff   weight    loss fn.        min       max
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'EyeT',      23.82,   0.81,     2.00,     2.0,  'boundary',      0.0,  realmax );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'CorneaT',    0.55,   0.03,     0.16,    32.0,  'boundary',      0.39,    0.71 );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Cornea1R1',  7.81,   0.25,     2.00,     1.0,  'boundary',      6.5,  realmax );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Cornea1R2',  7.81,   0.25,     2.00,     1.0,  'boundary',      6.5,  realmax );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Cornea2R1',  6.44,   0.23,     2.00,     1.0,  'boundary',      5.5,  realmax );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Cornea2R2',  6.44,   0.23,     2.00,     1.0,  'boundary',      5.5,  realmax );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Cornea1k',  -0.29,   0.09,     3.00,     1.0,  'boundary', -realmax,  0.0 );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Cornea2k',  -0.34,   0.24,     3.00,     1.0,  'boundary', -realmax,  0.0 );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Cornea1Th',  0.0,    0.0,     45.00,     0.1,  'average',  -realmax,  realmax );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Cornea2Th',  0.0,    0.0,     45.00,     0.1,  'average',  -realmax,  realmax );
            for i = ZernikeNumCoeffs( 0 ) + 1 : ZernikeNumCoeffs( 2 )
            avars{ 2 }{ end + 1 } = self.MakeVariableAnatomical( cznames{ i }, 0.0,    0.0,      0.10,     1.0,  'boundary', -realmax, realmax );
            end
            for i = ZernikeNumCoeffs( 2 ) + 1 : ZernikeNumCoeffs( 4 )
            avars{ 2 }{ end + 1 } = self.MakeVariableAnatomical( cznames{ i }, 0.0,    0.0,      0.10,     1.0,  'boundary', -realmax, realmax );
            end
            for i = ZernikeNumCoeffs( 4 ) + 1 : ZernikeNumCoeffs( 6 )
            avars{ 2 }{ end + 1 } = self.MakeVariableAnatomical( cznames{ i }, 0.0,    0.0,      0.05,     1.0,  'boundary', -realmax, realmax );
            end
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'AqueousT',   2.90,   0.39,     1.00,     1.0,  'boundary',      2.2,      3.5 );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'LensD',      9.5,    0.3,      0.50,     1.0,  'boundary', -realmax,  realmax );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'LensV',    160.1,    2.5,      7.00,     0.1,  'boundary',      150,  realmax );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Lens1k',   - 4.4,    1.6,      6.00,     2.0,  'boundary', -realmax,     -1.0 );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Lens2k',   - 4.0,    2.0,      6.00,     2.0,  'boundary', -realmax,     -1.0 );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Lensdx',     0.0,    0.0,      0.80,     8.0,  'average',  -realmax,  realmax );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Lensdy',     0.0,    0.0,      0.80,     8.0,  'average',  -realmax,  realmax );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Lensax',     0.0,    0.0,      7.00,     1.0,  'average',  -realmax,  realmax );
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Lensay',     0.0,    0.0,      7.00,     1.0,  'average',  -realmax,  realmax );
            if self.optimization_params.measurement_lambda < 0
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'Lambda',   587.56,   0.0,    200.00,     0.0,  'boundary',    400.0,      700 );
            end
            if self.optimization_params.pupil_diameter < 0
            avars{ 1 }{ end + 1 } = self.MakeVariableAnatomical( 'PupilD',     6.0,    0.0,      3.00,     0.0,  'boundary', -realmax,   realmax );
            end

            % variables to echo when printing loss
            %%{
            evars{ end + 1 } = self.MakeVariableAnatomical( 'Corneadx',   0.0,    0.0,      0.30,   128.0, 'average',  -realmax,  realmax );
            evars{ end + 1 } = self.MakeVariableAnatomical( 'Corneady',   0.0,    0.0,      0.30,   128.0, 'average',  -realmax,  realmax );
            evars{ end + 1 } = self.MakeVariableAnatomical( 'Corneaax',   0.0,    0.0,      0.20,     1.0, 'average',  -realmax,  realmax );
            evars{ end + 1 } = self.MakeVariableAnatomical( 'Corneaay',   0.0,    0.0,      0.20,     1.0, 'average',  -realmax,  realmax );
            evars{ end + 1 } = self.MakeVariableAnatomical( 'Lens1R1',   10.54,   1.19,     2.00,     1.0, 'boundary', -realmax,  realmax );
            evars{ end + 1 } = self.MakeVariableAnatomical( 'Lens1R2',   10.38,   1.21,     6.00,     0.0, 'boundary', -realmax,  realmax );
            evars{ end + 1 } = self.MakeVariableAnatomical( 'Lens2R1',  - 6.94,   0.75,     2.00,     1.0, 'boundary', -realmax,  realmax );
            evars{ end + 1 } = self.MakeVariableAnatomical( 'Lens2R2',  - 6.84,   0.74,     5.00,     0.0, 'boundary', -realmax,  realmax );
            evars{ end + 1 } = self.MakeVariableAnatomical( 'LensT',     3.76,    0.22,     0.30,     1.0, 'boundary', -realmax,  realmax );
            evars{ end + 1 } = self.MakeVariableAnatomical( 'LensTh',     0.0,    0.0,     89.00,     0.1, 'average',  -realmax,  realmax );
            evars{ end + 1 } = self.MakeVariableAnatomical( 'VitreousT', 15.9,    0.73,     0.00,     0.0, 'boundary', -realmax,  realmax );
            %}
        end
        
        function variables = VariablesFunctional( self )
            % list of all the target variables
            variables = ...
            [ ...
                0.0 ...                         % n = 0
                0.5 0.5 ...                     % n = 1
                1.0 1.0 1.0 ...                 % n = 2
                1.0 1.0 1.0 1.0 ...             % n = 3
                2.0 2.0 2.0 2.0 2.0 ...         % n = 4
                4.0 4.0 4.0 4.0 4.0 4.0 ...     % n % 5
                4.0 4.0 4.0 4.0 4.0 4.0 4.0 ... % n = 6
            ];
        
            % scale factors
            scale_factors = ones( size( variables ) ) .* self.optimization_params.functional_weight_unspecified;
            for i = 1 : size( self.optimization_params.alpha, 1 )
                scale_factors( self.optimization_params.alpha( i, 1 ) ) = self.optimization_params.functional_weight_specified;
            end
            
            variables = variables .* scale_factors;
            variables = variables( 1 : ZernikeNumCoeffs( self.optimization_params.max_deg ) );
        end
    end
end

