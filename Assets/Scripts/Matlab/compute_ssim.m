function [mmssim, mssim, ssim_map_pc, ssim_map] = compute_ssim( result, reference )
    ssim_map_pc = zeros( size( reference ) );
    mssim = zeros( 1, 3 );
    for i = 1:3
        [mssim_channel, ssim_map_channel] = ssim( result( :, :, i ), reference( :, :, i ) );
        mssim( 1, i ) = mssim_channel;
        ssim_map_pc( :, :, i ) = ssim_map_channel;
    end
    mmssim = mean( mssim );
    
    % generate the jet-colored figure
    ssim_map = ind2rgb( im2uint8( mean( ssim_map_pc, 3 ) ) + 1, flipud( jet( 256 ) ) );
    
    %{
    fig = figure( 'visible', 'off' );
    imshow( mean( ssim_map_pc, 3 ) );
    colormap( fig, jet );
    colorbar;
    ax = gca;
    outerpos = ax.OuterPosition;
    ti = ax.TightInset; 
    left = outerpos(1) + ti(1);
    bottom = outerpos(2) + ti(2);
    ax_width = outerpos(3) - ti(1) - ti(3);
    ax_height = outerpos(4) - ti(2) - ti(4);
    ax.Position = [left bottom ax_width ax_height];
    frame = getframe( fig );
    ssim_map = frame.cdata;
    close( fig );
    %}
end