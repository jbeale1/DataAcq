# analyze data file to find channel delay via cross-correlation peak
# J.Beale  2019-09-07
# source("/home/john/magfield/mag-analyze2.m");

pkg load signal
format short    # default precision mode, 5 digits

# clock() = [ Y M D H M S ] # all integers, except seconds
cStart = clock();
dnStart = datenum(cStart); # day number from Year 0000
dstr = datestr(dnStart);

# fstart = [ 2019 09 07 14 13 (23.335) ]; # 190907-MagTest1 start
# fstart = [ 2019 09 07 17 49 35.359631 ]; # 190907-MagTest2 start
fstart = [ 2019 09 08 13 25 22.412079 ]; # 190908-MagTest1 start
#fname = "/home/john/magfield/190907-MagTest2.csv";  # input data
fname = "/home/john/magfield/190908-MagTest1.csv";  # input data
fout = "/home/john/magfield/190908-speeds.csv"; # output data

fid = fopen (fout, "w");

fDnum = datenum(fstart);  # file-start Day Number
fDstr = datestr(fDnum);
fprintf(fid,"enum, idx1,idx2, date, xcor, mph\n");
fprintf(fid,"# Runtime: %s\n",dstr);
fprintf(fid,"# Data filename: %s\n",fname);
fprintf(fid,"# Data start: %s\n",fDstr);

# ------------------------------------------------------------------
sample_rate = 7.457716;  # LTC2400 ADC sample rate, Hz
upsample_ratio = 10;   # upsample raw data to resolve xcorr peak
xwindow = 40;          # window size: samples around each event
xstart=3;           # ignore nose/data before this line
rThresh = 0.002;        # threshold on xcorr peak value for valid event
pkThresh = 0.005;       # initial raw peak threshold
sPolarity = -1;         # fudge factor of 1 or -1 if sensor order switched

off1=30;          # low side window offset from filtered peak center
off2=10;          # high side window offset from filtered peak center

# ------------------------------------------------------------------

# read data record of both sensors
raw = csvread(fname,[xstart,0,1e10,1]);
[xsize cols] = size(raw);
# ---

sum=raw(:,1) + raw(:,2);  # blindly add ch1 and ch2 together
[b, a] = butter(2, 0.05); # lowpass filter
filt = filter(b,a,abs(sum)); # smooth out combined data

# simple peak-find based on ch.1 + ch.2
warning ("off");
[pks2 idx2] = findpeaks(max(0,filt),
    "MinPeakHeight",pkThresh,"MinPeakDistance",10,"MinPeakWidth",8);
warning ("on");
t=1:rows(raw);
hours = t/(60*60*sample_rate);

#for i = (1 : rows(idx2));
#  c = idx2(i);
#  plot(raw(c-30:c+10,:)); axis tight;
#  input(' ');
#endfor
#break;

#plot(hours,lfilt,hours(idx2),lfilt(idx2),'or'); axis tight;
#xlabel("hours"); ylabel("log strength");


# ======================
#s = tf('s');
#g = 1/(2*s^2+3*s+4);
#h = impulse(g);

eventIdx = 0;  # keep track of each event
xs = xwindow; # x step size (= or smaller than window size)
for i = (1 : rows(idx2) );
  c = idx2(i);
  cstart = c - off1;
  cend = c + off2;
  if (cstart < 1) || (cend > xsize)
    # printf("i=%d start=%d end=%d, xsz=%d\n",i,cstart,cend,xsize);
    break;
  endif # too close to start or end
  s1=raw(cstart:cend,:);  # slice of full record with this event
  s1r = resample(s1,upsample_ratio,1); # n:1 interpolation from both sensors
  [R, lag] = xcorr(s1r(:,1),s1r(:,2));  # cross-correlation between 2 sensors
  [rpk, ridx] = max(R);  # if rpk is too low => no real event

  if (rpk > rThresh)
    [pk idx] = max(abs(s1));  # ch1,2 peak and index
    PkIdx1 = idx(1) + (c-off1); # off by 1?
    PkIdx2 = idx(2) + (c-off1); # off by 1?
    # --- calculate speed given xcorr value
    [e1 e2] = size(R);
    lead = ceil(e1/2) - ridx;  # ans = 89 (samples by which ch1 leads ch2)
    dt = lead / (sample_rate * upsample_ratio); # seconds by which ch1 leads ch2)
    dist = 7.5 + 4.6 - 0.01; # 12.09 meters
    if (abs(lead)>1)
      speed_ms = dist / dt;  # meters per second
      speed_mph = sPolarity * speed_ms * 2.23694;
    else
      speed_mph = 1E6;
    endif
    if (abs(speed_mph) < 65) # don't be ridiculous
      eventIdx += 1;
      start_min = PkIdx1 / (sample_rate * 60);
      # start_min = PkIdx2 / (sample_rate * 60);
      eventDnum = fDnum + start_min / (60*24);  # day number of this event
      eventDstr = datestr(eventDnum); # Y M D H:M:S format
      fprintf(fid,"%02d, %d,%d, %s, %05.2f, %+05.2f\n",
         eventIdx,PkIdx1,PkIdx2,eventDstr,rpk,speed_mph);
      evtspeed(eventIdx) = speed_mph;  # remember this event's speed
      evtdate(eventIdx) = eventDnum;
    endif
  endif
endfor

fclose (fid);  # close output file

# plot histogram
evtStart = evtdate(1);
evtDays = evtdate - evtStart;
totalDays = evtDays(end:end);
lfilt=log(abs(filt)+0.001);

subplot(2,1,1) # event histogram (1 bar = 30 minutes)
hist(evtDays,totalDays*48); axis tight; grid on;
xlabel("days (1 bar = 30 minutes)"); ylabel("traffic");

bins=6:1:50;
subplot(2,1,2) # speed histogram (1 bar = 1 mph)
hist(abs(evtspeed),bins); axis tight; grid on;
xlabel("mph"); ylabel("vehicles");
caption1 = sprintf("Duration = %4.2f days",totalDays);
caption2 = sprintf("Traffic = %d vehicles",eventIdx);
text(6, 80, caption1);
text(6, 60, caption2);

# -----------------------------------------------
#{
fname = "/home/john/magfield/190909-raw500_3.csv";
raw = csvread(fname);
r1 = reshape(raw',1,[]);
# ps = periodogram(r1,[],8192,500);
sf=500; sf2 = sf/2;
[b1,a1]=pei_tseng_notch( [60/sf2, 120/sf2, 179.9/sf2, 200/sf2],
  [4/sf2, 2/sf2, 2/sf2, 3/sf2]); # notch a few peaks
f1 = filter(b1,a1,r1);
std(f1)
[b2,a2] = butter(4, 50/sf2); # just 4-pole rolloff above 50 Hz
f2 = filter(b2,a2,f1);
[psf wf] = periodogram(f2,[],8192,500);
plot(wf,log(psf));
std(f2)

#}

#{
sensor1 time at drvway  R-val   MPH       desc
10:45:59 AM  10:46:00 AM  0.17  -27.26   L slv 4D
10:46:05 AM  10:46:07 AM  10.89 +27.63   R gry crsovr
10:47:04 AM  10:47:06 AM  4.5   +25.53   R slvr crsovr
10:48:16 AM  10:48:18 AM  2.93  +30.56   R grn jeep
10:48:37 AM  10:48:38 AM  0.01  -15.88   L wht truck/van
10:49:09 AM  10:49:11 AM  3.87  +23.45   R blu SUV
10:49:22 AM  10:49:24 AM  19.98 +29.66   R slvr 4D
10:49:32 AM  10:49:34 AM  5.83  +30.10   R wht 4D
10:50:06 AM  10:50:08 AM  0.68  +17.39   R red pickup
10:50:18 AM  10:50:20 AM  5.99  +26.19   R grey SUV
10:51:23 AM  10:51:25 AM  3.6   +24.3    R gry 4D
10:52:26 AM  10:52:27 AM  0.36  -31.03   L slvr 4D
10:53:19 AM  10:53:20 AM  0.11  -27.63   L gry 4D
10:53:23 AM  10:53:24 AM  0.65  -28.81   L red SUV
10:53:45 AM  10:43:46 AM  0.37  -34.77   L slvr 4D
10:56:11 AM  10:56:13 AM  5.47  +26.54   R wht pickup
10:56:40 AM  10:56:43 AM  10.8  +21.23   R blk SUV
10:57:50 AM  10:57:52 AM  9.74  +16.95   R grey 4D
10:58:59 AM  10:59:01 AM  2.8   +26.89   R turq jeep
11:00:56 AM  11:00:57 AM  0.29  -33.06   L slvr SUV
11:03:33 AM  11:03:34 AM  0.03  -13.27   L FedEx Trk
11:03:39 AM  11:03:41 AM  0.07  -21.01   L blk crsovr
11:05:04 AM  11:05:06 AM  0.17  -26.19   L wht 4D
11:05:53 AM  11:05:55 AM  4.87  +30.56   R wht prius
11:08:14 AM  11:08:15 AM  0.3   -28.81   L slvr 4D
11:09:52 AM  11:09:53 AM  0.2   -28.81   L dkrd 4D
11:12:08 AM  11:12:10 AM  0.13  -34.77   L red 4D
11:14:03 AM  11:14:05 AM  0.68  +24.01   R white 4D
11:14:53 AM  11:14:55 AM  0.76  +29.23   R drk crsover
11:15:03 AM  11:15:05 AM  4.98  +26.19   R silver crsovr
11:16:19 AM  11:16:21 AM  0.04  -33.62   L red crossover
11:16:34 AM  11:16:36 AM  5.94  +22.16   R silver 4D
11:17:55 AM  11:17:57 AM  4.73  +24.90   R silver crossover
11:18:04 AM  11:18:05 AM  0.25  -31.03   L grey crossover
11:18:27 AM  11:18:29 AM  0.32  +26.89   R grey prius
11:21:26 AM  11:21:28 AM  3.22  +26.89   R black 4D
11:22:01 AM  11:22:02 AM  0.87  -31.51   L silver 4D
11:24:02 AM  11:24:03 AM  0.16  -31.51   L silver compact
11:26:00 AM  11:26:01 AM  0.03  -21.69   L dark 4D
11:27:09 AM  11:27:17 AM  6.38  +25.21   R silver 4D
11:31:14 AM  11:31:15 AM  16.88 +22.66   R silver 4D
11:32:08 AM  11:32:10 AM  1.27  +30.56   R black pickup
11:35:01 AM  11:35:00 AM  5.02  -87.69   error:L truck + 2 R cars
11:35:59 AM  11:36:02 AM  1.00  +28.81   R wht 4D
#}
