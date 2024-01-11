function [ result, slices, edges, objects ] = ...
    ground_truth_barsky( color, depth, depth_slices, psfs )
    
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
    edges = find_edges( depth, edge_sigma, edge_threshold, true );
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
        inds_single = depth >= depth_begin & depth < depth_end;
        slices.depth{ i } = depth .* double( inds_single );
        
        % extend the slice
        edges_slice = find_edges( slices.depth{ i }, edge_sigma, edge_threshold, false );
        labels_slice = unique( labels( edges_slice > 0.5 ) );
        
        inds_extended = ismember( labels, labels_slice ) | inds_single;
        slices.color{ i } = color .* double( inds_extended );
        
        % convolve with the psf
        slices.filtered{ i } = zeros( imsize );
        for c = 1:3            
            % convolve
            psf = double( psfs{ min( i, num_slices ), min( c, size( psfs, 2 ) ) } );
            slices.filtered{ i }( :, :, c ) = filter( slices.color{ i }( :, :, c ), psf );
        end
        
        % compute the slice alpha
        alpha_slice = ones( imsize( 1:2 ) ) .* double( inds_single );
        alpha_slice = repmat( filter( alpha_slice, psf ), 1, 1, 3 );
        slices.alpha{ i } = alpha_slice;
        
        % blend the esult
        result = alpha_slice .* slices.filtered{ i } + result;
        alpha = alpha_slice + alpha;
        
        % store the partial result
        slices.partial{ i } = result;
    end
    
    % normalize the result
    result = result ./ alpha;
    tEnd = toc( tStart );
    fprintf( 'Processing time: %f s\n', tEnd );
end

function result = filter( col, kernel )
    result = imfilter( col, kernel, 'replicate' );
    %result = conv2d( col, kernel, 'same' );
end

function edges = find_edges( d, sigma, threshold_scale, connect )
    [~,threshOut] = edge( d, 'Canny', [], sigma );
    threshold = threshOut * threshold_scale;
    edges = edge( d, 'Canny', threshold, sigma );
    if connect
        [ ~, edges ] = edgelink( edges, 3 );
    end
end