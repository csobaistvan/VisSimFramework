function print_aberrations( coeffs, names, col_length, col_spacing, precision )
    % print settings
    if nargin < 3, col_length = 14; end
    if nargin < 4, col_spacing = 4; end
    if nargin < 6, precision = 5; end
    % prepend the 'Coefficient' column name
    names = [ {'Coefficient'}, names ];
    % Print the header
    for i = 1 : length( names )
        if i == 1
            format = '%-*s';
        else
            format = '%*s';
        end
        fprintf( format, col_length, names{ i } );
        fprintf( '%*s', col_spacing, ' ' );
    end
    fprintf( '\n' );
    % Print the underlines
    for i = 1 : length( names )
        if i == 1
            format = '%-*s';
        else
            format = '%*s';
        end
        underline = '---------------------------------------------';
        fprintf( format, col_length, underline( 1 : strlength( names{ i } ) ) );
        fprintf( '%*s', col_spacing, ' ' );
    end
    fprintf( '\n' );
    col_lengths = zeros( 1, size( coeffs, 2 ) );
    for i = 1 : size( coeffs, 1 )
    for j = 1 : size( coeffs, 2 )
        %coeff_length = sgnlen( coeffs( i, j ) ) + int( log10( coeffs( i, j ) ) );
        coeff_length = floor( log10( abs( coeffs( i, j ) ) ) ) + precision + 2;
        if isinf( coeff_length ) == false
            col_lengths( j ) = max( col_lengths( j ), coeff_length  );
        end
    end
    end
    % Print the coeffs themselves
    for i = 1 : size( coeffs, 1 )
        [ n, m ] = ZernikeIndices( i );
        coeff_name = sprintf( 'Z[%2d, %2d ]: ', n, m );
        fprintf( '%-*s', col_length, coeff_name );
        for j = 1 : size( coeffs, 2 )
            fprintf( '%*s', col_spacing, ' ' );
            fprintf( '%*s%1s%*.*f', col_length - 1 - col_lengths( j ), ' ', sgnchar( coeffs( i, j ) ), col_lengths( j ), precision, abs( coeffs( i, j ) ) );
        end
        fprintf( '\n' );
    end
end

function c = sgnlen( x )
    if x < 0, c = 1;
    else, c = 0; end
end
        
function c = sgnchar( x )
    if x < 0, c = '-';
    else, c = ' '; end
end