import pylab as p
import numpy

def gogo(title,wfun):
  N=64
  dr=-140.
  no=8*N/2.
  no2=8*N/2.
  w=wfun(N)
  w = w/numpy.max(w)
  H = abs(numpy.fft.fft(numpy.append(w,numpy.zeros((1,7*N)))));
  H = numpy.fft.fftshift(H);
  mH=numpy.max(H)
  H = [x/mH for x in H]
  H = 20.*numpy.log10(H);
  l=len(numpy.arange(-no+1,no+1))
  x=numpy.arange(-no+1,no+1)/l
  fig=p.figure()
  ax1 = p.subplot(111)
  p.title(title+' window')
  p.plot(x,H, 'b', lw=1)
  ax1.set_ylim(dr,0)
  ax1.set_xlim(-0.5,0.5)

  w=wfun(8*N)
  l2=len(numpy.arange(-no2,no2))
  x2=numpy.arange(-no2,no2)/l2
  p.ylabel("dB")
  ax2 = p.twinx()
  p.plot(x2,w, 'r', lw=2)
  ax2.set_xlim(-0.5,0.5)
  ax2.set_ylim(0,1.)
  #p.show()
  fig.set_size_inches( (8,6) )
  p.savefig(title.replace(' ','_').lower()+'.png',dpi=100)

def wp(i,n):
  return (i-n/2.)/(n/2.)
  
def hanning(n):
  scale=2*numpy.pi/n;
  return [(.5-.5*numpy.cos(scale*i)) for i in range(n)]


def tgauss_deep(n):
  TGA=1.e-4
  TGB=21.6
  return [numpy.exp(-TGB*numpy.power(wp(i,n),2)) *
    numpy.power(1.-numpy.abs(wp(i,n)),TGA) for i in range(n)]

def maxwell1(n):
  scale=2*numpy.pi/n;
  return [ numpy.power(119.72981 -
    119.24098*numpy.cos(scale*i) +
    0.10283622*numpy.cos(2*scale*i) +
    0.044013144*numpy.cos(3*scale*i) +
    0.97203713*numpy.cos(4*scale*i)
    ,1.9730763)/50000. for i in range(n)]

def rect(n):
  return [1.0]*n

def sine(n):
  scale=numpy.pi/n;
  return [numpy.sin(scale*i) for i in range(n)]

gogo('Hanning',hanning)
gogo('Triangular gaussian unimodal',tgauss_deep)
gogo('Rectangular',rect)
gogo('Maxwell',maxwell1)
gogo('Sine',sine)

