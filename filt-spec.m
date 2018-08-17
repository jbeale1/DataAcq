% extract amplitude envelope of signal, smooth it
% tested in GNU Octave  2018-08-17 J.Beale

pkg load signal                            % need hilbert fn for envelope

dsfac = 400;         % factor by which to downsample envelope of input data
ne = 50;            % number of elements in FIR filter
fcut = 0.05;         % normalized cutoff of lowpass envelope filter

% [y,Fs] = audioread("180817_filter1.wav");  % load signal data
% [y1,Fs] = audioread("ChA_2018-08-17_07-45-00.wav");  % load signal data
[y1,Fs] = audioread("ChA_2018-08-17_14-00-00.wav");  % load signal data

y = y1(5E5:1E6);      % select just a portion of the full data
% y = y1;

x = abs(hilbert(y));             % get high-freq amplitude envelope
envh = accumarray(ceil((1:numel(x))/dsfac)',x(:),[],@mean);  % downsample 100 to 1

blo = fir1(ne,fcut,chebwin(ne+1)); % construct a FIR filter kernel
tmp = filter(blo,1,envh);
tln = numel(tmp);
envl = zeros(tln,1) + 0.001;
envl(1:(tln-(ne/2))) = tmp((ne/2)+1:tln);             % shift fwd in time, to realign filt. with original

step=ceil(20*Fs/1000);    # one spectral slice every 20 ms
window=ceil(100*Fs/1000); # 100 ms data window
[S, f, t] = specgram(y);

figure (1);
subplot(3,1,1);         % plots stacked, do top:
plot(y);                % have a look at the resulting envelope
axis(axis(),"tight");   % don't make x axis longer than data
subplot(3,1,2);         % plots stacked, do middle:
semilogy(envl);         % plot of total energy at each time point
axis(axis(),"tight");   % don't make x axis longer than data
subplot(3,1,3);         % plots stacked, do bottom:
specgram(y, 2^nextpow2(window), Fs, window, window-step); % show spectrogram
