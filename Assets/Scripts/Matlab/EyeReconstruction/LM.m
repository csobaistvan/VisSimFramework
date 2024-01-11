% from: https://people.cas.uab.edu/~mosya/cl/MATLABcircle.html
function Par = LM(XY,ParIni,LambdaIni)

%--------------------------------------------------------------------------
%  
%     Geometric circle fit (minimizing orthogonal distances)
%     based on the standard Levenberg-Marquardt scheme 
%        in the full (a,b,R) parameter space
%     This is perhaps the best geometric circle fit
%
%     Input:  XY(n,2) is the array of coordinates of n points x(i)=XY(i,1), y(i)=XY(i,2)
%             ParIni = [a b R] is the initial guess (supplied by user)
%             LambdaIni is initial value for the correction factor lambda
%               (this is optional; if it is missing, LM sets it to 1)
%
%     Output: Par = [a b R] is the fitting circle:
%                           center (a,b) and radius R
%
%--------------------------------------------------------------------------

if (nargin < 3), LambdaIni = 1; end;  % if Lambda(initial) is not supplied, set it to one

epsilon=0.000001;    % tolerance (small threshold)

IterMAX = 50;      % maximal number of (main) iterations; usually 10-20 suffice 

lambda_sqrt = sqrt(LambdaIni);   %  sqrt(Lambda) is actually used by the code

Par = ParIni;     % starting with the given initial guess

[J,g,F] = CurrentIteration(Par,XY);    % compute objective function and its derivatives

for iter=1:IterMAX         %  main loop, each run is one (main) iteration

    while (1)         %  secondary loop - adjusting Lambda (no limit on cycles)

        DelPar = [J; lambda_sqrt*eye(3)]\[g; zeros(3,1)];   % step candidate
        progress = norm(DelPar)/(norm(Par)+epsilon);
        if (progress < epsilon)  break;  end;               % stopping rule
        ParTemp = Par - DelPar';
        [JTemp,gTemp,FTemp] = CurrentIteration(ParTemp,XY);  % objective function + derivatives

        if (FTemp < F && ParTemp(3)>0)        %   yes, improvement
           lambda_sqrt = lambda_sqrt/2;   % reduce lambda, move to next iteration
           break;
        else                            %   no improvement
           lambda_sqrt = lambda_sqrt*2; % increase lambda, recompute the step
           continue;
        end
    end   %   while (1), the end of the secondary loop
%    fprintf(1,'   %d     %.8f   %.8f   %.8f\n',iter,Par);
    if (progress < epsilon)  break;  end;             % stopping rule
    Par = ParTemp;  J = JTemp;  g = gTemp;  F = FTemp;  % update the iteration
end    %  the end of the main loop (over iterations)
end    %    LM

%================ function CurrentIteration ================

function [J,g,F] = CurrentIteration(Par,XY)

%    computes the objective function F and its derivatives at the current point Par

Dx = XY(:,1) - Par(1);
Dy = XY(:,2) - Par(2);
D = sqrt(Dx.*Dx + Dy.*Dy);
J = [-Dx./D, -Dy./D,  -ones(size(XY,1),1)];
g = D - Par(3);
F = norm(g)^2;

end  %  CurrentIteration