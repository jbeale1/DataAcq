# analyze data file to find channel delay via cross-correlation peak
# J.Beale  2019-09-06
# source("/home/john/magfield/mag-analyze1.m");

pkg load signal
raw2ch = csvread("/home/john/magfield/190906-MagSenseRd_1137am.csv",[1,0,73000,3]);
dat1 = raw2ch(:,1);
dat2 = raw2ch(:,2);
t1=dat1(27300:27400);
t2=dat2(27300:27400);

t1r=resample(t1,10,1);
t2r=resample(t2,10,1);
sf = (max(t2r)-min(t2r)) / (max(t1r)-min(t1r));
t1rs = sf * t1r;  # rescale amplitude to sort-of match

[R, lag] = xcorr(t1rs,t2r);
plot(R);
[rpk, ridx] = max(R);
[R1, lag1] = xcorr(t1rs,t1rs);
[rpk1, ridx1] = max(R1);
[e1 e2] = size(R1);
ceil(e1/2) - ridx  # ans = 89 (samples by which ch1 leads ch2)
