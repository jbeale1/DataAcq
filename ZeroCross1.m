 
 # https://octave.sourceforge.io/signal/function/zerocrossing.html
 # example for finding zero crosings
 
 pkg load signal
 
 fname = "C:/Users/beale/Documents/STM32F4/2023-June10-Interferometer-6.csv";
 fnameOut = "C:/Users/beale/Documents/STM32F4/test-out.csv";

 dat = dlmread(fname, ",", 1,0); # read CSV data, skip 1st row header line
# rows = size(dat,1);
 srate = 11; #samples per second
 numsamp = size(dat,1);
 #numsamp = 5000;  # how many samples to examine at once
 
 #y = rand(1,100)-0.5;
 y = dat(1:numsamp,1)' # take only first column
 elems = size(y,2)
 x = linspace(0,(elems/srate),elems);
 
 dcLevel = mean(y);
 y1 = y - dcLevel;  # get 0-centered signal
 
 x0= zerocrossing(x,y1);
 y0 = interp1(x,y1,x0);
 figure (1);
 plot(x,y1,x0,y0,'x') # data with zero crossings marked
 
 dx0 = [ 0 diff(x0)'];
 x01 = x0';
 figure(2);
 plot(x01, dx0)  # separation of zero crossings
