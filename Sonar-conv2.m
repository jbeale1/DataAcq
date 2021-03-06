# Sonar Ranging calculation: find time & distance from Tx pulse to Rx echo
# v0.61 J.Beale 24-AUG-2020

pkg load signal                  # for xcorr, resample

# -----------------------------------------------------------------------------

p.tOff00 = 0.05;           # seconds: initial ROI offset from start of file
p.tOff0 = 0.05;            # seconds: current ROI offset from start of file
p.tW = 0.120;              # seconds: initial ROI window size
p.tWs = 0.203;             # seconds: step interval between pulses

p.pThresh = 0.3;           # 1st sample above this level is Tx pulse start
p.pThresh2 = 0.55;         # 2ndary corr. peaks of initial pulse can reach 0.4
p.minPkDist = .002;        # minimum allowed time between peaks (s)

p.msOff1 = 0.37;           # (msec) pulse window start before pulse edge
p.msOff2 = 0.55;           # (msec) pulse window length (1.7 ms best for pk.2)
p.holdoff = 16;            # (msec) start looking for echoes this long after Tx
p.twi2 = 0.002;            # target Rx window width in seconds

# 18..20 msec: location of pipe joint at 10'0.5" (3.061 m) (RTT: 18.03 ms)

p.wOff1 = 3.5;                     # (msec) full signal: start before Tx
p.wLen = 42 ;                      # (msec) full signal: window size

p.ambV = 346.06;           # sound speed (m/s) in ambient at 77 F / 25 C
p.speed = 339.6;           # sound speed (m/s) at 14 C  (339.02 @ 13 C)
#p.p=100;                  # p/q is oversample ratio
p.p=87;                    # p/q is oversample ratio
p.q=1;                     # denominator of oversample ratio
p.factor = 1.0;            # scaling factor for pulse in correlation

# ============================================================================
# ----------------------------------------------------------------------------
# findOffset()
# find distance (m) and correlation (0..1: quality value) between peaks
# in each channel of input vector v

function [r] = findOff(v, fs, p)

  r.dist=0;  # Tx-Rx distance in m
  r.hRet=0;  # correlation amplitude
  r.max=0;   # signal amplitude (0..1)

                              # ROI_0 is one TxRx segment of entire train
  ioff = int32(fs * p.tOff0); # ROI_0 initial offset into file
  iwide = int32(fs * p.tW);   # ROI_0 width of window
  iend = ioff+iwide;
  if (iend > length(v))
    return
  endif

  rfac = int32(p.p/p.q);                 # integer resample factor
  fsr = fs * (p.p/p.q);                  # sample rate of resampled waveform

  #yr0 = v(ioff:iend,:);    # ROI_0
  yrB = resample(v(ioff:iend,:),p.p,p.q);  # resample(v,p,q) v at new rate p/q
  yr0 = yrB(:,1);        # ROI_0 ch.1 : Tx sig
  [pk, ipk] = max(yr0(:,1));  # find peak, which is top of this Tx pulse
  r.yr0 = yr0;
  r.max = pk;            # peak +amplitude within ROI
  r.ipk = ipk;           # index of peak
# absolute peak position in v: ipk + int32(fsr * p.tOff0)

  # y will contain ROI_1 : a tight window on Tx pulse only
  iwoff1 = int32(p.wOff1 / 1E3 * fsr);       # start of ROI_1 pulse window
  iwLen = int32(p.wLen / 1E3 * fsr);         # end of ROI_1 pulse window
  #iy1 = ipk-iwoff1;
  iy1 = ipk-iwoff1;
  iy2 = iy1+iwLen;

  p1= yr0(iy1:iy2,1);            # y now contains just Tx pulse
  p1= p1 ./ pk;                 # normalize to +pk amplitude

  r.p1 = p1;   # DEBUG return this to plot later

  yi1 = iy1 - iwLen;
  yi2 = yi1 + 3*iwLen;
  y = yrB(yi1:yi2,2);    # window on second channel to search for Tx signal
#  y = yrB(yi1:yi2,1);    # DEBUG : test window on same channel

  winOffset = iwLen;  # lag offset for same-signal correlation

  [R,lag] = xcorr(y, p1 .* p.factor);  # cross-correlation finds best match
  [xpk, xipk] = max(R);  # peak position shows Ch.1-Ch.2 lag

  delta = winOffset - lag(xipk);  # (samples) Tx offset between channels
  delay = double(delta) / fsr;  # delay in seconds
  r.dist = p.ambV * delay;
  r.hRet = xpk / rfac;           # value of correlation pk
  # r.R = R;
  # r.lag = lag;

endfunction


#error: yr0(12027): out of bound 11521
#error: called from
#    findDist at line 129 column 4
#    conv1 at line 210 column 6


# -----------------------------------------------------------------------------
# findDist()
# find distance (m) and correlation (0..1: quality value) to largest reflection
# Start looking for Tx,Rx signals at offset time p.tOff0 into array w

function [r] = findDist(w, fs, p)

  r.dist=0;  # Tx-Rx distance in m
  r.hRet=0;  # correlation amplitude
  r.max=0;   # signal amplitude (0..1)

                              # ROI_0 is one TxRx segment of entire train
  ioff = int32(fs * p.tOff0); # ROI_0 initial offset into file
  iwide = int32(fs * p.tW);   # ROI_0 width of window
  iend = ioff+iwide;
  if (iend > length(w))
    return
  endif

  yr0 = w(ioff:iend);  # ROI_0 containing one signal + return set
  [pk, ipk] = max(yr0);  # find peak, which is top of Tx pulse
  r.max = pk;            # peak +amplitude within ROI

  #r.yr0 = yr0;    # DEBUG

                                            # ROI_1 is tight window on TxRx
  iwoff1 = int32(p.wOff1 / 1E3 * fs);       # start of ROI_1 pulse window
  iwLen = int32(p.wLen / 1E3 * fs);         # end of ROI_1 pulse window
  iy1 = ipk-iwoff1;
  iy2 = iy1+iwLen;
  if (iy1 < 1) || (iy2 > length(yr0))    # not the signal we expected, apparently
    return
  endif

  y=(yr0(iy1:iy2));                   # ROI_1 containing pulse + echo #1
  y= y ./ pk;                               # normalize to +pk amplitude
 # ----------------------------------------------------
  #plot(y,'-x');                    # have a look

  ioff1 = int32((p.msOff1 / 1E3) * fs);         # start of pulse window
  ioff2 = int32((p.msOff2 / 1E3) * fs);         # width of pulse window

  ip1 = ipk-(iy1+ioff1);
  p1=(y(ip1:ip1+ioff2));            # HERE: contains ~3 cycles of Tx pulse

  [R,lag] = xcorr(y, p1 .* p.factor);           # do the cross-correlation

  Rn = max(0,R ./ max(R));	 # normalize for peak=1.0, clamp neg to 0
  #plot(lag,Rn);
  #r.Rn = Rn;         # DEBUG

  [pks idx] = findpeaks(Rn,"MinPeakHeight",p.pThresh2,"MinPeakDistance",p.minPkDist*fs);
             # idx(1) = initial pulse, idx(2) = end refl. idx(3) = back reflection

  [w, iPk] = max(Rn);     # iPk is index of Tx peak
  iWin = iPk + ((p.holdoff/1E3) * fs); # index of beginning of valid return
  [r.hRet, iRet] = max(Rn(iWin:end));  # find largest return peak
  iRet += iWin -1;                      # adjust index for window offset in Rn()
  delta = iRet-iPk;            # index count from Tx to Rx peaks

# based on coarse location of the return, refine it  w/ resampled data

  rfac = int32(p.p/p.q);               # integer resample factor
  yr = resample(y,p.p,p.q);            # resample(y,p,q) y at new rate p/q
  fsr = fs * (p.p/p.q);                # sample rate of resampled waveform

  p1ri = ip1*rfac;                 # starting index of Tx pulse
  p1r=yr(p1ri:p1ri+(ioff2*rfac));   # resampled Tx pulse
  twi1 = (ip1 + delta-(0.3*p.twi2*fs))*rfac;  # start idx of resampled Rx window
  twin=yr(twi1 : twi1+(p.twi2*fsr));    # resampled target window

  [R2,lag2] = xcorr(twin, p1r .* p.factor); # cross-corr. on resampled data
  [w2, iPk2] = max(R2);                     # iPk2 index of the target peak

    # lag2(iPk2) is Target pk offset from start of twin, in reampled units
    # twi1 + lag2(iPk2) is target pk from start of yr() in resampled units

  delta1 = (twi1 + lag2(iPk2)) - p1ri;

  etime = double(delta1) / fsr;         # (sec) = index count / sample rate
  r.dist = (p.speed * etime)/2;  # one-way trip (meters) = round-trip / 2

endfunction
# ---------------------------------------------

# ============================================================================
# Main program starts here

#infile='R_1598112250.wav';       # file with stereo (2-channel) recording
#infile='R_1598206307b.wav';
#infile='R_1598145286.wav';
#infile='R_1598206307.wav';

arg_list = argv ();              # command-line inputs to this function
infile = arg_list{1};

[yraw, fs] = audioread(infile);  # load in data from wav file

fseq = strsplit(infile,"/");       # split full pathname into parts
fname = char(fseq(1,end));         # get just the filename
tstamp = fname(3:12);
kHz = fs/1000;                     # sample rate in kHz
# maxC1 = max(abs(yraw(:,1)));       # maximum value in channel
# maxC2 = max(abs(yraw(:,2)));


# search for bottom-of-pipe reflection (+)

#[dist1 c1] = findDist(yraw(:,1),fs,p);             # find distance to reflection on this trace
# =====================================================================================

pti=0;         # pulse train index

while ( 1 )

  r1 = findDist(yraw(:,1),fs,p);      # find distance to reflection on ch.1
  if (r1.hRet < 0.01)
    break
  endif

  r2 = findDist(yraw(:,2),fs,p);      # find distance to reflection on ch.2
  if (r2.hRet < 0.01)
    break
  endif

  pti += 1;   # we've got a set of pulse data
  dist(pti,1) = r1.dist;
  dist(pti,2) = r2.dist;
  cor(pti,1) = r1.hRet;
  cor(pti,2) = r2.hRet;
  max(pti,1) = r1.max;
  max(pti,2) = r2.max;

  p.tOff0 += p.tWs;                   # move ahead to next Tx/Rx pair
endwhile

if (pti > 0)  # don't print anything, if no signals detected

  #printf("%7.5f %5.3f\n",dist1, c1);

  # ----------------------------------------------
  # search for closer pipe joint reflection (expanded diam. section, so -)

  p.wLen = 24;      # (msec) full signal: window size for joint near 3.2m 19 ms
  p.factor = -1;    # look for inverted reflection
  p.msOff2 = 0.5;   # (msec) pulse window length
  p.tOff0 = p.tOff00;  # reset to start of signal

  r3 = findDist(yraw(:,1),fs,p);  # find distance to Rx on ch.1
  r4 = findDist(yraw(:,2),fs,p);  # find dist on ch.2


  # ----------------------------------------------
  # find separation between ch.1 and ch.2
  
  p.tOff0 = 0.102;      # seconds: current ROI offset from start of file
  p.tW = 0.012;         # seconds: initial ROI window size
  p.factor = 1;         # same sign signal
  p.msOff2 = 0.8646;    # (msec) pulse window length
  p.wOff1 = 0.3646;     # (msec) pulse start before peak
  p.wLen = 0.8646;      # (msec) Tx pulse ROI
  p.p = 250;            # oversample ratio p/q
  #p.tOff0 = p.tOff00;  # reset to start of signal

  r5 = findOff(yraw, fs, p);


  dist1 = mean(dist(:,1));  # average distance Ch.1
  dist2 = mean(dist(:,2));  # avg. dist Ch.2
  c1 = mean(cor(:,1));      # avg. correlation peak Ch.1
  c2 = mean(cor(:,2));
  max1 = mean(max(:,1));
  max2 = mean(max(:,2));

  nomDiff = 0.087;  # nominal separation between microphones

  d12 = dist2-dist1;  # difference in distance on ch1 and ch2 - water bottom
  d34 = r4.dist-r3.dist;  # difference in distance on ch1 and ch2 - pipe joint

  chk1 = d12-nomDiff;  # is there a large difference from expected value?
  # chk2 = d34-nomDiff;  # more fragile for some reason

  if (abs(chk1) > 0.01)
    printf("# ");
    system('touch /dev/shm/R_flag.wav');  # flag the error
  endif

  # ts = str2double(tstamp) + p.tOff0;  # realtime start point

  printf("%s %d %5.3f %5.3f ",tstamp,kHz,max1,max2);
  printf("%7.5f %7.5f %7.5f %5.4f %5.4f ",dist1,dist2,d12,c1,c2);
  printf("%7.5f %7.5f %7.5f %5.4f %5.4f ",r3.dist,r4.dist,d34,r3.hRet,r4.hRet);
  printf("%7.6f %2.1f\n",r5.dist,r5.hRet);

endif

# ====================================================================================
