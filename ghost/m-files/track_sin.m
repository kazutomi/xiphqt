function [A,w,phi,a,b,syn] = track_sin (x,A,w,phi,noise);

N=length(x);
n=0:N-1;
xp = A*cos(w.*n+phi);
r = x-xp;
plot (r-noise);
G(1,:) = xp.*n;
G(2,:) = n.*n.*sin(w.*n+phi);
G(3,:) = cos(w.*n+phi);
%G(4,:) = sin(w.*n+phi);

%na = sqrt(G(1,:)*G(1,:)');
%nb = sqrt(G(2,:)*G(2,:)');
%G(1,:) = G(1,:)/sqrt(G(1,:)*G(1,:)');
%G(2,:) = G(2,:)/sqrt(G(2,:)*G(2,:)');

p = G'\r';
a=p(1);
b=p(2);
AA=p(3);
%a = a + AA/(1.5*N);
b=max(-2e-6,min(2e-6,b));
a=max(-3e-3*A,min(3e-3*A,a));
syn = (A+a*n).*cos(w.*n-b*n.^2+phi);
A=A*(1+a*(N-1));
phi = mod(phi+N*w-b*N.^2,2*pi);
w=w-2*b*N;
%syn = xp + a*G(1,:) + b*G(2,:);
%syn = xp + (G'*(G'\r'))';



