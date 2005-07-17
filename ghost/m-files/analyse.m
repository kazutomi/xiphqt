A=B;w=ww;phi=p;
[A,w,phi,a,b,syn1] = track_sin3b(t(1:256)',A,w,phi);
%[A,w,phi,a,b,syn2] = track_sin3(t(257:512)',A,w,phi);
%[A,w,phi,a,b,syn3] = track_sin3(t(513:768)',A,w,phi);
%[A,w,phi,a,b,syn4] = track_sin3(t(769:1024)',A,w,phi);
%syn=[syn1,syn2,syn3,syn4];

syn=syn1;
for k=1:10
   [A,w,phi,a,b,synn] = track_sin3b(t(1+k*256:256*k+256)',A,w,phi);
   syn=[syn,synn];
end
