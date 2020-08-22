# Sonar Ranging calculation: find time & distance from Tx pulse to Rx echo
# v0.3 J.Beale 22-AUG-2020

pkg load signal                  # for xcorr, resample

# --------------------------------------------------------------------------------------------
p.pThresh = 0.3;                   # 1st sample above this is start of outgoing pulse
p.pThresh2 = 0.55;                 # 2ndary corr. peaks of initial pulse can reach 0.4
p.minPkDist = 80;                  # minimum allowed distance between peaks

p.msOff1 = 0.4;                    # (msec) pulse window start before pulse edge
p.msOff2 = 1.0;                    # (msec) pulse window length (1.7 ms best for pk.2)
p.holdoff = 0.625;                 # (msec) start looking for echoes this long after Tx pulse

p.wOff1 = 3.5;                     # (msec) full signal: start before Tx
p.wOff2 = 15;                      # (msec) full signal: window size

p.speed = 346.06;                  # sound speed (m/s) at 77 F / 25 C
p.p=800;                           # p/q is oversample ratio
p.q=1;				   # denominator of oversample ratio
# --------------------------------------------------------------------------------------------

# find distance (meters) and correlation (0..1: quality value) to largest reflection
function [dist hRet] = findDist(yr, fs, p)

  pStart = find(yr>p.pThresh);   # find index of start of pulse
  iwoff1 = int32(p.wOff1 / 1E3 * fs);         # start of pulse window
  iwoff2 = int32(p.wOff2 / 1E3 * fs);         # end of pulse window
  y=(yr(pStart-iwoff1:(pStart-iwoff1)+iwoff2)); # ROI containing pulse + echo #1
  #plot(y,'-x');                    # have a look

  rfac = int32(p.p/p.q);               # integer resample factor
  yr = resample(y,p.p,p.q);            # resample(y,p,q) y at new rate p/q
  fsr = fs * (p.p/p.q);                # sample rate of resampled waveform

  pStartr = find(yr>p.pThresh);      # find leading edge of pulse in resampled signal

  ioff1 = int32((p.msOff1 / 1E3) * fsr);         # start of pulse window
  ioff2 = int32((p.msOff2 / 1E3) * fsr);         # end of pulse window
  p1=(yr(pStartr-ioff1:(pStartr-ioff1)+ioff2));         # region with pulse + echo #1

  #plot(p1,'-x');
  [R,lag] = xcorr(yr,p1);           # do the cross-correlation
  Rn = max(0,R ./ max(R));	 # normalize for peak=1.0, clamp neg to 0
  #plot(lag,Rn);

  [pks idx] = findpeaks(Rn,"MinPeakHeight",p.pThresh2,"MinPeakDistance",p.minPkDist);
             # idx(1) = initial pulse, idx(2) = end refl. idx(3) = back reflection

  [w, iPk] = max(Rn);     # iPk is index of Tx peak
  iWin = iPk + ((p.holdoff/1E3) * fsr); # index of beginning of valid return
  [hRet, iRet] = max(Rn(iWin:end));  # find largest return peak
  iRet += iWin;                      # adjust index for window offset in Rn()

  delta = iRet-iPk;            # index count from Tx to Rx peaks
  etime = delta / fsr;         # (sec) = index count / sample rate
  dist = (p.speed * etime)/2;  # one-way trip (meters) = round-trip / 2

endfunction


# ====================================================================================
# Main program starts here

#infile='R_1598112250.wav';       # file with stereo (2-channel) recording

arg_list = argv ();              # command-line inputs to this function
infile = arg_list{1};

[yraw, fs] = audioread(infile);  # load in data from wav file

[dist1 c1] = findDist(yraw(:,1),fs,p);             # find distance to reflection on this trace
[dist2 c2] = findDist(yraw(:,2),fs,p);             # find distance to reflection on this trace

fseq = strsplit(infile,"/");       # split full pathname into parts
fname = char(fseq(1,end));         # get just the filename

d12 = dist2-dist1;  # difference in distance on ch1 and ch2
printf("%s %8.6f %8.6f %8.6f %5.4f %5.4f\n",fname,dist1,dist2,d12,c1,c2);

# ====================================================================================
