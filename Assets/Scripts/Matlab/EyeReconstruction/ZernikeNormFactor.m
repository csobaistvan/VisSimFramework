function norm_factor = ZernikeNormFactor( c )
    id = ZernikeIndices( c );
    if id( 2 ) == 0
        norm_factor = sqrt( id( 1 ) + 1 );
    else
        norm_factor = sqrt( 2 * ( id( 1 ) + 1 ) );
    end
end