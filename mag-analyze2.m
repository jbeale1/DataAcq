# analyze data file to find channel delay via cross-correlation peak
# J.Beale  2019-09-06
# source("/home/john/magfield/mag-analyze2.m");

pkg load signal
format short    # default precision mode, 5 digits

# ------------------------------------------------------------------
sample_rate = 7.457716;  # LTC2400 ADC sample rate, Hz
upsample_ratio = 10;   # upsample raw data to resolve xcorr peak
xwindow = 80;          # window size: samples around each event
xstart=7000;           # ignore nose/data before this line
rThresh = 0.10        # threshold on xcorr peak value for valid event
fname = "/home/john/magfield/190906-MagSenseRd_1137am.csv"  # input data
# from xstart, data covers 10:46 - 11:37 am PDT 2019-09-06
# duration:  51.455 minutes (23024 samples at 7.4577 Hz)
# last event: 11:35 am sample# 22001, 49.17 min idx=833 Rpk = 05.01 : -87.69 mph
# rThresh 0.10, step 80: 36 events (all true)
# rThresh 0.04, step 80: 40 events
# rThresh 0.02, step 80: 44 events (7 false)
# ------------------------------------------------------------------

# read entire data record of both sensors
raw = csvread(fname,[xstart,0,1e10,1]);
[xsize cols] = size(raw);
# ---

# x=1065;
eventIdx = 0;  # keep track of each event
xs = xwindow; # x step size (= or smaller than window size)
for x = (1 : xs : xsize-xs-1);

  s1=raw(x:x+xwindow,:);  # active slice of full record to search for event
  s1r = resample(s1,upsample_ratio,1); # n:1 interpolation from both sensors
  [R, lag] = xcorr(s1r(:,1),s1r(:,2));  # cross-correlation between 2 sensors
  [rpk, ridx] = max(R);  # if rpk is too low => no real event

  if (rpk > rThresh)
    eventIdx += 1;
    # --- calculate speed given xcorr value
    [e1 e2] = size(R);
    lead = ceil(e1/2) - ridx;  # ans = 89 (samples by which ch1 leads ch2)
    dt = lead / (sample_rate * upsample_ratio); # seconds by which ch1 leads ch2)
    dist = 7.5 + 4.6 - 0.01; # 12.09 meters
    speed_ms = dist / dt;  # meters per second
    speed_mph = speed_ms * 2.23694;
    start_min = x / (sample_rate * 60);
    printf("%02d : %05d=%5.2f min idx=%d Rpk = %05.2f : %+05.2f mph\n",
       eventIdx,x,start_min,ridx,rpk,speed_mph);
    # printf("Xcor: %3.1f  Speed: %5.2f mph\n",rpk,speed_mph);

  endif

endfor

# ==================================================
# useful threshold: abs(dat) > 0.01  (good "car" detector for ch.1)
# ch1: largest actual car pk-pk: 4.0V  smallest: 0.015 V (?)
# ch1: background: stdev=5.8E-4 pk-pk=2.6E-3  (mostly)

#{
01 : 00081= 0.18 min idx=737 Rpk = 10.89 : +27.63 mph 10:46:07 R gry crsovr
02 : 00561= 1.25 min idx=731 Rpk = 04.50 : +25.53 mph 10:47:06 R slvr crsovr
03 : 01121= 2.51 min idx=745 Rpk = 01.91 : +31.03 mph 10:48:18 R grn jeep
     01241= 2.77 min idx=943 Rpk = 00.01 : -15.16 mph 10:48:38 L wht truck/van
04 : 01441= 3.22 min idx=728 Rpk = 02.29 : +24.60 mph 10:49:11 R blu SUV
05 : 01601= 3.58 min idx=742 Rpk = 19.98 : +29.66 mph 10:49:24 R slvr 4D
06 : 01681= 3.76 min idx=743 Rpk = 05.83 : +30.10 mph 10:49:34 R wht 4D
07 : 01921= 4.29 min idx=694 Rpk = 00.69 : +17.39 mph 10:50:08 R red pickup
08 : 02001= 4.47 min idx=733 Rpk = 05.99 : +26.19 mph 10:50:20 R grey SUV
09 : 02481= 5.54 min idx=727 Rpk = 03.60 : +24.30 mph 10:51:25 R gry 4D
     02521= 5.63 min idx=857 Rpk = 00.04 : -42.91 mph 10:52:27 L slvr 4D
10 : 02961= 6.62 min idx=875 Rpk = 00.36 : -31.03 mph 10:53:20 L gry 4D
     03321= 7.42 min idx=883 Rpk = 00.10 : -27.63 mph ?
11 : 03361= 7.51 min idx=880 Rpk = 00.76 : -28.81 mph 10:53:24 L red SUV
12 : 03521= 7.87 min idx=868 Rpk = 00.37 : -34.77 mph 10:43:46 L slvr 4D
13 : 04641=10.37 min idx=734 Rpk = 05.47 : +26.54 mph 10:56:13 R wht pickup
14 : 04881=10.91 min idx=717 Rpk = 09.65 : +21.69 mph 10:56:43 R blk SUV
15 : 05361=11.98 min idx=691 Rpk = 09.74 : +16.95 mph 10:57:52 R grey 4D
16 : 05921=13.23 min idx=736 Rpk = 00.56 : +27.26 mph 10:59:01 R turq jeep
                                                      11:00:57 L slvr SUV
                                                      11:03:34 L FedEx Trk
17 : 06721=15.02 min idx=871 Rpk = 00.25 : -33.06 mph 11:03:41 L blk crsovr
18 : 08641=19.31 min idx=887 Rpk = 00.17 : -26.19 mph 11:05:06 L wht 4D
19 : 08961=20.03 min idx=744 Rpk = 04.87 : +30.56 mph 11:05:55 R wht prius
20 : 10001=22.35 min idx=880 Rpk = 00.30 : -28.81 mph 11:08:15 L slvr 4D
21 : 10721=23.96 min idx=880 Rpk = 00.18 : -28.81 mph 11:09:53 L dkrd 4D
22 : 11761=26.28 min idx=868 Rpk = 00.13 : -34.77 mph 11:12:10 L red 4D
23 : 12641=28.25 min idx=726 Rpk = 00.68 : +24.01 mph 11:14:05 R white 4D
24 : 12961=28.97 min idx=742 Rpk = 00.57 : +29.66 mph 11:14:55 R drk crsover
25 : 13041=29.14 min idx=733 Rpk = 04.97 : +26.19 mph 11:15:05 R silver crsovr
  x  13681=30.57 min idx=867 Rpk = 00.03 : -35.38 mph 11:16:21 L red crossover
26 : 13761=30.75 min idx=719 Rpk = 05.94 : +22.16 mph 11:16:36 R silver 4D
27 : 14321=32.00 min idx=729 Rpk = 04.70 : +24.90 mph 11:17:57 R silver crossover
28 : 14401=32.18 min idx=875 Rpk = 00.25 : -31.03 mph 11:18:05 L grey crossover
29 : 14561=32.54 min idx=735 Rpk = 00.32 : +26.89 mph 11:18:29 R grey prius
30 : 15921=35.58 min idx=735 Rpk = 03.22 : +26.89 mph 11:21:28 R black 4D
                                                      11:22:02 L silver 4D
31 : 16161=36.12 min idx=873 Rpk = 00.83 : -32.01 mph 11:24:03 L silver compact
32 : 17121=38.26 min idx=874 Rpk = 00.16 : -31.51 mph 11:26:01 L dark 4D
33 : 18481=41.30 min idx=730 Rpk = 06.38 : +25.21 mph 11:27:17 R silver 4D
34 : 20321=45.41 min idx=721 Rpk = 16.88 : +22.66 mph 11:31:15 R silver 4D
35 : 20721=46.31 min idx=744 Rpk = 01.27 : +30.56 mph 11:32:10 R black pickup
36 : 22001=49.17 min idx=833 Rpk = 05.01 : -87.69 mph 11:35:00 XX truck+2cars
#}
