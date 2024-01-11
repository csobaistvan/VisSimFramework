function ZernikePSF( coefficients, focal_plane_sampling, num_samples, lambda, aperture_diameter, focal_length )
    % convert inputs to meters
    focal_plane_sampling = focal_plane_sampling * 1e-6;
    lambda = lambda * 1e-9;
    aperture_diameter = aperture_diameter * 1e-3;
    focal_length = focal_length * 1e-3;
    
    % Calculate pupil plane sampling
    delta_fx = 1 / ( focal_plane_sampling * num_samples );
    x_pupil = (-fix(num_samples/2):fix((num_samples-1)/2)) * delta_fx * lambda * focal_length;
    [X_pupil,Y_pupil] = meshgrid(x_pupil);
    [ t_pupil, r_pupil ] = cart2pol( X_pupil, Y_pupil );
    R_pupil = sqrt(X_pupil.^2 + Y_pupil.^2);
    R_norm = R_pupil/(aperture_diameter/2);
    
    z = ZernikeEvalSurface( coefficients, R_norm, t_pupil );
    
    % create wavefront
    E = exp( 1i * 2 * pi * z ); % complex amplitude
    E(R_norm>1) = 0; % impose aperture size
    %E(r_pupil / max(max(r_pupil)) > 1) = 0; % impose aperture size
    
    % create point-spread function
    psf = abs(fftshift(fft2(ifftshift(E)))).^2;
    psf = psf/sum(psf(:));
    x_psf = (-fix(num_samples/2):fix((num_samples-1)/2)) * focal_plane_sampling;
    figure;imagesc(x_psf*1e6,x_psf*1e6,psf);
    title(sprintf('Point-spread function'));
    xlabel(sprintf('Position (\\mum)'));
    ylabel(sprintf('Position (\\mum)'));

end