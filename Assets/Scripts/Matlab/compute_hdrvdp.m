function [P_det, P_map, P_map_jet] = compute_hdrvdp( result, reference, ...
    Y_peak, contrast, gamma, E_ambient, display_size, display_dimensions, ...
    view_distance, surround, sens_correction )
    addpath( 'hdrvdp-3.0.6' );
    
    % Computation parameters
    task = 'detection';
    color_space = 'rgb-native';
    options = {};
    options = [ 'surround', surround, options ];
    options = [ 'rgb_display', 'led-lcd-srgb', options ];
    options = [ 'do_robust_pdet', true, options ];
    options = [ 'sensitivity_correction', sens_correction, options ];
    
    % rescale the images
    reference_scaled = hdrvdp_gog_display_model( ( double( reference ) / 255 ), Y_peak, contrast, gamma, E_ambient );
    result_scaled = hdrvdp_gog_display_model( ( double( result ) / 255 ), Y_peak, contrast, gamma, E_ambient );
    
    ppd = hdrvdp_pix_per_deg( display_size, display_dimensions, view_distance );
    res = hdrvdp3( task, reference_scaled, result_scaled, color_space, ppd, options );
    P_det = res.P_det;
    P_map = res.P_map;
    P_map_jet = ind2rgb( im2uint8( P_map ) + 1, jet( 256 ) );
    %P_map_jet = hdrvdp_visualize( 'pmap', res.P_map );
end