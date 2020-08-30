# Sonar Ranging calculation: find time & distance from Tx pulse to Rx echo
# v0.3 J.Beale 29-AUG-2020

pkg load signal                  # for xcorr, resample

# -----------------------------------------------------------------------------
# =============================================================================

#infile='R_1598206307b.wav';  # input file with 2-channel sonar signal
#infile='R_1598550498.wav';  # exp
#infile='R_1598553633a.wav'; # exp
#infile='R_1598553634_PVC10c.wav';
#infile='R_1598710985_Aug29a.wav';
#infile='R_1598713155_Aug29b.wav';
#infile='R_1598723365_Aug29c.wav';
#infile='R_1598723935_Aug29d.wav';
#infile='R_1598724730_Aug29e.wav';
#infile='R_1598736794_Aug29f.wav'; # wall: T=71.9 F
#infile='R_1598737570_Aug29g.wav'; # wall: T=71.9 F
#infile='R_1598738130_Aug29h.wav'; # wall: T=71.9 F
#infile='R_1598738572_Aug29_721F.wav'; # wall: T=72.1 F
#infile='R_1598739059_Aug29_721G.wav';
infile='R_1598742360_Aug29_726H.wav';

#arg_list = argv ();              # command-line inputs to this function
#infile = arg_list{1};

# ============================================================================
# ---------------------------------------------------------
# find Tx, Rx peaks and distance between them

function [r] = findPks(p)

 r.pk1 = -1;
 r.idx2 = -1;
 r.tdelay = -1;
  
 rfac = int32(p.p/p.q);          # integer resample factor
 fsr = p.fs * (p.p/p.q);           # sample rate of resampled waveform

 w1 = resample(p.w0,p.p,p.q);  # upsample input waveform (inverted)
 [pk(1) idx(1)] = max(max(w1,0));  # find highest (Tx) peak
 w1 = w1 ./ pk(1);                 # normalize so +peak = 1.0
 
 w2= w1(1:idx(1)+1);  # range to search for 1st important Tx pk
 idx3 = find(w2 > p.thr,1);  # find first data above key Tx threshold
 w3=w2(idx3:end);  # peak of interest is in here
 nd3 = diff(w3);
 nd3 = nd3 ./ max(nd3); # normalize
 x = linspace(1,length(nd3),length(nd3));
 zc = zerocrossing(x,nd3);
 idx4 = int32(zc(1)+0.5);  # position of 1st Tx peak above threshold (float)

 tOff = zc(1) + double(idx3);  # index into w1() vector (fsr rate)
 txEdge = (tOff / fsr); # tx start, abs.seconds
 # ---------------------------------------
 
 sl2 = idx(1) + int32(p.t2 * fsr);  # delay after Tx peak to look for Rx
 w2 = w1(sl2:end);

 [pk(2) idx(2)] = max(max(w2,0));  # find Rx peak
 
 rxno = idx(2) - (p.t3 * fsr); # rx peak negative offset for search start  
 w3 = w2(rxno:idx(2)+1);
 w3 = w3 ./ max(w3);    # normalize
 
 idx4 = find(w3 > p.thr,1);  # find first data above key Rx threshold
 w4 = w3(idx4:end);
 nd4 = diff(w4);
 x = linspace(1,length(nd4),length(nd4));
 zc = zerocrossing(x,nd4);

 rOff = zc(1) + double(idx4 + rxno + sl2 - 3.0);  # index into w1() vector (fsr rate)
 rxEdge = (rOff / fsr); 
 
 idx(1) = int32(tOff);  # new more accurate peak positions
 idx(2) = int32(rOff);

 tdelay = rxEdge - txEdge;  # elapsed time Tx ... Rx

 r.pk1 = double(idx(1))/fsr;  # time of Tx peak
 r.idx2 = idx(2);
 r.txEdge = txEdge;
 r.rxEdge = rxEdge;
 r.tdelay = tdelay;

 V1 = (2 * p.L1) / tdelay;  # observed speed of sound
 # printf("%d %d %d %5.3f %5.3f %5.3f %5.3f\n",
 #    p.count,idx(1),idx(2),txEdge*1E3,rxEdge*1E3,tdelay*1E3,V1);
 printf("%d %5.3f %5.3f\n",
    p.count, p.st0+txEdge, V1);
 fflush(stdout);  # actually print the line   

  if (p.doPlot)
   xr = linspace(1,length(w1),length(w1));
   plot(xr,w1,xr(idx),w1(idx),"xm");
   grid on;
   #printf("peaks: Tx: %d  Rx: %d  ",idx(1),idx(2));
   #xlim([1:length(w1)]);
   axis("tight");
   text(1000,-0.25,num2str(p.count),"fontsize",20);
   ch = kbhit();  # wait for keypress
 endif
 
endfunction

# =======================================================

# ============================================================================
# Main program starts here

[yraw, fs] = audioread(infile);  # load in data from wav file

fseq = strsplit(infile,"/");       # split full pathname into parts
fname = char(fseq(1,end));         # get just the filename
tstamp = fname(3:12);
kHz = fs/1000;                     # sample rate in kHz
# =====================================================================================

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

 iTxTh = 0.3;  # initial Tx search threshold
 tLead = 0.005;  # lead time included before Tx threshold point
 
 p.t2 = 0.0031; # seconds delay after Tx pk to look for Rx (298 @ 96k)
 p.t3 = 0.001;  # seconds before Rx peak to look for leading edge
 p.thr = 0.67;  # select Tx pk as 1st one over this fraction of max height
 PRR = 0.130;  # seconds Tx pulse repetition rate
 
# p.p=30;  # resampling parameters
 p.p=30;  # resampling parameters
 p.q=1;
 p.count=1;  # count which Tx pulse in train
 p.L1 = 2.9175; # length of test PVC pipe in meters(:

 p.yraw = yraw;
 p.fs = fs;
 p.doPlot = 1;   # display output graph
 
 #plot(yraw(:,1)); axis("tight");
 #ch = kbhit();  # wait for keypress
    
 st0 = 0.05;  # (sec) starting index of ROI within input waveform

 printf("pulse time(s) V(m/s)\n"); # CSV column headers
 printf("# %s %s\n",tstamp,fname);
 
 while (1)
   sl0 = int32(st0 * fs);  # convert time to sample index  
   sl1 = int32(t1 * fs); # width of ROI window

   #printf("sl0,sl1 %d %d\n",sl0,sl1);   # DEBUG
   #fflush(stdout);

   if (sl0+sl1 > length(yraw(:,1)))  # quit if we've gone past end of data
     break;
   endif
   
   #plot(-yraw(sl0:sl0+sl1,1));
   #grid on;
   #axis("tight");
   #ch = kbhit();  # wait for keypress
   
   # find next Tx pulse: first element above iTxTh threshold
   tStart = find(-yraw(sl0:sl0+sl1,1) > iTxTh,1); # first elem above threshold
   sl0 = sl0 + tStart - int32(tLead*fs); # now back off tLead seconds from thr.
   
   #plot(-yraw(sl0:sl0+sl1,1));
   #grid on;
   #axis("tight");
   #ch = kbhit();  # wait for keypress

   st0 = double(sl0) / double(fs);  # update time offset (seconds)   
   p.w0 = -yraw(sl0:sl0+sl1,1);  # current ROI within waveform: one Tx,Rx set   
   p.st0 = st0;                  # start time offset
   
   # =================================================   
   r1 = findPks(p);  # locate peaks in current window
   # =================================================   

   if (r1.tdelay < 0)
     break;
   endif
   st0 = st0 + PRR;  # move window to next pulse
   p.count += 1;     # count up to the next pulse 

 endwhile
 
 return

# =============================================================================
#{

1598723365 R_1598723365_Aug29c.wav  (wall: 72.1 F)
peaks: Tx: 3413  Rx: 8292  Time: 0.080 s  Pk 0.091 s  Speed = 344.428 m/s
peaks: Tx: 2672  Rx: 7551  Time: 0.223 s  Pk 0.233 s  Speed = 344.454 m/s
peaks: Tx: 3617  Rx: 8496  Time: 0.367 s  Pk 0.380 s  Speed = 344.461 m/s
peaks: Tx: 2887  Rx: 7765  Time: 0.511 s  Pk 0.522 s  Speed = 344.488 m/s
peaks: Tx: 2158  Rx: 7037  Time: 0.655 s  Pk 0.663 s  Speed = 344.482 m/s

1598723935 R_1598723935_Aug29d.wav
Time: 0.080 s  Pk 0.089 s  Speed = 344.430 m/s
Time: 0.223 s  Pk 0.231 s  Speed = 344.451 m/s
Time: 0.367 s  Pk 0.378 s  Speed = 344.458 m/s
Time: 0.511 s  Pk 0.520 s  Speed = 344.475 m/s
Time: 0.655 s  Pk 0.661 s  Speed = 344.489 m/s
Time: 0.799 s  Pk 0.809 s  Speed = 344.493 m/s
Time: 0.944 s  Pk 0.950 s  Speed = 344.490 m/s
Time: 1.087 s  Pk 1.092 s  Speed = 344.518 m/s
Time: 1.231 s  Pk 1.239 s  Speed = 344.516 m/s
Time: 1.375 s  Pk 1.380 s  Speed = 344.519 m/s
Time: 1.519 s  Pk 1.522 s  Speed = 344.508 m/s
Time: 1.663 s  Pk 1.670 s  Speed = 330.510 m/s

pulse time(s) V(m/s)
# 1598736794 R_1598736794_Aug29f.wav
1 0.090 344.620
2 0.232 344.648
3 0.379 344.669
4 0.521 344.668
5 0.662 344.679
6 0.810 344.692
7 0.951 344.700
8 1.093 344.702
9 1.240 344.730
10 1.381 344.733
11 1.523 344.718
12 1.654 344.721

pulse time(s) V(m/s)
# 1598737570 R_1598737570_Aug29g.wav
1 0.090 344.659
2 0.232 344.690
3 0.379 344.689
4 0.521 344.709
5 0.662 344.723
6 0.810 344.753
7 0.951 344.748
8 1.093 344.748
9 1.240 344.739
10 1.381 344.733
11 1.523 344.770
12 1.654 344.770

#}
