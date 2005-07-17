function [A,w,phi,a,b,syn] = track_sin3 (x,A,w,phi);


N=length(x);
M=length(w);
n=0:N-1;
r=x;
for k=1:M
   G(k,:)   = cos(w(k)*n+phi(k));
   G(k+M,:) = sin(w(k)*n+phi(k));
   r = r - A(k)*G(k,:);
end

%xp = cos(w.*n+phi);
%xd = sin(w.*n+phi);
%r = x-A*xp;
%G(1,:) = xp;
%G(2,:) = xd;

%na = (G(1,:)*G(1,:)');
%nb = (G(2,:)*G(2,:)');

p = G'\r';
%p= G*r';
%p(1)=p(1)/na;
%p(2)=p(2)/nb;
p1 = p(1:M)';
p2 = p(M+1:end)';

%p1=max(-.3*A,min(p1,.3*A));
a=1.5*p1/N;
%b = (p2./(.1+A+p1))*4/(2*N*N);
b = atan2(p2, A+p1)*4/(2*N*N);
%a = max(-1e-4*A,min(1e-4*A,a));
b = max(-1e-6,min(1e-6,b));

%a=a*0;
%b=b*0;
%[atan2(p2,A+p1);A+p1]

%phi = phi - atan2(p2,A+p1);
delta_phi = -atan2(p2,A+p1);

A = sqrt((A+p1).^2 + p2.^2);
%a = 1.5*(sqrt((A+p1).^2 + p2.^2)-A)/N;
b=b*0;
a=a*0;

syn=0;
for k=1:M
   syn = syn + (A(k)+a(k)*n).*cos(w(k)*n-b(k)*n.^2+phi(k) + delta_phi(k));%*min(8*n/256,1));
end

%syn = (G'*p+G(1:M,:)'*A')';
%syn = (G'*p)';

A=A+a*N;
phi = mod(phi+N*w-b*N.^2+delta_phi(k),2*pi);
w = w + .3*delta_phi(k)/N;
%w=w-.2*b*N;

