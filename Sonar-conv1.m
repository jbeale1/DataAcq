# Sonar Ranging calculation: find distance between pulse and echo
# v0.2 J.Beale 21-AUG-2020

pkg load signal                  # for xcorr, resample

# --------------------------------------------------------------------------------------------
p.pThresh = 0.3;                   # 1st sample above this is start of outgoing pulse
p.pThresh2 = 0.55;                 # 2ndary corr. peaks of initial pulse can reach 0.4
p.minPkDist = 80;                  # minimum allowed distance between peaks

p.msOff1 = 0.4;                    # (msec) pulse window start before pulse edge
p.msOff2 = 1.7;                    # (msec) pulse window length (1.7 ms best for pk.2)

p.wOff1 = 3.5;                     # (msec) signal window before edge
p.wOff2 = 15;                      # (msec) signal window size (was 20)

p.speed = 346.06;                  # sound speed (m/s) at 77 F / 25 C
p.p=400;                           # p/q is oversample ratio
p.q=1;				 # denominator of oversample ratio
# --------------------------------------------------------------------------------------------

function [dist pks] = findDist(yr, fs, p)   # find distance to first reflection in inputdata

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

  delta = diff(idx);           # distance (idx count) between successive peaks
  etime = delta(1) / fsr;      # (sec) = index count / sample rate
  dist = (p.speed * etime)/2;    # one-way trip = round-trip / 2

endfunction

# ====================================================================================
# Main program starts here

#infile='recData_0820_1010.wav';  # filename with stereo recording
#infile='R_1598021217.wav';
#infile='R_1598019458.wav';

arg_list = argv ();              # command-line inputs to this function
infile = arg_list{1};

[yraw, fs] = audioread(infile);  # load in data from audio wav file

[dist1 pks1] = findDist(yraw(:,1),fs,p);             # find distance to reflection on this trace
[dist2 pks2] = findDist(yraw(:,2),fs,p);             # find distance to reflection on this trace

fseq = strsplit(infile,"/");       # split full pathname into parts
fname = char(fseq(1,end));         # get just the filename

d12 = 1E3 * (dist2-dist1);  # difference in distance on ch1 and ch2, in mm
printf("%s %8.6f %8.6f %6.3f %5.3f %5.3f\n",fname,dist1,dist2,d12,pks1(2),pks2(2));

# ====================================================================================
