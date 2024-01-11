function n_refr = refrindx( wavelength, glass )
% REFRINDX refractive index calculations.
%
% INPUT:
%   wavelength - light wavelength in meters
%   glass - material string
%
% OUTPUT:
%   n_refr - refractive index for the wavelengths: 587.6, 486.1, 656.3 nm
%
% Data from JML Optical Industries, Inc available at:
%   http://www.netacc.net/~jmlopt/transmission2.html
%   Or at:
%   http://www.jmlopt.com/level2/TechInfo/materialstable.aspx
%
%   The human eye data are from in Escudero-Sanz & Navarro, 
%   "Off-axis aberrations of a wide-angle schematic eye model", 
%   JOSA A, 16(8), 1881-1891 (1999).

% glass name, abc, Mu
glasses = { ...
    'air', [ 0.000000, -0.000000, 1.000000 ], [ 3149974800465.330078, 980141478368.826172 ]; ...
    'mirror', [ 0.000000, -0.000000, 1.000000 ], [ 3149974800465.330078, 980141478368.826172 ]; ...
    'soot', [ 0.000000, -0.000000, 1.000000 ], [ 3149974800465.330078, 980141478368.826172 ]; ...
    'cornea', [ 0.000145, 0.003461, 1.377691 ], [ 3383918571882.492188, 992337142120.325562 ]; ...
    'aqueous', [ 0.000180, 0.003608, 1.339115 ], [ 3383918571882.492188, 992337142120.325562 ]; ...
    'lens', [ 0.000300, 0.004601, 1.422200 ], [ 3383918571882.492188, 992337142120.325562 ]; ...
    'vitreous', [ 0.000145, 0.003461, 1.337691 ], [ 3383918571882.492188, 992337142120.325562 ]; ...
};
glass_names = glasses( :, 1 );

if isempty( wavelength )
    wavelength = 5876e-10; % green
end

id = find( strcmp( glass_names, glass ) );
abc = glasses{ id, 2 };
Mu = glasses{ id, 3 };

n_refr = polyval( abc, 1./wavelength.^2, [], Mu );

end
