function cn = find_hexagonal_number( cnt )
    cnt1 = round( cnt * 2 * sqrt(3) / pi );
    tmp = (-3 + sqrt( 9 - 12 * ( 1 - cnt1 ) ) ) / 6;
    cn( 1 ) = floor( tmp );
    cn( 2 ) = ceil(  tmp );
    totn = 1 + 3 * cn .* ( 1 + cn );
    [ ~, i ] = min( abs( totn - cnt1 ) );
    cn = cn( i );
end