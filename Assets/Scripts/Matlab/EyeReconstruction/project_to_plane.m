function [xy] = project_to_plane( p0, n, positions, up )
    if nargin < 4, up = [ 0, 0, 1 ]; end
    
    % number of input points
    np = size( positions, 1 );
    
    % x and y vectors w.r.t. the plane
    x = cross( n, up ); x = x ./ sqrt( sum( x.^2 ) );
    y = cross( x, n ); y = y ./ sqrt( sum( y.^2 ) );
    
    % positions relative to the plane center
    p_loc = positions - p0;
    
    % 2d locations
    t1 = dot( repmat( x, np, 1 ), p_loc, 2 );
    t2 = dot( repmat( y, np, 1 ), p_loc, 2 );
    %[ x; y ]
    
    % write out the results
    xy = [ t1, t2 ];
end