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
#fstart = [ 2019 09 08 13 25 22.412079 ]; # 190908-MagTest1 start
#fstart = [ 2019 09 10 13 14 57.250011 ]; # 190910-MagTest1 start
# fstart = [ 2019 09 11 08 42 12.187210 ]; # 190911-MagTest1 start
#fstart = [ 2019 09 12 10 23 00.111578 ]; # 190912-MagTest1 start
fstart = [ 2019 09 13 23 55 20.472794 ]; # 190913-MagTest1 start

fdir = "/home/john/magfield/"
#fin_name = "190910-MagTest1.csv";  # input data
#fin_name = "190911-MagTest1.csv";  # input data
#fin_name = "190912-MagTest1.csv";  # input data
fin_name = "190913-MagTest1.csv";  # input data

fname = [fdir fin_name];
fout = [fdir fin_name(1:end-4) "_speeds.csv"];
printf("Writing to output file: %s\n",fout);

fid = fopen (fout, "w"); # open output results file

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
[xsize cols] = size(raw);  # total samples in record
totalDays = xsize / (sample_rate * 60 * 60 * 24); # total duration in days
fDEstr = datestr(fDnum+totalDays); # end time of file
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

hAxis1 = subplot(3,1,1); # raw signal vs. time, event detection
plot(hours,filt,hours(idx2),filt(idx2),'or'); axis tight;
xlabel("hours"); ylabel("log strength");


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
    if (abs(speed_mph) < 65) && (abs(speed_mph) > 4) # don't be ridiculous
      eventIdx += 1;
      start_min = PkIdx1 / (sample_rate * 60);
      eventDnum = fDnum + start_min / (60*24);  # day number of this event
      eventDstr = datestr(eventDnum); # Y M D H:M:S format
      fprintf(fid,"%02d, %d,%d, %06.4f, %s, %05.2f, %+05.2f\n",
         eventIdx,PkIdx1,PkIdx2,start_min/(60*24),eventDstr,rpk,speed_mph);
      evtspeed(eventIdx) = speed_mph;  # remember this event's speed
      evtdate(eventIdx) = eventDnum;
      # return; # DEBUG
    endif
  endif
endfor

fclose (fid);  # close output file
# -------------------------------------------------------

mSpeed = median(abs(evtspeed));  # overall median speed
moSpeed = mode(round(abs(evtspeed)));

# plot histogram
evtStart = fDnum; # day number of first sample in file
evtDays = evtdate - evtStart; # day number of each event from file start
lfilt=log(abs(filt)+0.001);

ehbins = floor(totalDays*48);
binsize = totalDays/ehbins;
xlpos = 0:binsize:totalDays;
binctrs = xlpos(1:end-1) + binsize/2;
hAxis2 = subplot(3,1,2); # event histogram (1 bar = 30 minutes)
# [ hN, hX ] = hist(evtDays,ehbins);
[ hN, hX ] = hist(evtDays,binctrs);
bar(hX,hN,1.0); axis tight; grid on;
xlabel("days (1 bar = 30 minutes)"); ylabel("traffic");
colormap(cool(64));
bars = columns(xlpos)-1; # better be same as ehbins
for i = (1:bars);
  evtdRel = evtdate - fDnum; # day number relative to start time
  idx = find( (evtdRel >= xlpos(i)) & (evtdRel < xlpos(i+1)) );
  if (columns(idx) > 0)
    sSlice = evtspeed(idx(1):idx(end));
    ms = median(abs(sSlice));
    as = mean(abs(sSlice));
  else
    sSlice = [];
    ms = 0;
    as = 0;
  endif
  medCap0 = sprintf("%02d",columns(sSlice)); # number of events in slice
  medCap1 = sprintf("%02d",floor(ms)); # median
  medCap2 = sprintf("%02d",floor(as)); # average or mean
  # xcpos = (i-0.5) * (totalDays/(bars)) ; # center on bar
  xcpos = hX(i);
   # median of data slice for this bar
  text(xcpos,25,medCap0,"horizontalalignment","center");
  text(xcpos,15,medCap1,"horizontalalignment","center");
  text(xcpos,5,medCap2,"horizontalalignment","center");
  printf("%d: (%5.4f,%5.4f) events: %d median: %d mean:%d \n",
    i,xlpos(i),xlpos(i+1),columns(sSlice),floor(ms),floor(as));
  #if (i == 24)
  #     return;
  #endif
endfor

bins=6:2:50;
hAxis3 = subplot(3,1,3); # speed histogram (1 bar = 1 mph)
[HistN, HistX] = hist(abs(evtspeed),bins);
bar(HistX,HistN,1.0); axis tight; grid on;
Ymax = max(HistN);  # max value of Y axis on plot
Xpos = HistX(1);  # X coord to start in-plot captions
xlabel("mph"); ylabel("vehicles");
caption0 = sprintf("%s",fname);
captionp5 = sprintf("Data start: %s",fDstr);
captionp6 = sprintf("Data end: %s",fDEstr);
caption1 = sprintf("Duration = %4.2f days",totalDays);
caption2 = sprintf("Traffic = %d vehicles",eventIdx);
caption3 = sprintf("Median = %5.2f mph",mSpeed);
caption4 = sprintf("Mode = %d mph",moSpeed);
text(Xpos, Ymax*0.85, caption0);
text(Xpos, Ymax*0.75, captionp5);
text(Xpos, Ymax*0.70, captionp6);
text(Xpos, Ymax*0.65, caption1);
text(Xpos, Ymax*0.55, caption2);
text(Xpos, Ymax*0.45, caption3);
text(Xpos, Ymax*0.35, caption4);

pos = get( hAxis1, 'Position' );
pos(2)=0.68; pos(4)=0.22; set( hAxis1, 'Position', pos);
pos2 = get( hAxis2, 'Position' );
pos2(2)=0.38; pos2(4)=0.24; set( hAxis2, 'Position', pos2);
pos3 = get( hAxis3, 'Position' );
pos3(2)=0.07; pos3(4)=0.24; set( hAxis3, 'Position', pos3);
# -----------------------------------------------
printf("%s\n%s\n",captionp5,captionp6);  # show start,end time/date
