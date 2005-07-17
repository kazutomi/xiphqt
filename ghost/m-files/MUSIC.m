function [R,d] = MUSIC(a, N, M);
M = 2*M;
for j=1:N
   for k=1:N
      A(j,k)=sum(a(j:end-N+j).*a(k:end-N+k));
   end
end
[V,D]=eig(A);
G=V(:,1:N-M)*V(:,1:N-M)';
r=zeros(2*N-1,1);
for j=1:N
   for k=1:N
      r(j-k+N) = r(j-k+N)+G(j,k);
   end
end
R = roots(r);
d = diag(D);
