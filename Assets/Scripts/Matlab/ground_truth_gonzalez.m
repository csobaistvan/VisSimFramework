function [ result, slices, edges, objects ] = ...
    ground_truth_gonzalez( color, field_coords, depth_slices, hor_slices, vert_slices, psfs )
    
    tStart = tic;
    edge_sigma = 10.0;
    %edge_threshold = 1.0 * ( 1.0 / edge_sigma );
    edge_threshold = 0.5;

    % convert the color to normalized double
    color = double( color ) / 255;
    
    % resulting image
    imsize = size( color );
    result = zeros( imsize );
    
    % object labels
    edges = find_edges( field_coords( :, :, 3 ), edge_sigma, edge_threshold, true );
    labels = watershed( imcomplement( edges ) );
    objects = label2rgb( labels );

    % total number of slices
    num_slices = length( depth_slices );
    diopter_slices = 1 ./ depth_slices;
    
    % output slices
    slices = struct;
    slices.color = cell( num_slices );
    slices.filtered = cell( num_slices );
    slices.depth = cell( num_slices );
    slices.alpha = cell( num_slices );
    slices.partial = cell( num_slices );
    alpha = zeros( imsize );
    
    for i = 1:num_slices
        fprintf( "Slice %d/%d\n", i, num_slices );
        
        % depth start and end slices
        if i == 1
            depth_end = realmax;
            depth_begin = 1.0 / ( 0.5 * diopter_slices( 1 ) + 0.5 * diopter_slices( 2 ) );
        elseif i == num_slices
            depth_begin = 0;
            depth_end = 1.0 / ( 0.5 * diopter_slices( num_slices - 1 ) + 0.5 * diopter_slices( num_slices ) );
        else
            depth_begin = 1.0 / ( 0.5 * diopter_slices( i + 1 ) + 0.5 * diopter_slices( i ) );
            depth_end = 1.0 / ( 0.5 * diopter_slices( i ) + 0.5 * diopter_slices( i - 1 ) );
        end

        % create the ith depth slice
        inds_single = field_coords( :, :, 3 ) >= depth_begin & field_coords( :, :, 3 ) < depth_end;
        slices.depth{ i } = field_coords( :, :, 3 ) .* double( inds_single );
        slices.mask{ i } = ones( imsize( 1:2 ) ) .* double( inds_single );
        
        % extend the slice
        edges_slice = find_edges( slices.depth{ i }, edge_sigma, edge_threshold, false );
        labels_slice = unique( labels( edges_slice > 0.5 ) );
        
        inds_extended = ismember( labels, labels_slice ) | inds_single;
        slices.color{ i } = color .* double( inds_extended );
        
        % convolve with the psf
        slices.filtered{ i } = zeros( imsize );
        for c = 1:3
            fprintf( " > Channel %d/%d\n", c, 3 );
            % extract the PSF
            psf = psfs( min( i, num_slices ), min( c, size( psfs, 2 ) ), :, : );
            psf = reshape( psf, length( hor_slices ), length( vert_slices ) );
            
            % pad each PSF
            max_size = 0;
            for h = 1:length( hor_slices )
            for v = 1:length( vert_slices )
                psf_size = length( psf{ h, v } );
                max_size = max( max_size, psf_size );
            end, end
            
            for h = 1:length( hor_slices )
            for v = 1:length( vert_slices )
                psf_size = length( psf{ h, v } );
                pad_size = fix( ( max_size - psf_size ) / 2 );
                psf{ h, v } = padarray( psf{ h, v }, [ pad_size, pad_size ], 0.0, 'both' );
                psf{ h, v } = double( psf{ h, v } );
                %psf{ h, v } = flipud( fliplr( psf{ h, v } ) );
            end, end
        
            % convolve
            slice = slices.color{ i }( :, :, c );
            mask = slices.mask{ i };
            [ slice_filtered, alpha_filtered ] = filter( slice, field_coords, mask, psf, hor_slices, vert_slices );
            slices.filtered{ i }( :, :, c ) = slice_filtered;
            slices.alpha{ i } = alpha_filtered;
        end
        
        % blend the esult
        %result = slices.alpha{ i } .* slices.filtered{ i } + ( 1.0 - slices.alpha{ i } ) .* result;
        %alpha = slices.alpha{ i } + ( 1.0 - slices.alpha{ i } ) .* alpha;
        result = slices.alpha{ i } .* slices.filtered{ i } + result;
        alpha = slices.alpha{ i } + alpha;
        
        % store the partial result
        slices.partial{ i } = result;
    end
    
    % normalize the result
    result = result ./ alpha;
    tEnd = toc( tStart );
    fprintf( 'Processing time: %f s\n', tEnd );
end

function [ result, alpha ] = filter( col, field_coords, mask, kernels, hor_slices, vert_slices )
    % initialize the result
    img_size = size( col );
    iw = img_size( 1 );
    ih = img_size( 2 );
    result = zeros( img_size );
    alpha = zeros( img_size );
    
    % process each pixel of the layer
    for yi = 1:ih
        fprintf( "   >> Column %d/%d\n", yi, ih );
        parfor xi = 1:iw
            % compute the convolution kernel
            center_coords = reshape( field_coords( xi, yi, : ), 1, [] );
            kernel = lerp_psfs( kernels, hor_slices, vert_slices, center_coords );
            kern_size = length( kernel );
            kern_radius = fix( kern_size / 2 );
            
            % accummulate the sample
            px_col = 0;
            px_mask = 0;
            for yk = 1:kern_size
            for xk = 1:kern_size
                sample_coords = min( max( [ xi - kern_radius - 1 + xk, yi - kern_radius - 1 + yk ], [ 1, 1 ] ), img_size );
                % calculate the samples
                sample_col = col( sample_coords( 1 ), sample_coords( 2 ) );
                sample_mask = mask( sample_coords( 1 ), sample_coords( 2 ) );
                % evaluate the kernel
                weight = kernel( xk, yk );
                % accummulate
                px_col = px_col + sample_col * weight;
                px_mask = px_mask + sample_mask * weight;
            end, end
        
            % store the result
            result( xi, yi ) = px_col;
            alpha( xi, yi ) = px_mask;
        end
    end
end

function edges = find_edges( d, sigma, threshold_scale, connect )
    [~,threshOut] = edge( d, 'Canny', [], sigma );
    threshold = threshOut * threshold_scale;
    edges = edge( d, 'Canny', threshold, sigma );
    if connect
        [ ~, edges ] = edgelink( edges, 3 );
    end
end

function kernel = lerp_psfs( psfs, hor_slices, vert_slices, field_point )
    % identify the four relevant PSFs
    [~, horidxi] = min( abs( hor_slices - field_point( 1 ) ) ); horidxj = min( horidxi + 1, length( hor_slices ) );
    [~, veridxi] = min( abs( vert_slices - field_point( 2 ) ) ); veridxj = min( veridxi + 1, length( vert_slices ) );
    hori = hor_slices( horidxi ); horj = hor_slices( horidxj );
    veri = vert_slices( veridxi ); verj = vert_slices( veridxj );
    
    % interpolation coordinates
    diff = [ field_point( 1 ), field_point( 2 ) ] - [ hori, veri ];
    slice_diff = abs( [ horj - hori, verj - veri ] );
    uv = diff ./ slice_diff;
    uv = max( min( uv, [ 0, 0 ] ), [ 1, 1 ] );
    
    % extract the appropriate PSFs
    psf11 = psfs{ horidxi, veridxi };
    psf12 = psfs{ horidxi, veridxj };
    psf21 = psfs{ horidxj, veridxi };
    psf22 = psfs{ horidxj, veridxj };
    
    % interpolate along the horizontal axis
    psf1 = lerp( psf11, psf12, uv( 1 ) );
    psf2 = lerp( psf21, psf22, uv( 1 ) );
    
    % interpolate along the vertical axis
    kernel = lerp( psf1, psf2, uv( 2 ) );
end

function result = lerp( a, b, alpha )
    result = ( 1.0 - alpha ) * a + alpha * b;
end