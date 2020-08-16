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

doGraph = false;  # should we show a graph of results
verbose = false;  # verbose output for each return

# -------------------------------------------
function plotPulses(dm)
 pulses = size(dm)(1);          # how many total Tx pulses processed

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
endfunction
# ===========================================================

infile = '20200815_144503.wav';  # load this file

arg_list = argv ();  # command-line inputs to this function
infile = arg_list{1};

if verbose
  printf("\n");
  printf("File: %s  ",infile);
endif
more off;   # turn off Octave command window paginator
# hold off;   # permit erasing of any previous plots

# get file and trim first,last 0.5 sec (glitch)
[yraw, fs] = audioread(infile);
y = yraw(fs/2:end-int32(fs/2));

file_dur = size(y)(1) / fs;   # file duration in seconds

if verbose
  printf("%d Hz  %5.3f sec\n", fs,file_dur);
endif
x=1:size(y)(1);

pulseNum = 1;
Tidx = 1;
eThresh = 0.05;                # minimum acceptable peak amplitude
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
  # causes: warning: matrix singular to machine precision, rcond = 2.04929e-20
[pks idx] = findpeaks(env,"MinPeakHeight",eThresh,"MinPeakDistance",MPD);

pcount = size(pks)(1);
if verbose
  printf("Found %d peaks.\n", pcount);
endif
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

#  plot(x,env,x(idx),env(idx),'xm'); # show all envelopes in file, with leading edges marked

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
  if (verbose)
   printf("%03d : ",pulseNum);
   printf("D1: %5.3f m ", d1);
   printf("D2: %5.3f m  ", d2) ;
   printf("D1: %5.3f ft  ", ft1);
   printf("D2: %5.3f ft", ft2);
   printf(" %5.3f  %5.3f", env(int32(p2)), env(int32(p3)));
   printf("\n");
  endif

  pulseNum += 1;
  Tidx += 3;
until (Tidx > pcount-2)  # | (pulseNum> 84)

#yAccum = yAccum ./ pulseNum;  # scale sum to form average
# plot(1:wSize+1,yAccum);       # show averaged waveform

pulses = size(dm)(1);          # how many total Tx pulses processed
D1Mean = mean(dm(:,1,1));
D2Mean = mean(dm(:,2,1));
D1Real = 3.062;  # actual meters to 1st coupler (length of pipe segment 1)
cfactor = D1Real / D1Mean;    # length correction factor based on known distance
# cfactor > 1 means apparent D1 is too small => apparent sound speed faster

if verbose
  printf("------------------------------------\n");
endif
printf("%s  ", infile);
printf(" %d kHz  %5.3f sec  %d pulses ", int32(fs/1000),file_dur,pulses);
printf("D1: %5.3f (%4.4f) m  D2: %5.3f (%4.4f) m  %5.3f m/s D2a: %6.4f m\n",
  D1Mean, std(dm(:,1,1)), D2Mean, std(dm(:,2,1)),
  cfactor*soundSpd, cfactor*D2Mean);

if (pulses > 1) & doGraph
  plotPulses(dm);
endif

# ===============================================================
#{
Real length of 1st pipe: 120.5" = 3.061 m (old cplr):+2 cm (new cplr):+0.1 cm

( rp50 + 96kHz ADC )
20200815_132003.wav   96 kHz  10.000 sec  40 pulses 339.0 m/s
avg (std): D1: 3.035 (0.0009) m  D2: 5.500 (0.0004) m  (1.0090) D2a: 5.549 m
20200815_133855.wav   96 kHz  10.000 sec  40 pulses 339.0 m/s
avg (std): D1: 3.035 (0.0009) m  D2: 5.501 (0.0007) m  (1.0089) D2a: 5.550 m
20200815_134003.wav   96 kHz  10.000 sec  40 pulses 339.0 m/s
avg (std): D1: 3.035 (0.0007) m  D2: 5.501 (0.0006) m  (1.0090) D2a: 5.550 m
20200815_135504.wav   96 kHz  10.000 sec  40 pulses 339.0 m/s
avg (std): D1: 3.035 (0.0009) m  D2: 5.501 (0.0008) m  (1.0090) D2a: 5.550 m
20200815_143503.wav   96 kHz  10.000 sec  47 pulses 339.0 m/s
avg (std): D1: 3.035 (0.0007) m  D2: 5.501 (0.0005) m  (1.0089) D2a: 5.5504 m

#}
