# Read wav file with set of sonar returns. Find distances to objects.
# GNU Octave script v.04 by J.Beale 14-Aug-2020

clear;  # forget variables from any previous run

pkg load signal          # findpeaks()
pkg load miscellaneous   # clip()

#soundSpd = 343.15;  # sound speed, m/s in air at 20 C
#soundSpd = 340.2;   # sound speed, m/s in air at 15 C
soundSpd = 339.02;  # sound speed, m/s in air at 13 C
#soundSpd = 337.24;  # sound speed, m/s in air at 10 C

fpm = 3.28084;   # feet per meter

#infile = '20200811_184702_Pipe7_R192_crop1.wav';
#infile = '20200811_184702_Pipe7_R192.wav';
#infile = '20200812_141432_Pipe8_clip1.wav';
#infile = '20200812_141432_Pipe8_clip2.wav';
#infile = '20200812_141432_Pipe8_clip3.wav';
#infile = '20200812_141432_Pipe8_clip4.wav';
#infile = '20200813_090232_Pipe9_R192.wav';
#infile = '20200813_090232_Pipe9_R192_crop1.wav';
#infile = '20200814_102927_Pipe10_192.wav';
#infile = '20200814_111850_Pipe11_192.wav';
#infile = '20200814_144418_Pipe12_192.wav';
#infile = '20200814_151444_Pipe13_192.wav'; # new smaller coupler
#infile = '20200814_151444_Pipe13_192_b.wav';
#infile = '20200814_175133_Pipe15_192.wav';
#infile = '20200814_175255_Pipe16_192.wav';
#infile = '20200814_193036_Pipe17_192.wav';
#infile = '20200814_175255_Pipe16.wav';
infile = '20200814_203445.wav';


printf("\n");
printf("File: %s  ",infile);
more off;   # turn off Octave command window paginator
# hold off;   # permit erasing of any previous plots

[y, fs] = audioread(infile);  # load in data from wav file
file_dur = size(y)(1) / fs;   # file duration in seconds

printf("%d Hz  %5.3f sec\n", fs,file_dur);
x=1:size(y)(1);

pulseNum = 1;
Tidx = 1;
pkSearchT = 1.0;        # half of window time (msec) within to search real peak
wOffT = -1.0;           # offset (msec) from p1 moment on Tx waveform
wSizeT = 36.0;          # window (msec) containing all data from this pulse
mpdT = 10.0;            # event separation (msec) between peaks

pkSearch = (pkSearchT/1000) * fs;   # samples in half window
wOff = (wOffT/1000) * fs;           # offset from p1 point on Tx waveform
wSize = (wSizeT/1000) * fs;         # size of window containing all data from this pulse
MPD = (mpdT/1000) * fs;             # minimum peak distance in findpeaks() 

yAccum = zeros(wSize+1,1);  # initialize averaged-pulse data accumulator
tScaleFac = 0.1;            # pulse-start threshold as fraction of peak value 


env=abs(hilbert(y));  # find envelope of signal
# plot(x,abs(y),'g;raw;',x,env,'b;envelope;'); # show rectified signal w/envelope

# maxima of envelope in sonar returns = reflecting object
[pks idx] = findpeaks(env,"MinPeakHeight",0.1,"MinPeakDistance",MPD); 
pcount = size(pks)(1);
printf("Found %d peaks.\n", pcount);
# plot(x,env,x(idx),env(idx),'xm'); # display envelope with peaks marked


# step through each set of returns, starting with Tx pulse
do 
      # refine position of first peak (could avoid this, with original Tx sig)
  p01=idx(Tidx);     # 1st peak in envelope (Tx signal)
  p02=idx(Tidx+1);
  p03=idx(Tidx+2);

  iStart = p01-pkSearch;
  envWin = env(iStart:p01);  # window containing beginning of this envelope peak
  p1Thresh = env(p01)*tScaleFac;
  p1 = find(envWin < p1Thresh)(end);  # last index to be below threshold
  p1 += (iStart - 1);
         # linear interpolation between sample points on either side of threshold
  delta1 = env(p1+1) - env(p1);
  delta2 = p1Thresh - env(p1);
  f1 = delta2/delta1;

  iStart = p02-pkSearch;
  envWin = env(iStart:p02);  # window containing beginning of this envelope peak
  p2Thresh = env(p02)*tScaleFac;
  p2 = find(envWin < p2Thresh)(end);  # last index to be below threshold
  p2 += (iStart - 1);
         # linear interpolation between sample points on either side of threshold
  delta1 = env(p2+1) - env(p2);
  delta2 = p2Thresh - env(p2);
  f2 = delta2/delta1;

  iStart = p03-pkSearch;
  envWin = env(iStart:p03);  # window containing beginning of this envelope peak
  p3Thresh = env(p03)*tScaleFac;
  p3 = find(envWin < p3Thresh)(end);  ##last index to be below threshold
  p3 += (iStart - 1);
         # linear interpolation between sample points on either side of threshold
  delta1 = env(p3+1) - env(p3);
  delta2 = p3Thresh - env(p3);
  f3 = delta2/delta1;

  idx(Tidx)=p1;    # update "peak" indexes to "start" indexes
  idx(Tidx+1)=p2;
  idx(Tidx+2)=p3;

  plot(x,env,x(idx),env(idx),'xm'); # show all envelopes in file, with leading edges marked

  yWin = y(p1+wOff:p1+wOff+wSize);  # window containing this pulse and 2 reflections
  # plot(1:wSize+1,yWin);  # show this particular waveform
  # hold on;               # overlay next plot
  #yAccum = yAccum .+ yWin;

  r1 = (p2+f2) - (p1+f1);    # count samples to first reflection
  r2 = (p3+f3) - (p1+f1);    # count samples to 2nd reflection

  t1 = r1/fs;  # round-trip time to reflection #1
  t2 = r2/fs;

  d1 = (soundSpd * t1)/2; # one-way distance, in meters
  d2 = (soundSpd * t2)/2;
  dm(pulseNum,1,1) = d1;      # save in distance matrix dm(pulseNum, objectNum)
  dm(pulseNum,2,1) = d2;
  dm(pulseNum,1,2) = env(p2);   # signal amplitude at chosen peak
  dm(pulseNum,2,2) = env(p3);   # signal amplitude at chosen peak

  ft1 = d1 * fpm;  # distance in feet
  ft2 = d2 * fpm;
  printf("%03d : ",pulseNum);
  printf("D1: %5.3f m ", d1);
  printf("D2: %5.3f m  ", d2);
  printf("D1: %5.3f ft  ", ft1);
  printf("D2: %5.3f ft", ft2);
  printf(" %5.3f  %5.3f", env(int32(p2)), env(int32(p3)));
  printf("\n");

  pulseNum += 1;
  Tidx += 3;
until (Tidx > pcount-2)  # | (pulseNum> 84)

#yAccum = yAccum ./ pulseNum;  # scale sum to form average
# plot(1:wSize+1,yAccum);       # show averaged waveform

pulses = size(dm)(1);          # how many total Tx pulses processed
D1Mean = mean(dm(:,1,1));
D2Mean = mean(dm(:,2,1));
D1Real = 3.062;  # measured distance to 1st coupler
cfactor = D1Real / D1Mean;    # length correction factor based on known distance

printf("------------------------------------\n");
printf("%s  ", infile);
printf(" %d kHz  %5.3f sec  %d pulses %4.1f m/s\n", int32(fs/1000),file_dur,pulses,soundSpd);
printf("avg (std): D1: %5.3f (%4.4f) m  D2: %5.3f (%4.4f) m  (%5.4f) D2a: %5.3f m\n", 
  D1Mean, std(dm(:,1,1)), D2Mean, std(dm(:,2,1)), 
  cfactor, D2Mean*cfactor);

# -------------------------------------------
if (pulses > 1)
 clf;
 subplot(2,1,1);
 plot(dm(:,1,1));  # show each D1 range
 axis([1 pulses]);
 grid on;           # easier to see trends
 title("Coupler Distance");
 ylabel ("meters");
 subplot(2,1,2);
 plot(1:pulses, dm(:,2,1));  # show each D2 range  
 axis([1 pulses]);
 grid on;
 title("Water Level");
 xlabel ("pulse #");
 ylabel ("meters");
endif
# ==========================================================

#{
Actual length of 1st pipe: 120.5" = 3.061 m  (old cplr): +2 cm (new cplr): +0.1 cm

v.04
20200812_141432_Pipe8_clip4.wav   192 kHz  24.839 sec  217 pulses 339.0 m/s
avg (std): D1: 3.025 (0.0088) m  D2: 5.393 (0.0321) m  (0.0049) (0.0036)

20200813_090232_Pipe9_R192.wav   192 kHz  44.121 sec  217 pulses 339.0 m/s
avg (std): D1: 3.035 (0.0006) m  D2: 5.429 (0.0005) m  (0.0008) (0.0010)

20200814_102927_Pipe10_192.wav   192 kHz  25.538 sec  126 pulses 339.0 m/s
avg (std): D1: 3.031 (0.0007) m  D2: 5.449 (0.0007) m  (0.0010) (0.0013)

20200814_111850_Pipe11_192.wav   192 kHz  20.833 sec  217 pulses 339.0 m/s
avg (std): D1: 3.035 (0.0010) m  D2: 5.438 (0.0095) m  (0.0032) (0.0040)

20200814_144418_Pipe12_192.wav   192 kHz  22.708 sec  112 pulses 339.0 m/s
avg (std): D1: 3.038 (0.0006) m  D2: 5.455 (0.0006) m  (0.0011) (0.0010)

(new coupler)
20200814_151444_Pipe13_192_b.wav   192 kHz  16.173 sec  80 pulses 339.0 m/s
avg (std): D1: 3.016 (0.0004) m  D2: 5.433 (0.0005) m  (1.0153) D2a: 5.516 m

(after air pumped at ~ 10 feet)
20200814_175133_Pipe15_192.wav   192 kHz  40.625 sec  201 pulses 339.0 m/s
avg (std): D1: 3.056 (0.0006) m  D2: 5.490 (0.0011) m  (1.0018) D2a: 5.500 m

20200814_175255_Pipe16_192.wav   192 kHz  29.375 sec  145 pulses 339.0 m/s
avg (std): D1: 3.057 (0.0004) m  D2: 5.491 (0.0005) m  (1.0017) D2a: 5.500 m

20200814_175255_Pipe16.wav   48 kHz  29.375 sec  145 pulses 339.0 m/s
avg (std): D1: 3.057 (0.0003) m  D2: 5.491 (0.0004) m  (1.0017) D2a: 5.500 m

20200814_193036_Pipe17.wav   48 kHz  24.583 sec  121 pulses 339.0 m/s
avg (std): D1: 3.053 (0.0018) m  D2: 5.498 (0.0009) m  (1.0030) D2a: 5.515 m

20200814_193036_Pipe17_192.wav   192 kHz  24.456 sec  121 pulses 339.0 m/s
avg (std): D1: 3.052 (0.0006) m  D2: 5.498 (0.0006) m  (1.0032) D2a: 5.516 m

20200814_203445.wav   48 kHz  33.125 sec  164 pulses 339.0 m/s
avg (std): D1: 3.050 (0.0006) m  D2: 5.502 (0.0005) m  (1.0040) D2a: 5.524 m

#}
