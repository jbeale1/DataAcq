# Sonar Ranging calculation: find time & distance from Tx pulse to Rx echo
# v0.51 J.Beale 02-Sep-2020

pkg load signal                  # for xcorr, resample

# -----------------------------------------------------------------------------
# =============================================================================

arg_list = argv ();              # command-line inputs to this function

#infile='R_1598206307b.wav';  # input file with 2-channel sonar signal
#infile='R_1599077834.wav';
#infile='R_1599077838.wav';

infile = arg_list{1};

# ============================================================================
# ---------------------------------------------------------
# find Tx, Rx peaks and distance between them

function [r] = findPks(p)

 r.pk1 = -1;
 r.idx2 = -1;
 r.tdelay = -1;
 
  
 rfac = int32(p.p/p.q);          # integer resample factor
 fsr = p.fs * (p.p/p.q);           # sample rate of resampled waveform

 w1 = resample(p.w0,p.p,p.q);  # upsample input waveform
 [pk(1) idx(1)] = max(max(w1,0));  # find highest (Tx) peak
 w1 = w1 ./ pk(1);                 # normalize so +peak = 1.0
 
 w2= w1(1:idx(1)+p.zwl*fsr);  # range to search for 1st important Tx pk
 
 #hold off;
 #plot(w2);
 #ch=kbhit();
 
 idx3 = find(w2 > p.thr,1)-1;  # find first data above key Tx threshold
 w3=w2(idx3:end);  # peak of interest is in here
 # nd3 = diff(w3);
 nd3 = w3;
 nd3 = nd3 ./ max(nd3); # normalize
 x = linspace(1,length(nd3),length(nd3));
 zc = zerocrossing(x,nd3);

 #{
 subplot(1,1,1);
 plot(x.+0.5,nd3);  # derivative of waveform
 hold on;
 grid on;
 plot(w3.-1); # original waveform (offset)
 plot(zc(1)+0.5,0,"x"); # zero crossing of derivative 
 printf("zc(1) = %5.3f\n",zc(1));
 hold off;
 ch=kbhit();
 #}
  
 #idx4 = int32(zc(1)+0.5);  # position of 1st Tx peak above threshold (float)

 tOff = zc(1) + double(idx3) -1;  # index into w1() vector (fsr rate)
 txEdge = (tOff / fsr); # tx start, abs.seconds
 # ---------------------------------------
 
 sl2 = idx(1) + int32(p.t2 * fsr);  # delay after Tx peak to look for Rx
 w2 = w1(sl2:end);

 [pk(2) idx(2)] = max(max(w2,0));  # find Rx peak
 
 rxno = idx(2) - (p.t3 * fsr); # rx peak negative offset for search start  
 w3 = w2(rxno:idx(2)+p.zwl*fsr);
 w3 = w3 ./ max(w3);    # normalize
 
 idx4 = find(w3 > p.thr,1)-1;  # find first data above key Rx threshold
 w4 = w3(idx4:end);
 # nd4 = diff(w4);
 nd4 = w4;
 x = linspace(1,length(nd4),length(nd4));
 zc = zerocrossing(x,nd4);

 rOff = zc(1) + double(idx4 + rxno + sl2 - 3);  # index into w1() vector (fsr rate)
 idx5 = int32(rOff+0.5);
 rxEdge = (rOff / fsr); 
 # ------------------------------
 
 idx(1) = int32(tOff+0.5);  # new more accurate peak positions
 idx(2) = int32(rOff+0.5);

 tdelay = rxEdge - txEdge;  # elapsed time Tx ... Rx

 r.pk1 = double(idx(1))/fsr;  # time of Tx peak
 r.idx2 = idx(2);
 r.txEdge = txEdge;
 r.rxEdge = rxEdge;
 r.tdelay = tdelay;
 r.tOff = tOff;  # tx peak in sample units
 r.rOff = rOff;

 v1 = (2 * p.L1) / tdelay;  # observed speed of sound
 r.v1 = v1;

 # == DEBUG ==
 printf("%5.3f %5.3f\n",r.tOff/p.p,(r.rOff-r.tOff)/p.p); 
     
 #if (p.count > 6)  # first 6 pulses often significantly lower Vs, for some reason
 if (0)
   printf("%d %5.3f %5.3f\n", p.count, p.st0+txEdge, v1);
   fflush(stdout);  # actually print the line   

   # printf("%d %d %d %5.3f %5.3f %5.3f %5.3f\n",
   #    p.count,idx(1),idx(2),txEdge*1E3,rxEdge*1E3,tdelay*1E3,v1);
 endif


  if (p.doPlot)
   xr = linspace(1,length(w1),length(w1));
   subplot(2,1,1);
   plot(xr,w1,xr(idx),w1(idx),"xm");
   grid on;
   #printf("peaks: Tx: %d  Rx: %d  ",idx(1),idx(2));
   #xlim([1:length(w1)]);
   axis("tight");
   text(1000,-0.25,num2str(p.count),"fontsize",20);
   
   w=3; # plot window around Tx peak
   subplot(2,1,2);
   #plot(xr(idx(1)-w:idx(1)+w),w1(idx(1)-w:idx(1)+w),tOff,1,"xm");
   plot(xr(idx(2)-w:idx(2)+w),w1(idx(2)-w:idx(2)+w),rOff,w1(idx5),"xm");     
         
   grid on;
   axis("tight");
   ch = kbhit();  # wait for keypress
 endif
 
endfunction

# =======================================================
# generate sinewave at given frequency, length, samplerate
function [xt yt] = mkPulse(freq, tdur, fs)
  xt=linspace(0,tdur,fs*tdur);  # time vector
  yt=sin(xt .* (2*pi*freq));
endfunction

# write out set of single-cycle pulses into audio file
function writeTx(f,pulses,fout)
  fs=48000; # Tx sample rate in Hz
  [xt yt] = mkPulse(f,1/f,fs);  # single pulse
  wt = int16(32767*(yt)); # 16-bit signed int words

  pRep = 3*0.130;  # seconds frame length = Tx pulse repetition rate
  pOff = 0.050;  # offset of pulse in frame
  pSamp = int32(fs*pRep);
  F0 = zeros (pSamp, 1, "int16"); # all-zero frame
  F = F0;  # frame to insert a pulse in
  pOffi = pOff*fs;
  F(pOffi:pOffi+length(wt)-1) = wt;  # insert pulse into frame
  set=F;  # create first pulse in output waveform
  for i = 2:pulses
    set=[set; F];   # add in this many pulse frames
  endfor
  set=[set; F0]; # zero frame at end to allow time for echo
  audiowrite (fout, set, fs, 'Comment', '12 single cycle pulses at 5.1 kHz');
endfunction

#{
f = 5100; # frequency in Hz
writeTx(f,12,"p12_5p1kHz.wav");
#}

# ============================================================================
# Main program starts here
# ============================================================================

[yraw, fs] = audioread(infile);  # load in data from wav file
Ainfo = audioinfo(infile);        # get .wav comment

fseq = strsplit(infile,"/");       # split full pathname into parts
fname = char(fseq(1,end));         # get just the filename
tstamp = fname(3:12);
kHz = fs/1000;                     # sample rate in kHz
# =============================================================================

# 76.5 F : 345.9 m/s
# 76.7 F : 345.96 m/s
# 77.0 F : 346.06 m/s
# 77.4 F : 346.19 m/s
# 77.5 F : 346.22 m/s
# 72.3 F : 344.54 m/s
# 72.8 F : 344.70 m/s
# T actual 72.8 F, should be 344.7 m/s

 t0 = 0.0795;  # seconds  start of current window in full recording
 t1 = 0.045;  # seconds width of window, containg Tx, Rx

 iTxTh = 0.415;  # initial Tx search threshold
 tLead = 0.005;  # lead time included before Tx threshold point
 
 p.t2 = 0.0031; # seconds delay after Tx pk to look for Rx (298 @ 96k)
 p.t3 = 0.001;  # seconds before Rx peak to look for leading edge
 p.thr = 0.67;  # select Tx pk as 1st one over this fraction of max height
 p.zwl = 0.001; # seconds after pk to find 1st zero crossing
 PRR = 0.130;  # seconds Tx pulse repetition rate
 
# p.p=30;  # resampling parameters
 p.p=2;  # resampling parameters
 p.q=1;
 p.count=1;  # count which Tx pulse in train
 # p.L1 = 2.9175; # length of test PVC pipe in meters
 p.L1 = 3.228;

 p.fs = fs;
 #p.doPlot = 1;   # display output graph
 p.doPlot = 0;   # display output graph
 
 #plot(yraw(:,1)); axis("tight");
 #ch = kbhit();  # wait for keypress
    
 st0 = 0.110;  # (sec) starting index of ROI within input waveform

 #printf("pulse time(s) V(m/s)\n"); # CSV column headers
 #printf("# %s\n",Ainfo.Comment);
 di = 1;  # index into dat() output array
 
 while (1)
   sl0 = int32(st0 * fs);  # convert time to sample index  
   sl1 = int32(t1 * fs); # width of ROI window

   #printf("sl0,sl1 %d %d\n",sl0,sl1);   # DEBUG
   #fflush(stdout);

   #if (sl0+sl1 > length(yraw(:,1)))  # quit if we've gone past end of data
   if (sl0+sl1 > length(yraw))  # quit if we've gone past end of data
     break;
   endif
   
   #plot(yraw(sl0:sl0+sl1));
   #grid on;
   #axis("tight");
   #ch = kbhit();  # wait for keypress
   
   # find next Tx pulse: first element above iTxTh threshold
   #tStart = find(-yraw(sl0:sl0+sl1,1) > iTxTh,1); # first elem above threshold
   tStart = find(yraw(sl0:end) > iTxTh,1); # first elem above threshold
   if isempty(tStart)  # have we run out of Tx peaks in input data?
     break
   endif
   sl0 = sl0 + tStart - int32(tLead*fs); # now back off tLead seconds from thr.
   
   #plot(-yraw(sl0:sl0+sl1,1));
   #grid on;
   #axis("tight");
   #ch = kbhit();  # wait for keypress

   st0 = double(sl0) / double(fs);  # update time offset (seconds)   
   #p.w0 = -yraw(sl0:sl0+sl1,1);  # current ROI within waveform: one Tx,Rx set   
   p.w0 = yraw(sl0:sl0+sl1);  # current ROI within waveform: one Tx,Rx set   
   p.st0 = st0;                  # start time offset
   
   # =================================================   
   r1 = findPks(p);  # locate peaks in current window
   # =================================================   
   if (p.count > 6)
    dat(di) = r1.v1; # measured speed of sound
    di += 1;
   endif

   if (r1.tdelay < 0)
     break;
   endif
   st0 = st0 + PRR;  # move window to next pulse
   p.count += 1;     # count up to the next pulse 

 endwhile
 stdev = std(dat);   # standard deviation normally < 0.014

 if (stdev > 0.1)    # comment out the results with large variance
   printf("# ");
 endif

 printf("%s ",tstamp);
 printf("%d %5.3f %5.3f %5.3f %5.3f",
     di,mean(dat),stdev,r1.tOff/p.p,(r1.rOff-r1.tOff)/p.p); 
 printf("\n"); # end of output
 return

# =============================================================================
#{

#}
