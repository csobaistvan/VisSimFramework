
function [ fdist, coc ] = compute_eye_focus_distance( pd, num_passes, num_rays, num_subdivs, eye_parameters, varargin )
    % test eye
    test_eye = EyeParametric( );

    % set the eye parameters    
    for i = 1:length( eye_parameters )
        pname = eye_parameters{ i, 1 };
        pval = eye_parameters{ i, 2 };
        % fprintf( '[%d]: %s: %f\n', i, pname, pval );
        if isprop( test_eye, pname )
            test_eye.( pname ) = pval;
        end
    end

    % Setup the test eye
    test_eye.PupilD = pd;
    test_eye.Lambda = 587.56;
    test_eye.MakeElements( );
    
    % compute the focus distance
    %[ fdist, coc ] = test_eye.FocusDistance( 587.56, num_passes, num_rays, num_subdivs, [ 0.01, 8.0 ] );
    pd
    [ fdist, coc ] = test_eye.FocusDistance( 587.56, 5, 100, 10, [ 0.01, 8.0 ] );
    fdist
end