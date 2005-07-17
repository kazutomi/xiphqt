function [A,w,phi,a,b,syn] = track_sin2 (x,A,w,phi);

N=length(x);
n=0:N-1;
xp = cos(w.*n+phi);
xd = sin(w.*n+phi);
r = x-A*xp;
G(1,:) = xp;
G(2,:) = xd;

%na = (G(1,:)*G(1,:)');
%nb = (G(2,:)*G(2,:)');

p = G'\r';
%p= G*r';
%p(1)=p(1)/na;
%p(2)=p(2)/nb;

a=1.5*p(1)/N;
b = (p(2)/(1+p(1)))*3/(2*N*N);
a = max(-1e-3*A,min(1e-3*A,a));
b = max(-1e-5,min(1e-5,b));

syn = (A+a*n).*cos(w.*n-b*n.^2+phi);
A=A+a*N;
phi = mod(phi+N*w-b*N.^2,2*pi);
w=w-2*b*N;




