function [peaksnr, snr] = compute_ssim( result, reference )
    peaksnr = zeros( 1, 3 );
    snr = zeros( 1, 3 );
    for i = 1:3
        [ peaksnr_channel, snr_channel ] = psnr( result( :, :, i ), reference( :, :, i ) );
        peaksnr( 1, i ) = peaksnr_channel;
        snr( 1, i ) = snr_channel;
    end
end