function [ rgb ] = hex2rgb( hex )
    hex( :, 1 ) = [];
    rgb = sscanf( hex.', '%2x' ).' ./ 255;
end

