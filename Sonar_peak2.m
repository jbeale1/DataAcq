# Sonar Ranging calculation: find time & distance from Tx pulse to Rx echo
# v0.6 J.Beale 09-Sep-2020

pkg load signal                  # for resample, filter
# -----------------------------------------------------------------------------
# =============================================================================

arg_list = argv ();              # command-line inputs to this function

#infile='R_1598206307b.wav';  # input file with 2-channel sonar signal
#infile='R_1599704814.wav';
#infile='R_1599711276.wav';
#infile='R_1599711395.wav';
#infile='R_1599711408.wav';

infile = arg_list{1};

# epoch  pulses d2a      std2a d2b      std2b A11   A12   B21   T1      T2      T3      T4      T5
# 1599711818 17 5.809913 0.010 5.905880 0.005 0.838 0.144 0.547 26.9947 25.1710 24.0148 26.5193 47.2
# ============================================================================
# ---------------------------------------------------------
# find Tx, Rx peaks and distance between them

function [r] = findPks(p)

 r.pk1 = -1;
 r.idx2 = -1;
 r.tdelay = -1;  # invalid defaults as "error" flag for early exit
 
 fhp =  2000;              # prefilter: highpass frequency shoulder in Hz
 [b,a] = butter(1,double(fhp)/p.fs, "high");  # create highpass Btwth filter
 w0f = filter(b,a, p.w0);
  
 rfac = int32(p.p/p.q);          # integer resample factor
 fsr = p.fs * (p.p/p.q);           # sample rate of resampled waveform

 w1 = resample(w0f,p.p,p.q);       # upsample input waveform
 [pk(1) idx(1)] = max(max(w1,0));  # find highest (Tx) peak
 w1 = w1 ./ pk(1);                 # normalize so +peak = 1.0
 r.pk1 = pk(1);                    # peak value of input waveform 
 
 w2= w1(1:idx(1)+p.zwl*fsr);  # range to search for 1st important Tx pk
 
 #hold off;  # === DEBUG ====
 #plot(w2);
 #ch=kbhit();
 
 idx3 = find(w2 > p.thr,1)-1;  # find first data above key Tx threshold
 w3=w2(idx3:end);  # peak of interest is in here
 # nd3 = diff(w3);
 nd3 = w3;
 # nd3 = nd3 ./ max(nd3); # normalize
 x = linspace(1,length(nd3),length(nd3));
 zc = zerocrossing(x,nd3);

 idx3a = idx3 - int32(p.tB4 * fsr);  # index of start of region to search
 w3a = w2(idx3a:idx3+1);  # ROI to search for zero-crossing before peak
 x2 = linspace(1,length(w3a),length(w3a));
 zc2 = zerocrossing(x2,w3a);  # zc2(end) is last zerocrossing before peak

 #{
 subplot(1,1,1);  # === DEBUG ===
 plot(x2,w3a,zc2(end),0,"xm");  # plot waveform ROI
 hold on;
 grid on;
 hold off;
 ch=kbhit();
#}
  
 #idx4 = int32(zc(1)+0.5);  # position of 1st Tx peak above threshold (float)

 tOff = zc(1) + double(idx3) -1;  # index into w1() vector (fsr rate)
 tOff2 = zc2(end) + double(idx3a) -1; # index into w1()
 
 # (tOff,tOff2) are closest zero crossings before & after Tx peak
 # ---------------------------------------
 
 sl2 = idx(1) + int32(p.t2 * fsr);  # delay after Tx peak to look for Rx
 w2 = w1(sl2:end);

 [pk(2) idx(2)] = max(max(w2,0));  # highest + peak is Rx peak
 r.pk2 = pk(2);                  # peak value of input waveform 
 
 rxno = idx(2) - (p.t3 * fsr); # rx peak negative offset for search start  
 w3 = w2(rxno:idx(2)+p.zwl*fsr);
 w3 = w3 ./ max(w3);    # normalize
 
 idx4 = find(w3 > p.thr,1)-1;  # find first data before key Rx threshold
 w4 = w3(idx4:end);
 # nd4 = diff(w4);
 nd4 = w4;
 x = linspace(1,length(nd4),length(nd4));
 zc = zerocrossing(x,nd4);  # zc(1) is 1st zero crossing after peak value
 
 idx4a = idx4 - int32(p.tB4 * fsr);  # index of start of region to search
 if (idx4a < 1)
   return
 endif
 w4a = w3(idx4a:idx4+1);  # ROI to search for zero-crossing before peak
 x2 = linspace(1,length(w4a),length(w4a));
 zc2 = zerocrossing(x2,w4a);  # zc2(end) is last zerocrossing before peak

 
 # w1 is full resampled input vector p.w0  w2 is w1 starting at sl2
 rOff = zc(1) + double(idx4 + rxno + sl2 - 3);  # index into w1() vector (fsr)
 rOff2 = zc2(end) + double(idx4a + rxno + sl2 - 3);

 # ------------------------------
 rxEdge = ((rOff+rOff2) / (2*fsr));  # rx pos'n: average of two zeros near peak
 txEdge = ((tOff+tOff2) / (2*fsr));  # tx pos'n
 tdelay = rxEdge - txEdge;  # <= Result: Elapsed time Tx ... Rx
 
 idx(1) = int32(tOff+0.5);  # peak positions
 idx(2) = int32(rOff+0.5);

 #r.pk1 = double(idx(1))/fsr;  # time of Tx peak
 r.idx2 = idx(2);
 r.txEdge = txEdge;
 r.rxEdge = rxEdge;
 r.tdelay = tdelay;
 
 r.tOff = (tOff+tOff2)/2;  # tx peak in sample units
 r.rOff = (rOff+rOff2)/2;  # rx peak 

 # == DEBUG ==
 #printf("%5.3f %5.3f\n",r.tOff/p.p,(r.rOff-r.tOff)/p.p); 
     
 #if (p.count > 6)  # first 6 pulses often significantly lower Vs, for some reason
 if (0)
   printf("%d %5.3f %5.3f\n", p.count, p.st0+txEdge, v1);
   fflush(stdout);  # actually print the line   

   # printf("%d %d %d %5.3f %5.3f %5.3f %5.3f\n",
   #    p.count,idx(1),idx(2),txEdge*1E3,rxEdge*1E3,tdelay*1E3,v1);
 endif


  if (p.doPlot)
   subplot(1,1,1);
   #plot(p.w0);
   #grid on; axis("tight");
   #ch = kbhit();  # wait for keypress
   
   xr = linspace(1,length(w1),length(w1));
   #subplot(2,1,1);
   # plot(xr,w1,xr(idx),w1(idx),"xm");
   plot(xr,w1,tOff2,0,"xm",tOff,0,"xg",rOff2,0,"xb",rOff,0,"xr");
   
   grid on;
   #printf("peaks: Tx: %d  Rx: %d  ",idx(1),idx(2));
   #xlim([1:length(w1)]);
   axis("tight");
   text(1000,-0.25,num2str(p.count),"fontsize",20);
   
   w=3; # plot window around Tx peak
   #subplot(2,1,2);
   #plot(xr(idx(1)-w:idx(1)+w),w1(idx(1)-w:idx(1)+w),tOff,1,"xm");
   #plot(xr(idx(2)-w:idx(2)+w),w1(idx(2)-w:idx(2)+w));     
         
   #grid on;
   #axis("tight");
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
# 13.0 C: 339.02 m/s  speed of sound vs temp.
# 14.0 C: 339.62 m/s
# 15.0 C: 340.21 m/s
# 20.0 C: 343.15 m/s
# 25.0 C: 346.06 m/s

 vs = 339.14; # speed of sound at 13.2 C

 t0 = 0.0795;  # seconds  start of current window in full recording
 t1 = 0.045;  # seconds width of window, containg Tx, Rx
 st0 = 0.110;  # (sec) starting index of ROI within input waveform

 iTxTh = 0.415;  # initial Tx search threshold
 tLead = 0.005;  # lead time included before Tx threshold point
 
 p.t2 = 0.0031; # seconds delay after Tx pk to look for Rx (298 @ 96k)
 p.t3 = 0.001;  # seconds before Rx peak to look for leading edge
 p.thr = 0.67;  # select Tx pk as 1st one over this fraction of max height
 p.zwl = 0.001; # seconds after pk to find 1st zero crossing
 PRR = 0.130;  # seconds Tx pulse repetition rate
 p.tB4 = 0.0002;  # seconds to search for zero crossing before peak
 
# p.p=30;  # resampling parameters
 p.p=2;  # resampling parameters
 p.q=1;
 p.count=1;  # count which Tx pulse in train
 # p.L1 = 2.9175; # length of test PVC pipe in meters
 # p.L1 = 3.228;  #
 p.L1 = 3.169;
 p.L2 = p.L1 + 0.0998;  # port separation: 100 mm
 
 p.fs = fs;
 #p.doPlot = 1;   # display output graph
 p.doPlot = 0;   # display output graph
 
 #plot(yraw(:,1)); axis("tight");
 #ch = kbhit();  # wait for keypress
    

 #printf("pulse time(s) V(m/s)\n"); # CSV column headers
 #printf("# %s\n",Ainfo.Comment);
 di1 = 1;  # index into dat() output array
 di2 = 1;  # index into dat() output array
 dat1=[];
 dat2=[];
 pk=[]; 
 
 while (1)
   sl0 = int32(st0 * fs);  # convert time to sample index  
   sl1 = int32(t1 * fs); # width of ROI window

   if (sl0+sl1 > length(yraw(:,1)))  # quit if we've gone past end of data   
     break;
   endif
   
   # -----------------------
   # find next Tx pulse: first element above iTxTh threshold
   tStart = find(yraw(sl0:end,1) > iTxTh,1); # first elem above threshold
   if isempty(tStart)  # have we run out of Tx peaks in input data?
     break
   endif
   sl0 = sl0 + tStart - int32(tLead*fs); # now back off tLead seconds from thr.
   
   st0 = double(sl0) / double(fs);  # update time offset (seconds)   
   p.w0 = yraw(sl0:sl0+sl1,1);  # current ROI within waveform: one Tx,Rx set   
   p.st0 = st0;                  # start time offset
   
   # =================================================   
   r1 = findPks(p);  # locate ch.1 peaks in current window
   # =================================================   
   #printf("Peaks: %5.3f %5.3f\n",r1.pk1,r1.pk2);  # ===DEBUG 
   #break;  # ===DEBUG
   
   if (r1.tdelay < 0)
     break;
   endif

   d2a = (r1.tdelay * vs)/2;  # observed 1-way distance to reflection

   if (p.count > 7)
    dat1(di1) = d2a;    # Ch.1 measured distance
    pk(di1,1) = r1.pk1;  # Ch.1 peak Tx amplitude
    pk(di1,2) = r1.pk2;  # Ch.1 peak Rx amplitude
    di1 += 1;
   endif
   
   #if ( 0 )  # === DEBUG
   # -----------------------
   # find ch.2 Tx pulse: first element above iTxTh threshold
   tStart = find(yraw(sl0:end,1) > iTxTh,2); # first elem above threshold
   if isempty(tStart)  # have we run out of Tx peaks in input data?
     break
   endif
   sl0 = sl0 + tStart - int32(tLead*fs); # now back off tLead seconds from thr.
   
   st0 = double(sl0) / double(fs);  # update time offset (seconds)   
   p.w0 = yraw(sl0:sl0+sl1,2);  # current ROI within waveform: one Tx,Rx set   
   p.st0 = st0;                  # start time offset
   
   # =================================================   
   r2 = findPks(p);  # locate ch.2 peaks in current window
   # =================================================      
   if (r2.tdelay < 0)
     break;
   endif

   d2b = (r2.tdelay * vs)/2;  # observed 1-way distance to reflection

   if (p.count > 7)
    dat2(di2) = d2b; # measured speed of sound    
    pk(di2,3) = r2.pk1;  # Ch.2 peak Tx amplitude
    pk(di2,4) = r2.pk2;  # Ch.2 peak Rx amplitude
    di2 += 1;
   endif
   # printf("%5.4f %5.4f\n",v1,v2); # === DEBUG ===   
      
   #endif  # === DEBUG   
      
   st0 = st0 + PRR;  # move window to next pulse
   p.count += 1;     # count up to the next pulse 
   
 endwhile

 stdev1 = std(dat1)*1E3;   # standard deviation normally < 0.014
 stdev2 = std(dat2)*1E3;   # standard deviation normally < 0.014
 a1 = mean(dat1);
 a2 = mean(dat2);
 p1 = mean(pk(:,1));  # Tx, ch1
 p2 = mean(pk(:,2));  # Rx, ch1
 p3 = mean(pk(:,3));  # Tx, ch2
 
 if (stdev1 > 0.1)    # comment out the results with large variance
   printf("# ");
 endif

 printf("%s ",tstamp);
 printf("%d %8.6f %5.3f %8.6f %5.3f %5.3f %5.3f %5.3f",
     di1,a1,stdev1,a2,stdev2,p1,p2,p3); 
 printf("\n"); # end of output
 return

# =============================================================================
#{

#}
