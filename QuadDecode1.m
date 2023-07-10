 # Find net motion given A,B signals in quadrature 
 # eg. from split-field photodiode in interferometer
 # 10-July-2023 J.Beale
 
 pkg load signal
 
 fname = "/home/john/Documents/STM32/DualPD-Test16.csv";
 #fname = "/home/john/Documents/STM32/Interferometer-12.csv";
 fnameOut = "C:/Users/beale/Documents/STM32F4/test-out.csv";

 dat = dlmread(fname, ",", 1,0); # read CSV data, skip 1st row header line
# rows = size(dat,1);
 srate = 11; #samples per second
 numsamp = size(dat,1);
 # numsamp = floor(size(dat,1)/2);
 # numsamp = 16000;  # how many samples to examine at once
 
 #y = dat(1:numsamp,1)'; # take only first column
 y = dat(1:numsamp,[1,3])'; # take both channels
 elems = size(y,2);
 x = linspace(0,(elems/srate),elems);
 
 dcLevel = mean(y,2); # average all rows
 y1 = y - dcLevel;  # get 0-centered signals
 figure (1);
 
 y1t = (y1 > 0);  # threshold to form 0,1 binary signal
 Anew = [ 0 y1t(1,:) ]; # zero pad at start
 Bnew = [ 0 y1t(2,:) ];
 Aold = [ y1t(1,:) 0 ]; # zero pad at end
 Bold = [ y1t(2,:) 0 ];
 
 yState = Aold * 8 + Bold * 4 + Anew * 2 + Bnew + 1;  # index number in [1..16]
 sTable = [0 -1 1 0 1 0 0 -1 -1 0 0 1 0 1 -1 0];  # quadrature lookup table

 stepSeq = sTable(yState); # incremental motion, one of three values: -1, 0, 1
 pos = cumsum(stepSeq);  # position is sum of each step
 plot(pos);
 return;
 
# Description of quadrature decoder:
# http://makeatronics.blogspot.com/2013/02/efficiently-reading-quadrature-with.html
# word = oldA * 8 + oldB * 4 + newA * 2 + newB;  # <= A,B are binary vectors
