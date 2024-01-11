function RMS = ZernikeRMS( c )
    [ n, m ] = ZernikeIndices( c );
    if m == 0
        RMS = sqrt( n + 1 );
    else
        RMS = sqrt( 2 * ( n + 1 ) );
    end
end