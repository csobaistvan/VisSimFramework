
function [ lensd, aqueoust, delalensd, deltaaqueoust ] = compute_focused_eye( pd, fdist, num_passes, num_rays, num_subdivs, eye_parameters, varargin )
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
    
    % compute the focused parameters
    focused_eye = test_eye.FocusAt( fdist, 587.56, num_passes, num_rays, num_subdivs, [ test_eye.LensD * 0.91, test_eye.LensD ] );

    % return the result
    lensd = focused_eye.LensD;
    aqueoust = focused_eye.AqueousT;
    delalensd = focused_eye.LensD - test_eye.LensD;
    deltaaqueoust = focused_eye.AqueousT - test_eye.AqueousT;
end