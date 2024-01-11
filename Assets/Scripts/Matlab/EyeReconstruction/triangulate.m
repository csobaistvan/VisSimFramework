function [ X, Y, Z ] = triangulate(x,y,z)

X = [ ];
Y = [ ];
Z = [ ];
for i=1:(size(z,1)-1)
    for j=1:(size(z,2)-1)
        
        p1 = [x(i,j)     y(i,j)     z(i,j)];
        p2 = [x(i,j+1)   y(i,j+1)   z(i,j+1)];
        p3 = [x(i+1,j+1) y(i+1,j+1) z(i+1,j+1)];
        X = [ X; p1( 1 ), p2( 1 ), p3( 1 ) ];
        Y = [ Y; p1( 2 ), p2( 2 ), p3( 2 ) ];
        Z = [ Z; p1( 3 ), p2( 3 ), p3( 3 ) ];
        
        p1 = [x(i+1,j+1) y(i+1,j+1) z(i+1,j+1)];
        p2 = [x(i+1,j)   y(i+1,j)   z(i+1,j)];
        p3 = [x(i,j)     y(i,j)     z(i,j)];
        X = [ X; p1( 1 ), p2( 1 ), p3( 1 ) ];
        Y = [ Y; p1( 2 ), p2( 2 ), p3( 2 ) ];
        Z = [ Z; p1( 3 ), p2( 3 ), p3( 3 ) ];
        
    end
end
        
    