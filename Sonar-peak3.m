# Sonar Ranging calculation: find time & distance from Tx pulse to one Rx echo
# v0.7 J.Beale 19-Oct-2020  tuning for Sensor #3

# note: 51.6 mm on Sensor #3 mic center to 1" coupler edge
#  also, 11.5 (loose) to 15.2 (tight) mm insertion depth of pipe end into 1" coupler

pkg load signal                  # for resample, filter
# -----------------------------------------------------------------------------
# =============================================================================

arg_list = argv ();              # command-line inputs to this function

#infile='R_1598206307b.wav';  # input file with 2-channel sonar signal
#infile='R_1603157801.wav';

infile = arg_list{1};

# CSV file column headers
# epoch pcnt d1a d1astd d2a d2astd d1b d1bstd d2b d2bstd A11 A12 B21 T1plate T2back T3cplr T4air T5CPU
# 1600021321 19 3.098103 0.009 5.736037 0.015 3.198298 0.010 5.833960 0.009 0.754 0.207 0.448 01 02 03 04 05
# ============================================================================
# ---------------------------------------------------------
# find Tx, Rx1, Rx2 peaks and distance between them

function [r] = findPks(p)

 r.pk1 = -1;
 r.idx2 = -1;
 r.t1delay = -1;  # invalid defaults as "error" flag for early exit
 r.t2delay = -1;  # invalid defaults as "error" flag for early exit
 
 fhp =  3000;     # prefilter: highpass frequency shoulder in Hz (2000)
 [b,a] = butter(1,double(fhp)/p.fs, "high");  # create highpass Btwth filter
 w0f = filter(b,a, p.w0);
  
 rfac = int32(p.p/p.q);          # integer resample factor
 fsr = p.fs * (p.p/p.q);           # sample rate of resampled waveform

 w1 = resample(w0f,p.p,p.q);       # upsample input waveform
 [pk(1) idx(1)] = max(max(w1,0));  # find highest (Tx) peak
 w1 = w1 ./ pk(1);                 # normalize so +peak = 1.0
 r.pk1 = pk(1);                    # peak value of input waveform 
 rangeEnd = cast(idx(1)+p.zwl*fsr, "int32");
 #printf("rangeEnd = %d\n",rangeEnd); # --- DEBUG
 if (rangeEnd > length(w1))
  return
 endif
 
 w2= w1(1:idx(1)+p.zwl*fsr);  # range to search for 1st important Tx pk
 
 if (p.doPlot)  # plot first region (contains Tx peak)
  plot(w2);
  ch=kbhit();
 endif
 
 idx3 = find(w2 > p.thr,1)-1;  # find first data point above key Tx threshold
 if (idx3 < 1)
   return
 endif
 #printf("  idx3: %d\n", idx3);
 w3=w2(idx3:end);  # peak of interest is in here

 if (p.doPlot)  # plot first region (contains Tx peak)
  plot(w3);
  ch=kbhit();
 endif

 #nd3 = w3;
 x = linspace(1,length(w3),length(w3));
 zc = zerocrossing(x,w3);

 idx3a = idx3 - int32(p.tB4 * fsr);  # index of start of region to search
 if (idx3a < 1)
  return
 endif
 w3a = w2(idx3a:idx3+1);  # ROI to search for zero-crossing *before* peak
 #printf("idx3a: %d\n", idx3a);

 x2 = linspace(1,length(w3a),length(w3a));
 zc2 = zerocrossing(x2,w3a);  # zc2(end) is last zerocrossing before peak

  if (p.doPlot)
   plot(x2,w3a,zc2(end),0,"xm");  # plot waveform ROI
   grid on;
   ch=kbhit();
  endif
  
 #idx4 = int32(zc(1)+0.5);  # position of 1st Tx peak above threshold (float)

 # Check if zc or zc2 have 0 elements. No zero crossings => bad data set
 if ((length(zc) < 1) || (length(zc2) < 1))
  return
 endif

 tOff = zc(1) + double(idx3) -1;  # index into w1() vector (fsr rate)
 tOff2 = zc2(end) + double(idx3a) -1; # index into w1()
 # (tOff2,tOff) are closest zero crossings before & after Tx peak

 # -------------------------------------------------------------------
 sl2 = idx(1) + int32(p.t2 * fsr);  # delay after Tx peak to look for Rx1,2
 
 # ---------------------------------------- 
 # --- search for Rx2 peak (water surface reflection)

 if ( (sl2+1) > length(w1) )
   return
 endif
 w2 = w1(sl2:end);

 [pk(2) idx(2)] = max(max(w2,0));  # highest + peak is Rx2 peak
 r.pk2 = pk(2);                  # peak value of input waveform 
 
 rxno = idx(2) - (p.t3 * fsr); # rx peak negative offset for search start  
 if ((rxno < 1) || rxno >= length(w2))
  return
 endif

 endRange = idx(2)+p.zwl*fsr;
 if (endRange > length(w2))
  return
 endif

 w3 = w2(rxno:endRange);
 w3 = w3 ./ max(w3);    # normalize
 
 idx4 = find(w3 > p.thr,1)-1;  # find first data before Rx2 threshold
 if (idx4 < 1)
  return
 endif
 w4 = w3(idx4:end); 
 nd4 = w4;
 x = linspace(1,length(nd4),length(nd4));
 zc = zerocrossing(x,nd4);  # zc(1) is 1st zero crossing after peak value
 if (length(zc) < 1)
  return
 endif
 
 idx4a = idx4 - int32(p.tB4 * fsr);  # index of start of region to search
 if (idx4a < 1)
   return
 endif
 w4a = w3(idx4a:idx4+1);  # ROI to search for zero-crossing before peak
 x2 = linspace(1,length(w4a),length(w4a));
 zc2 = zerocrossing(x2,w4a);  # zc2(end) is last zerocrossing before peak
 if (length(zc2) < 1)
  return
 endif
 
 # w1 is full resampled input vector p.w0  w2 is w1 starting at sl2
 rOff = zc(1) + double(idx4 + rxno + sl2 - 3);  # index into w1() vector (fsr)
 rOff2 = zc2(end) + double(idx4a + rxno + sl2 - 3);

 # ------------------------------
 rx2Edge = ((rOff+rOff2) / (2*fsr));  # rx2 pos'n: avg of two zeros by peak
 txEdge = ((tOff+tOff2) / (2*fsr));  # tx pos'n
 #rx2Edge = ((rOff) / (fsr));  # rx2 pos'n: do not average
 #txEdge = ((tOff) / (fsr));  # tx pos'n

 t2delay = rx2Edge - txEdge;  # <= Result: Elapsed time Tx ... Rx
 
 idx(1) = int32(tOff+0.5);  # peak positions
 idx(2) = int32(rOff+0.5);

 #r.pk1 = double(idx(1))/fsr;  # time of Tx peak
 r.idx2 = idx(2);
 r.txEdge = txEdge;
 #r.rx1Edge = rx1Edge;
 r.rx2Edge = rx2Edge;
 #r.t1delay = t1delay;
 r.t2delay = t2delay;
 
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
   plot(xr,w1,tOff2,0,"xm",tOff,0,"xg", rOff2,0,"xb",rOff,0,"xr");
   
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


# ============================================================================
# Main program starts here
# ============================================================================

[yrawR, fs] = audioread(infile);  # load in data from wav file
Ainfo = audioinfo(infile);        # get .wav comment

yraw = -yrawR;   # invert DEBUG

fseq = strsplit(infile,"/");       # split full pathname into parts
fname = char(fseq(1,end));         # get just the filename
tstamp = fname(3:12);
kHz = fs/1000;                     # sample rate in kHz
# =============================================================================
V0 = 331.23; # m/s reference speed of sound at T0 
T0 = 273.15; # K at 0 C, temperature of sound reference
#Tback=14.8; # current pipe-back temperature
#Tback=13.2; # current pipe-back temperature
Tback=20.2; # current pipe-back temperature

# vs = V0 * sqrt(Tback / T0)
# 13.0 C: 339.02 m/s  speed of sound vs temp.
# 14.0 C: 339.62 m/s
# 14.8 C: 340.09 m/s
# 15.0 C: 340.21 m/s
# 20.0 C: 343.15 m/s
# 25.0 C: 346.06 m/s

 # vs = 339.14; # speed of sound at 13.2 C

 t0 = 0.0795;  # seconds  start of current window in full recording
 #t0 = 0.1995;  # seconds  start of current window in full recording
 t1 = 0.045;  # seconds width of window, containg Tx, Rx
 st0 = 0.110;  # (sec) starting index of ROI within input waveform

 iTxTh = 0.415;  # initial Tx search threshold
 tLead = 0.005;  # lead time included before Tx threshold point
 throwCount = 1; # throw away this many pulses at start
 
 p.t2 = 0.0091; # seconds delay after Tx pk to look for Rx1,2 (298 @ 96k)
 p.t2a = .0218; # seconds after Tx pk beyond which Rx1 cannot be
 p.t3 = 0.001;  # seconds before Rx peak to look for leading edge
 p.thr = 0.5;  # select Tx pk as 1st one over this fraction of max height
 p.thr1 = 0.04; # Rx1 peak detect threshold as fraction of Tx peak
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
 
 if (p.doPlot)
  plot(yraw(:,1)); axis("tight");
  ch = kbhit();  # wait for keypress
 endif
    

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
   rangeEnd = sl0+sl1;
   if (rangeEnd > length(yraw(:,1)) )
     break
   endif
   
   st0 = double(sl0) / double(fs);  # update time offset (seconds)   
   p.w0 = -yraw(sl0:sl0+sl1,1);  # current ROI within waveform: one Tx,Rx set   
   p.st0 = st0;                  # start time offset
   
   # =================================================   
   r1 = findPks(p);  # locate ch.1 peaks in current window
   # =================================================   
   #printf("Peaks: %5.3f %5.3f\n",r1.pk1,r1.pk2);  # ===DEBUG 
   #break;  # ===DEBUG
   
   if (r1.t2delay < 0)  # quit due to error?
     return;
   endif

   vs = V0 * sqrt((Tback+T0) / T0); # speed of sound at T = (Tback)
   #d1a = (r1.t1delay * vs)/2;  # observed 1-way distance to refl.1
   d2a = (r1.t2delay * vs)/2;  # observed 1-way distance to refl.2

   if (p.count > throwCount)
    #dat11(di1) = d1a;    # Ch.1 measured distance
    dat12(di1) = d2a;    # Ch.1 measured distance
    pk(di1,1) = r1.pk1;  # Ch.1 peak Tx amplitude
    pk(di1,2) = r1.pk2;  # Ch.1 peak Rx amplitude
    di1 += 1;
   endif
   
   # -----------------------
   # find ch.2 Tx pulse: first element above iTxTh threshold
   tStart = find(yraw(sl0:end,1) > iTxTh,2); # first elem above threshold
   if isempty(tStart)  # have we run out of Tx peaks in input data?
     break
   endif

   st0 = st0 + PRR;  # move window to next pulse
   p.count += 1;     # count up to the next pulse 
   
 endwhile

 if (p.count < (throwCount+1))
  printf("# Error pulse count");
  return
 endif

 stdev12 = std(dat12)*1E3;   # standard deviation normally < 0.014
 a12 = mean(dat12);
 p1 = mean(pk(:,1));  # Tx, ch1
 p2 = mean(pk(:,2));  # Rx, ch1
 
 if (stdev12 > 20.0)    # comment out the results with large variance (mm)
   printf("# ");
 endif

 printf("%s ",tstamp);
 printf("%d %8.6f %5.3f %8.6f %5.3f %5.3f",
     di1-1,a12,stdev12,a12,stdev12,p1); 
 printf("\n"); # end of output
 return

# =============================================================================
