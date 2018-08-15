# Octave / Matlab code to read spectrogram of doppler sensor
# J.Beale 2018-08-14

pkg load image   % load in image processing package
%I = imread ("~/Pictures/Doppler_2018-08-14 08-28-03.png");
%I = imread ("~/Pictures/Doppler-car-2018-08-14 13-49-44.png");
%I = imread ("~/Pictures/Doppler_2018-08-08_a.png");
%I = imread ("~/Pictures/SlowCar_2018-08-14_23-33-17.png");
I = imread ("~/Pictures/CarPass_2018-08-14_23-34-51.png");


J = 214 - I;        % white signal on black background
J1 = rgb2gray(J);   % make it single-channel greyscale

J2 = conv2 (J1, ones (3, 9) / 6000, "same");  % 5x5 filter
J3 = conv2 (J2, ones (3, 5) / 10, "same");    % 5x5 filter again
csum = sum(J3)';             % sum each column
[mxv,idx] = max(J3,[],1);   % find index of max value on each column
figure (1);
subplot(3,1,1);     % plots stacked, do top:
plot(300-idx);      % plot peak-index trace in a second window
% figure (2);
subplot(3,1,2);     % plots stacked, do middle:
plot(csum);         % plot of total energy at each time point
subplot(3,1,3);     % plots stacked, do middle:
imshow(J3);    

J2 = conv2 (J1, ones (9, 3) / 6000, "same");  % 5x5 filter
J3 = conv2 (J2, ones (5, 3) / 10, "same");    % 5x5 filter again
csum = sum(J3)';             % sum each column
[mxv,idx] = max(J3,[],1);   % find index of max value on each column
figure (2);
subplot(3,1,1);     % plots stacked, do top:
plot(300-idx);      % plot peak-index trace in a second window
% figure (2);
subplot(3,1,2);     % plots stacked, do middle:
plot(csum);         % plot of total energy at each time point
subplot(3,1,3);     % plots stacked, do middle:
imshow(J3);    
