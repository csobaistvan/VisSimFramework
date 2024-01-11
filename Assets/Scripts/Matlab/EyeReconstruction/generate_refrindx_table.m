lambda_ref = [ 5876 4861 6563 ] * 1e-10;
lambda_eye = [ 4580 5430 5893 6328 ] * 1e-10;

glasses = { ...
    { 'air', [ 1 1 1 ], lambda_ref }, ...
    { 'mirror', [ 1 1 1 ], lambda_ref }, ...
    { 'soot', [ 1 1 1 ], lambda_ref }, ...
    { 'cornea', [1.3828 1.3777 1.376 1.3747], lambda_eye }, ...
    { 'aqueous', [1.3445 1.3391 1.3374 1.336], lambda_eye }, ...
    { 'lens', [1.4292 1.4222 1.42 1.4183], lambda_eye }, ...
    { 'vitreous', [1.3428 1.3377 1.336 1.3347], lambda_eye }, ...
};

cell_length = 12;
str_format = '%s';
flt_format = '%f';
row_format = sprintf( '    ''%s'', [ %s, %s, %s ], [ %s, %s ]; ...\n', str_format, flt_format, flt_format, flt_format, flt_format, flt_format );
fid = fopen( 'refrindex_table.ref', 'w' );
fprintf( fid, 'glasses = { ...\n' );
for i = 1:length( glasses )
    glass = glasses{ i };
    glass_name = glass{ 1 };
    nref = glass{ 2 };
    lambda = glass{ 3 };
    [ abc, ~, Mu ] = polyfit( 1./lambda.^2, nref, 2 );
    fprintf( fid, row_format, glass_name, abc( 1 ), abc( 2 ), abc( 3 ), Mu( 1 ), Mu( 2 ) );
end
fprintf( fid, '};' );
fclose( fid );