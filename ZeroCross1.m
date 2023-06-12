 
 # https://octave.sourceforge.io/signal/function/zerocrossing.html
 # Find zero crosings, calculate interval
 
 pkg load signal
 
 fname = "/home/john/Documents/STM32/Interferometer-9.csv"
 # fnameOut = "C:/Users/beale/Documents/STM32F4/test-out.csv";

 dat = dlmread(fname, ",", 1,0); # read CSV data, skip 1st row header line
# rows = size(dat,1);
 srate = 11; #samples per second
 numsamp = floor(size(dat,1)/2);
 #numsamp = 100;  # how many samples to examine at once
 
 #y = dat(1:numsamp,1)'; # take only first column
 y = dat(1:numsamp,[1,3])'; # take both channels
 elems = size(y,2);
 x = linspace(0,(elems/srate),elems);
 
 dcLevel = mean(y,2); # average all rows
 y1 = y - dcLevel;  # get 0-centered signals
 figure (1);
# plot(x,y1) # data with zero crossings marked
 y1t = (y1 > 0);
 yState = y1t(1,:)*2 + y1t(2,:); # 0,1,2,3
 yState1 = [0 yState(1:end-1) ];
 pEdge = (yState==0) & (yState1 == 1);
 pEdge2 = (yState==1) & (yState1 == 0);

 eIndex = find(pEdge2); # 1 where element non-zero
 #plot(pEdge);
 
# time in samples between whole fringes
 iDiff = diff(eIndex);  
 plot(iDiff);

 #ysDiff = diff(yState);
 # plot(ysDiff);
 #b = [ yState; yState1 ];
 #plot(x,b);

 return;
 
 z1 = zerocrossing(x,y1(1,:));
 z2 = zerocrossing(x,y1(2,:));
 npair = min(size(z1,1), size(z2,1));
 x0 = [ z1(1:npair) z2(1:npair) ]';
 zdif = x0(1,:) - x0(2,:);
 # plot(zdif);
 
 y0 = [ interp1(x,y1(1,:),x0(1,:)) interp1(x,y1(2,:),x0(2,:)) ]';
 # figure (1);
 # plot(x,y1,x0,y0,'x') # data with zero crossings marked
 
 dx0 = [ 0 diff(x0(1,:)) ];
 x01 = x0';
 # figure(2);
# plot(x01, dx0);  # separation of zero crossing
