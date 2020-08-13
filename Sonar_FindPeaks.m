# Read wav file of sonar returns and find peak values
# GNU Octave script by J.Beale 12-Aug-2020

pkg load signal
soundSpd = 341;  # m/s in air at 15 C
fpm = 3.28084;   # feet per meter

#[y, fs] = audioread('20200812_141432_Pipe8_clip1.wav');
#[y, fs] = audioread('20200812_141432_Pipe8_clip2.wav');
[y, fs] = audioread('20200812_141432_Pipe8_clip3.wav');
x=1:size(y)(1);

#[pks idx] = findpeaks(y,"DoubleSided","MinPeakHeight",0.03,"MinPeakDistance",2000);
# plot(x,y,x(idx),y(idx),'xm');

env=abs(hilbert(y));  # find envelope of signal
# plot(x,y,'g;raw;',x,env,'b;envelope;'); # show signal and envelope

# maxima of envelope in sonar returns = reflecting object
[pks idx] = findpeaks(env,"MinPeakHeight",0.1,"MinPeakDistance",2000); 
pcount = size(pks)(1);

plot(x,env,x(idx),env(idx),'xm'); # display envelope with peaks marked
printf("Found %d peaks.\n", pcount);

objnum = 1;
Tidx = 1;
# step through each set of returns, starting with Tx pulse
do 
# refine position of first peak (could avoid this, with original Tx sig)
  p0=idx(Tidx);     # initially found first peak (Tx peak)
  env1 = env(p0-200:p0+200);
  [pk2 idx2] = findpeaks(env1,"MinPeakHeight",0.5);
  p1 = idx2(1)-201+p0;          # new refined initial peak index
  r1 = idx(Tidx+1)- p1;  # samples to first reflection
  r2 = idx(Tidx+2)- p1;  # samples to 2nd reflection

  t1 = r1/fs;  # round-trip time to reflection #1
  t2 = r2/fs;

  d1 = (soundSpd * t1)/2; # distance in meters
  d2 = (soundSpd * t2)/2;

  ft1 = d1 * fpm;  # distance in feet
  ft2 = d2 * fpm;
  printf("%03d : ",objnum);
  printf("D1: %5.3f m ", d1);
  printf("D2: %5.3f m  ", d2);
  printf("D1: %5.3f ft  ", ft1);
  printf("D2: %5.3f ft", ft2);
  printf("\n");
  objnum += 1;
  Tidx += 3;
until (Tidx > pcount-2)

# =============
#{
>> run('Sonar_FindPeaks')
Found 48 peaks.
001 : D1: 3.128 m D2: 5.486 m  D1: 10.264 ft  D2: 17.999 ft
002 : D1: 3.128 m D2: 5.492 m  D1: 10.261 ft  D2: 18.017 ft
003 : D1: 3.125 m D2: 5.485 m  D1: 10.252 ft  D2: 17.996 ft
004 : D1: 3.125 m D2: 5.487 m  D1: 10.252 ft  D2: 18.002 ft
005 : D1: 3.126 m D2: 5.490 m  D1: 10.255 ft  D2: 18.011 ft
006 : D1: 3.124 m D2: 5.481 m  D1: 10.250 ft  D2: 17.982 ft
007 : D1: 3.123 m D2: 5.493 m  D1: 10.247 ft  D2: 18.023 ft
008 : D1: 3.124 m D2: 5.483 m  D1: 10.250 ft  D2: 17.988 ft
009 : D1: 3.126 m D2: 5.486 m  D1: 10.255 ft  D2: 17.999 ft
010 : D1: 3.128 m D2: 5.484 m  D1: 10.261 ft  D2: 17.993 ft
011 : D1: 3.126 m D2: 5.487 m  D1: 10.255 ft  D2: 18.002 ft
012 : D1: 3.126 m D2: 5.485 m  D1: 10.255 ft  D2: 17.996 ft
013 : D1: 3.126 m D2: 5.486 m  D1: 10.255 ft  D2: 17.999 ft
014 : D1: 3.126 m D2: 5.490 m  D1: 10.255 ft  D2: 18.011 ft
015 : D1: 3.124 m D2: 5.492 m  D1: 10.250 ft  D2: 18.020 ft
016 : D1: 3.125 m D2: 5.484 m  D1: 10.252 ft  D2: 17.991 ft
#}
