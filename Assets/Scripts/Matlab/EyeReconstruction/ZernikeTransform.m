function C2 = ZernikeTransform(C1,dia1,dia2,tx,ty,thetaR,thetaE)
    % "ZernikeTransform" returns transformed Zernike coefficient set, C2, from the original set, C1,
    % both in standard ANSI order
    % dia1-original pupil diameter
    % dia2-new pupil diameter [mm]
    % tx, ty-Cartesian translation coordinates [mm]
    % thetaR-angle of rotation [degrees]
    % thetaE-angle of elliptical shrink [degrees]
    % Scaling and translation is performed first and then rotation.
    dia2ma=max(max(dia2));
    dia2mi=min(min(dia2));
    etaS=dia2ma/dia1; % Scaling factor
    etaT=2*sqrt(tx^2+ty^2/dia1); % Translation coordinates
    etaE=dia2mi/dia2ma;
    thetaT=atan2(ty,tx);
    thetaR=thetaR*pi/180; % Rotation in radians
    thetaE=thetaE*pi/180; % Rotation in radians
    jnm=length(C1)-1; nmax=ceil((-3+sqrt(9+8*jnm))/2); jmax=nmax*(nmax+3)/2;
    S=zeros(jmax+1,1); S(1:length(C1))=C1; C1=S; clear S
    P=zeros(jmax+1); % Matrix P transforms from standard to Campbell order
    N=zeros(jmax+1); % Matrix N contains the normalization coefficients
    R=zeros(jmax+1); % Matrix R is the coefficients of the radial polynomials
    CC1=zeros(jmax+1,1); % CC1 is a complex representation of C1
    counter=1;
    for m=-nmax:nmax % Meridional indexes
    for n=abs(m):2:nmax % Radial indexes
        jnm=(m+n*(n+2))/2;
        P(counter,jnm+1)=1;
        N(counter,counter)=sqrt(n+1);
        for s=0:(n-abs(m))/2
            R(counter-s,counter)=(-1)^s*factorial(n-s)/(factorial(s)*factorial((n+m)/2-s)*factorial((n-m)/2-s));
        end
        if m<0 CC1(jnm+1)=(C1((-m+n*(n+2))/2+1)+i*C1(jnm+1))/sqrt(2);
        elseif m==0 CC1(jnm+1)=C1(jnm+1);
        else CC1(jnm+1)=(C1(jnm+1)-i*C1((-m+n*(n+2))/2+1))/sqrt(2);
        end
        counter=counter+1;
    end,end
    ETAC=[];ETAS=[]; ETAE=[]; ETAT=[]; ETAR=[]; % Coordinate-transfer matrces
    for m=-nmax:nmax
    for n=abs(m):2:nmax
        ETAC=[ETAC P*(transform(n,m,jmax,etaS,etaT,thetaT,thetaR))];
        ETAS=[ETAS P*(transformScale(n,m,jmax,etaS))];
        ETAE=[ETAE P*(transformElliptical(n,m,jmax,etaE,thetaE))];
        ETAT=[ETAT P*(transformTranslate(n,m,jmax,etaT,thetaT))];
        ETAR=[ETAR P*(transformRotate(n,m,jmax,thetaR))];
    end,end
    ETA=ETAR*ETAT*ETAE*ETAS;
    %ETA
    %ETAC-(ETAR*ETAT*ETAE*ETAS)
    %ETAC-(ETAR*ETAT*ETAS)
    %ETAC-(ETAR*ETAS*ETAT)
    C=inv(P)*inv(N)*inv(R)*ETA*R*N*P;
    CC2=C*CC1;
    C2=zeros(jmax+1,1); % C2 is formed from the complex Zernike coefficients, CC2
    for m=-nmax:nmax
    for n=abs(m):2:nmax
        jnm=(m+n*(n+2))/2;
        if m<0, C2(jnm+1)=imag(CC2(jnm+1)-CC2((-m+n*(n+2))/2+1))/sqrt(2);
        elseif m==0, C2(jnm+1)=real(CC2(jnm+1));
        else C2(jnm+1)=real(CC2(jnm+1)+CC2((-m+n*(n+2))/2+1))/sqrt(2);
        end
    end,end
end

function Eta=transform(n,m,jmax,etaS,etaT,thetaT,thetaR)
    % Returns coefficients for transforming a ro^n*exp(i*m*theta)-term into '-terms
    Eta=zeros(jmax+1,1);
    for p=0:((n+m)/2)
    for q=0:((n-m)/2)
        nnew=n-p-q; mnew=m-p+q;
        jnm=(mnew+nnew*(nnew+2))/2;
        Eta(floor(jnm+1))=Eta(floor(jnm+1))+...
            nchoosek((n+m)/2,p)*nchoosek((n-m)/2,q)*...
            etaS^(n-p-q)*...
            etaT^(p+q)*...
            exp(i*((p-q)*(thetaT-thetaR)+m*thetaR));
    end
    end
end

function Eta=transformTranslateScale(n,m,jmax,etaS,etaT,thetaT)
    % Returns coefficients for transforming a ro^n*exp(i*m*theta)-term into '-terms
    Eta=zeros(jmax+1,1);
    for p=0:((n+m)/2)
    for q=0:((n-m)/2)
        nnew=n-p-q; mnew=m-p+q;
        jnm=(mnew+nnew*(nnew+2))/2;
        Eta(floor(jnm+1))=Eta(floor(jnm+1))+...
            nchoosek((n+m)/2,p)*nchoosek((n-m)/2,q)*...
            etaS^(n-p-q)*...
            etaT^(p+q)*...
            exp(i*((p-q)*thetaT));
    end
    end
end

function Eta=transformTranslate(n,m,jmax,etaT,thetaT)
    % Returns coefficients for transforming a ro^n*exp(i*m*theta)-term into '-terms
    Eta=zeros(jmax+1,1);
    for p=0:((n+m)/2)
    for q=0:((n-m)/2)
        nnew=n-p-q; mnew=m-p+q;
        jnm=(mnew+nnew*(nnew+2))/2;
        Eta(floor(jnm+1))=Eta(floor(jnm+1))+...
            nchoosek((n+m)/2,p)*nchoosek((n-m)/2,q)*...
            etaT^(p+q)*...
            exp(i*((p-q)*thetaT));
    end
    end
end

function Eta=transformScale(n,m,jmax,etaS)
    % Returns coefficients for transforming a ro^n*exp(i*m*theta)-term into '-terms
    Eta=zeros(jmax+1,1);
    jnm=(m+n*(n+2))/2;
    Eta(floor(jnm+1))=etaS^n;
end

function Eta=transformRotate(n,m,jmax,thetaR)
    % Returns coefficients for transforming a ro^n*exp(i*m*theta)-term into '-terms
    Eta=zeros(jmax+1,1);
    jnm=(m+n*(n+2))/2;
    Eta(floor(jnm+1))=exp(i*((m*thetaR)));
end

function Eta=transformElliptical(n,m,jmax,etaE,thetaE)
    % Returns coefficients for transforming a ro^n*exp(i*m*theta)-term into '-terms
    Eta=zeros(jmax+1,1);
    for p=0:((n+m)/2)
    for q=0:((n-m)/2)
        nnew=n; mnew=m-2*p+2*q;
        jnm=(mnew+nnew*(nnew+2))/2;
        Eta(floor(jnm+1))=Eta(floor(jnm+1))+...
            1/(2^n)*...
            nchoosek((n+m)/2,p)*...
            nchoosek((n-m)/2,q)*...
            (etaE+1)^(n-p-q)*...
            (etaE-1)^(p+q)*...
            exp(i*(2*(p-q)*thetaE));
    end
    end
end
