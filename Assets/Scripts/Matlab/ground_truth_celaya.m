function result = ground_truth_celaya( color, field_coords, depth_slices, hor_slices, vert_slices, psfs )
    
    tStart = tic;

    % convert the color to normalized double
    color = double( color ) / 255;
    
    % resulting image
    imsize = size( color );
    result = zeros( imsize );
    
    fprintf( "hor: %f\n", hor_slices );
    fprintf( "vert: %f\n", vert_slices );
    fprintf( "depth: %f\n", depth_slices );

    % convolve with the psf
    for c = 1:3
        fprintf( "Channel %d/%d\n", c, 3 );
        % extract the PSF
        psf = psfs( :, min( c, size( psfs, 2 ) ), :, : );
        psf = reshape( psf, length( depth_slices ), length( hor_slices ), length( vert_slices ) );
        
        % pad each PSF
        max_size = 0;
        for d = 1:length( depth_slices )
        for h = 1:length( hor_slices )
        for v = 1:length( vert_slices )
            psf_size = length( psf{ d, h, v } );
            max_size = max( max_size, psf_size );
        end, end, end
        
        for d = 1:length( depth_slices )
        for h = 1:length( hor_slices )
        for v = 1:length( vert_slices )
            psf_size = length( psf{ d, h, v } );
            pad_size = fix( ( max_size - psf_size ) / 2 );
            psf{ d, h, v } = padarray( psf{ d, h, v }, [ pad_size, pad_size ], 0.0, 'both' );
            psf{ d, h, v } = double( psf{ d, h, v } );
            %psf{ d, h, v } = flipud( fliplr( psf{ d, h, v } ) );
        end, end, end
    
        % convolve
        result( :, :, c ) = filter( color( :, :, c ), field_coords, psf, depth_slices, hor_slices, vert_slices );
    end
    
    tEnd = toc( tStart );
    fprintf( 'Processing time: %f s\n', tEnd );
end

function result = filter( col, field_coords, kernels, depth_slices, hor_slices, vert_slices )
    % initialize the result
    img_size = size( col );
    iw = img_size( 1 );
    ih = img_size( 2 );
    result = zeros( img_size );
    
    % process each pixel of the layer
    for yi = 1:ih
        if yi == 1 || yi == ih || mod( yi, 100 ) == 0
            fprintf( "  > Column %d/%d\n", yi, ih );
        end
        parfor xi = 1:iw
            % compute the convolution kernel
            center_coords = reshape( field_coords( xi, yi, : ), 1, [] );
            kernel = lerp_psfs( kernels, hor_slices, vert_slices, depth_slices, center_coords );
            kern_size = length( kernel );
            kern_radius = fix( kern_size / 2 );
            
            % accummulate the sample value
            px = 0;
            num_samples = 0;
            for yk = 1:kern_size
            for xk = 1:kern_size
                sample_coords = min( max( [ xi - kern_radius - 1 + xk, yi - kern_radius - 1 + yk ], [ 1, 1 ] ), img_size );
                px = px + col( sample_coords( 1 ), sample_coords( 2 ) ) * kernel( xk, yk );
                num_samples = num_samples + 1;
            end, end
        
            % store the result
            result( xi, yi ) = px;
        end
    end
end

function kernel = lerp_psfs( psfs, hor_slices, vert_slices, depth_slices, field_point )
    % identify the eight relevant PSFs
    [~, horidxi] = min( abs( hor_slices - field_point( 1 ) ) ); horidxj = min( horidxi + 1, length( hor_slices ) );
    [~, veridxi] = min( abs( vert_slices - field_point( 2 ) ) ); veridxj = min( veridxi + 1, length( vert_slices ) );
    [~, depthidxi] = min( abs( depth_slices - field_point( 3 ) ) ); depthidxj = min( depthidxi + 1, length( depth_slices ) );
    hori = hor_slices( horidxi ); horj = hor_slices( horidxj );
    veri = vert_slices( veridxi ); verj = vert_slices( veridxj );
    depthi = depth_slices( depthidxi ); depthj = depth_slices( depthidxj );
    
    % interpolation coordinates
    diff = [ field_point( 3 ), field_point( 1 ), field_point( 2 ) ] - [ depthi, hori, veri ];
    slice_diff = abs( [ depthj - depthi, horj - hori, verj - veri ] );
    uv = diff ./ slice_diff;
    uv = max( min( uv, [ 0, 0, 0 ] ), [ 1, 1, 1 ] );

    % extract the appropriate PSFs
    psf111 = psfs{ depthidxi, horidxi, veridxi };
    psf112 = psfs{ depthidxi, horidxi, veridxj };
    psf121 = psfs{ depthidxi, horidxj, veridxi };
    psf122 = psfs{ depthidxi, horidxj, veridxj };
    psf211 = psfs{ depthidxj, horidxi, veridxi };
    psf212 = psfs{ depthidxj, horidxi, veridxj };
    psf221 = psfs{ depthidxj, horidxj, veridxi };
    psf222 = psfs{ depthidxj, horidxj, veridxj };
    
    % interpolate along the depth axis
    psf11 = lerp( psf111, psf211, uv( 1 ) );
    psf12 = lerp( psf112, psf212, uv( 1 ) );
    psf21 = lerp( psf121, psf221, uv( 1 ) );
    psf22 = lerp( psf122, psf222, uv( 1 ) );

    % interpolate along the horizontal axis
    psf1 = lerp( psf11, psf12, uv( 2 ) );
    psf2 = lerp( psf21, psf22, uv( 2 ) );

    % interpolate along the vertical axis
    kernel = lerp( psf1, psf2, uv( 3 ) );
end

function result = lerp( a, b, alpha )
    result = ( 1.0 - alpha ) * a + alpha * b;
end