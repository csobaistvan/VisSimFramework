function z = ZernikeEvalSurface( alpha, r, t )
    z = zeros( size( r ) );
    for i = 1 : length( alpha )
        [ n, m ] = ZernikeIndices( i );
        z = z + alpha( i ) * Zernike( r, t, n, m );
    end
end