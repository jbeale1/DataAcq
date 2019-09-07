# analyze data file to find channel delay via cross-correlation peak
# J.Beale  2019-09-07
# source("/home/john/magfield/mag-analyze2.m");

pkg load signal
format short    # default precision mode, 5 digits

# clock() = [ Y M D H M S ] # all integers, except seconds
cStart = clock();
dnStart = datenum(cStart); # day number from Year 0000
dstr = datestr(dnStart);

fstart = [ 2019 09 06 10 45 45 ]; # file start time
fDnum = datenum(fstart);  # file-start Day Number
fDstr = datestr(fDnum);
printf("Runtime: %s  File start: %s\n",dstr,fDstr);


# ------------------------------------------------------------------
sample_rate = 7.457716;  # LTC2400 ADC sample rate, Hz
upsample_ratio = 10;   # upsample raw data to resolve xcorr peak
xwindow = 40;          # window size: samples around each event
xstart=7000;           # ignore nose/data before this line
rThresh = 0.10;        # threshold on xcorr peak value for valid event
fname = "/home/john/magfield/190906-MagSenseRd_1137am.csv";  # input data
off1=30;          # low side window offset from filtered peak center
off2=10;          # high side window offset from filtered peak center

# from xstart, 2019-09-06 10:45:45 - 11:37 am PDT
# duration:  51.455 minutes (23024 samples at 7.4577 Hz)
# ------------------------------------------------------------------

# read data record of both sensors
raw = csvread(fname,[xstart,0,1e10,1]);
[xsize cols] = size(raw);
# ---

sum=raw(:,1) + raw(:,2);  # blindly add ch1 and ch2 together
[b, a] = butter(2, 0.05); # lowpass filter
filt = filter(b,a,abs(sum)); # smooth out combined data

warning ("off");
#[pks idx] = findpeaks(raw(:,1),
#  "MinPeakHeight",0.005,"MinPeakDistance",20,"MinPeakWidth",8,"DoubleSided");
[pks2 idx2] = findpeaks(max(0,filt),
    "MinPeakHeight",0.010,"MinPeakDistance",10,"MinPeakWidth",8);
#[pks3 idx3] = findpeaks(sum,
#    "MinPeakHeight",0.005,"MinPeakDistance",40,"MinPeakWidth",8,"DoubleSided");
warning ("on");
t=1:rows(raw);

#for i = (1 : rows(idx2));
#  c = idx2(i);
#  plot(raw(c-30:c+10,:)); axis tight;
#  input(' ');
#endfor
#break;

#subplot(2,1,1)
#plot(t,raw(:,1),t(idx),raw(idx,1),'or'); axis tight;
#subplot(2,1,2)
plot(t,filt,t(idx2),filt(idx2),'or'); axis tight;
#rows(pks)   # how many peaks found
#rows(pks2)
#break;

eventIdx = 0;  # keep track of each event
xs = xwindow; # x step size (= or smaller than window size)
for i = (1 : rows(idx2) );
  c = idx2(i);
  s1=raw(c-off1:c+off2,:);  # slice of full record with this event
  s1r = resample(s1,upsample_ratio,1); # n:1 interpolation from both sensors
  [R, lag] = xcorr(s1r(:,1),s1r(:,2));  # cross-correlation between 2 sensors
  [rpk, ridx] = max(R);  # if rpk is too low => no real event

  if (rpk > rThresh)
    eventIdx += 1;
    [pk idx] = max(abs(s1));  # ch1,2 peak and index
    PkIdx1 = idx(1) + (c-off1); # off by 1?
    PkIdx2 = idx(2) + (c-off1); # off by 1?
    # --- calculate speed given xcorr value
    [e1 e2] = size(R);
    lead = ceil(e1/2) - ridx;  # ans = 89 (samples by which ch1 leads ch2)
    dt = lead / (sample_rate * upsample_ratio); # seconds by which ch1 leads ch2)
    dist = 7.5 + 4.6 - 0.01; # 12.09 meters
    speed_ms = dist / dt;  # meters per second
    speed_mph = speed_ms * 2.23694;
    start_min = PkIdx2 / (sample_rate * 60);
    eventDnum = fDnum + start_min / (60*24);  # day number of this event
    eventDstr = datestr(eventDnum); # Y M D H:M:S format
    printf("%02d : %s  Rpk = %05.2f : %+05.2f mph\n",
       eventIdx,eventDstr,rpk,speed_mph);
    # printf("Xcor: %3.1f  Speed: %5.2f mph\n",rpk,speed_mph);
    # break;

  endif

endfor
# -----------------------------------------------

#{
sensor1 time at drvway	R-val	 MPH	     desc
10:45:59 AM	10:46:00 AM	0.17	-27.26   L slv 4D
10:46:05 AM	10:46:07 AM	10.89	+27.63   R gry crsovr
10:47:04 AM	10:47:06 AM	4.5	  +25.53   R slvr crsovr
10:48:16 AM	10:48:18 AM	2.93 	+30.56   R grn jeep
          	10:48:38 AM			           L wht truck/van
10:49:09 AM	10:49:11 AM	3.87	+23.45	 R blu SUV
10:49:22 AM	10:49:24 AM	19.98	+29.66	 R slvr 4D
10:49:32 AM	10:49:34 AM	5.83	+30.10   R wht 4D
10:50:06 AM	10:50:08 AM	0.68	+17.39	 R red pickup
10:50:18 AM	10:50:20 AM	5.99	+26.19	 R grey SUV
10:51:23 AM	10:51:25 AM	3.6	  +24.3	   R gry 4D
10:52:26 AM	10:52:27 AM	0.36	-31.03	 L slvr 4D
10:53:19 AM	10:53:20 AM	0.11	-27.63	 L gry 4D
10:53:23 AM	10:53:24 AM	0.65	-28.81	 L red SUV
10:53:45 AM	10:43:46 AM	0.37	-34.77	 L slvr 4D
10:56:11 AM	10:56:13 AM	5.47	+26.54	 R wht pickup
10:56:40 AM	10:56:43 AM	10.8	+21.23	 R blk SUV
10:57:50 AM	10:57:52 AM	9.74	+16.95	 R grey 4D
10:58:59 AM	10:59:01 AM	2.8	  +26.89	 R turq jeep
11:00:56 AM	11:00:57 AM	0.29	-33.06	 L slvr SUV
           	11:03:34 AM			           L FedEx Trk
	          11:03:41 AM			           L blk crsovr
11:05:04 AM	11:05:06 AM	0.17	-26.19	 L wht 4D
11:05:53 AM	11:05:55 AM	4.87	+30.56	 R wht prius
11:08:14 AM	11:08:15 AM	0.3	  -28.81	 L slvr 4D
11:09:52 AM	11:09:53 AM	0.2	  -28.81	 L dkrd 4D
11:12:08 AM	11:12:10 AM	0.13	-34.77	 L red 4D
11:14:03 AM	11:14:05 AM	0.68	+24.01	 R white 4D
11:14:53 AM	11:14:55 AM	0.76	+29.23	 R drk crsover
11:15:03 AM	11:15:05 AM	4.98	+26.19	 R silver crsovr
	          11:16:21 AM         			 L red crossover
11:16:34 AM	11:16:36 AM	5.94	+22.16	 R silver 4D
11:17:55 AM	11:17:57 AM	4.73	+24.90   R silver crossover
11:18:04 AM	11:18:05 AM	0.25	-31.03	 L grey crossover
11:18:27 AM	11:18:29 AM	0.32	+26.89	 R grey prius
11:21:26 AM	11:21:28 AM	3.22	+26.89	 R black 4D
11:22:01 AM	11:22:02 AM	0.87	-31.51	 L silver 4D
11:24:02 AM	11:24:03 AM	0.16	-31.51	 L silver compact
          	11:26:01 AM         			 L dark 4D
11:27:09 AM	11:27:17 AM	6.38	+25.21	 R silver 4D
11:31:14 AM	11:31:15 AM	16.88	+22.66	 R silver 4D
11:32:08 AM	11:32:10 AM	1.27	+30.56	 R black pickup
11:35:01 AM	11:35:00 AM	5.02	-87.69	 error:L truck + 2 R cars
11:35:59 AM	11:36:02 AM	1.00  +28.81	 R wht 4D
#}
