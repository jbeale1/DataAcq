# Read wav file with set of sonar returns. Find distances to objects.
# GNU Octave script by J.Beale 12-Aug-2020

pkg load signal          # findpeaks()
pkg load miscellaneous   # clip()

soundSpd = 341;  # m/s in air at 15 C
fpm = 3.28084;   # feet per meter

#infile = '20200811_184702_Pipe7_R192_crop1.wav';
#infile = '20200811_184702_Pipe7_R192.wav';
#infile = '20200812_141432_Pipe8_clip1.wav';
#infile = '20200812_141432_Pipe8_clip2.wav';
#infile = '20200812_141432_Pipe8_clip3.wav';
infile = '20200812_141432_Pipe8_clip4.wav';

printf("\n");
printf("File: %s\n",infile);
more off;   # turn off Octave command window paginator

[y, fs] = audioread(infile);  # load in data from wav file
x=1:size(y)(1);

env=abs(hilbert(y));  # find envelope of signal
# plot(x,clip(y),'g;raw;',x,env,'b;envelope;'); # show pos.half of signal w/envelope

# maxima of envelope in sonar returns = reflecting object
[pks idx] = findpeaks(env,"MinPeakHeight",0.1,"MinPeakDistance",2000); 
pcount = size(pks)(1);
printf("Found %d peaks.\n", pcount);

pulseNum = 1;
Tidx = 1;
pkSearch = 200;  # half of window size within to search real peak

# step through each set of returns, starting with Tx pulse
do 
      # refine position of first peak (could avoid this, with original Tx sig)
  p0=idx(Tidx);     # 1st peak in envelop (Tx signal)
  y1 = clip(-y(p0-pkSearch:p0+pkSearch)); # pos. part of signal near peak 1
  [pk2 idx2] = findpeaks(y1,"MinPeakHeight",0.1);
  p1 = idx2(1)-(pkSearch+1)+p0;          # new refined peak 1 index
  idx(Tidx) = p1;        # update original vector with refined value

  p0=idx(Tidx+1);   # 2nd envelope peak
  y1 = clip(-y(p0-pkSearch:p0+pkSearch)); # neg. part of signal near peak 2 (1st reflection)
  [pk2 idx2] = findpeaks(y1,"MinPeakHeight",0.026);
  p2 = idx2(1)-(pkSearch+1)+p0;          # new refined peak 2 index
  idx(Tidx+1) = p2;      # update with new refined value

  p0=idx(Tidx+2);   # 3rd envelope peak
  y1 = clip(+y(p0-pkSearch:p0+pkSearch)); # pos. part of signal near peak 3 (2nd reflection)
  [pk2 idx2] = findpeaks(y1,"MinPeakHeight",0.026);
  p3 = idx2(1)-(pkSearch+1)+p0;          # new refined peak 3 index
  idx(Tidx+2) = p3;      # update with new refined value


  r1 = p2 - p1;    # count samples to first reflection
  r2 = p3 - p1;    # count samples to 2nd reflection

  t1 = r1/fs;  # round-trip time to reflection #1
  t2 = r2/fs;

  d1 = (soundSpd * t1)/2; # distance in meters
  d2 = (soundSpd * t2)/2;
  dm(pulseNum,1,1) = d1;      # save in distance matrix dm(pulseNum, objectNum)
  dm(pulseNum,2,1) = d2;
  dm(pulseNum,1,2) = y(p2);   # signal amplitude at chosen peak
  dm(pulseNum,2,2) = y(p3);   # signal amplitude at chosen peak

  ft1 = d1 * fpm;  # distance in feet
  ft2 = d2 * fpm;
  printf("%03d : ",pulseNum);
  printf("D1: %5.3f m ", d1);
  printf("D2: %5.3f m  ", d2);
  printf("D1: %5.3f ft  ", ft1);
  printf("D2: %5.3f ft", ft2);
  printf(" %5.3f  %5.3f", y(p2), y(p3));
  printf("\n");

  pulseNum += 1;
  Tidx += 3;
until (Tidx > pcount-2)  # | (pulseNum> 84)

printf("------------------------------------\n");
printf("File: %s  %d pulses\n", infile, size(dm)(1) );
printf("avg (std): D1: %5.3f (%4.4f) m  D2: %5.3f (%4.4f) m  (%4.4f) (%4.4f)\n", 
  mean(dm(:,1,1)), std(dm(:,1,1)), mean(dm(:,2,1)), std(dm(:,2,1)), 
  std(dm(:,1,2)), std(dm(:,2,2)) );

plot(x,y,x(idx),y(idx),'xm'); # display envelope with peaks marked

# ==========================================================

#{
File: 20200811_184702_Pipe7_R192.wav  137 pulses
avg (std): D1: 3.100 (0.0015) m  D2: 5.427 (0.0041) m

File: 20200812_141432_Pipe8_clip1.wav  15 pulses
avg (std): D1: 3.126 (0.0015) m  D2: 5.487 (0.0035) m

File: 20200812_141432_Pipe8_clip4.wav  84 pulses
avg (std): D1: 3.125 (0.0012) m  D2: 5.487 (0.0049) m
#}
