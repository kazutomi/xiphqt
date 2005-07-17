function R = ESPRIT(a, N, M);
M = 2*M;
%Compute cross-correlation for signal (well, it's almost an auto-correlation)
for j=1:N
   for k=1:N
      A(j,k)=sum(a(j:end-N+j).*a(k:end-N+k));
   end
end
%Eigendecomposition
[V,D]=eig(A);
%Signal subspace
Eu = V(2:end,N-M+1:end);
Ed = V(1:end-1,N-M+1:end);

PHI = inv(Eu'*Eu)*Eu'*Ed;

%Get the frequencies
[V2,D2]=eig(PHI);
R=D2;
