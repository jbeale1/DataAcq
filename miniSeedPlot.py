# display spectra of MiniSEED time series (eg. infrasound)
# 17-Feb-2025 J.Beale

import numpy as np
import obspy
import matplotlib.pyplot as plt
from scipy.signal import spectrogram

def printMetaData(trace):
    print(f"Network: {trace.stats.network}")
    print(f"Station: {trace.stats.station}")
    print(f"Location: {trace.stats.location}")
    print(f"Channel: {trace.stats.channel}")
    print(f"Start Time: {trace.stats.starttime}")
    print(f"End Time: {trace.stats.endtime}")
    print(f"Sampling Rate: {trace.stats.sampling_rate} Hz")
    print(f"Number of Samples: {trace.stats.npts}")
    print(f"Data Format: {trace.stats.mseed['dataquality']}")
    print("----------------------")

def boxcar_average(data, n):
    """
    Computes the boxcar average of a 1D NumPy array.

    Args:
        data (np.ndarray): The input 1D array.
        window_size (int): The size of the moving window.

    Returns:
        np.ndarray: An array containing the boxcar average values. 
                    Returns an empty array if window_size is invalid.
    """
    elems = len(data)
    output = np.zeros_like(data)
    pad = int(np.floor(n/2))
    #print("pad = %d" % pad)

    if not isinstance(data, np.ndarray) or data.ndim != 1:
        raise ValueError("Input data must be a 1D NumPy array.")
    
    if not isinstance(n, int) or n <= 0 or n > len(data):
      raise ValueError("Window size must be a positive integer less than or equal to the length of the data.")
    
    csum = np.cumsum(data, dtype=float)
    csum[n:] = csum[n:] - csum[:-n]
    output[pad:-pad] = csum[n - 1:] / n
    output[0:pad] = output[pad]
    output[elems-pad:] = output[elems-pad-1]
    return output

def apply_window_fft(data, window_type='hann'):
    """
    Applies a windowing function to the input data and then computes the FFT.
    Args:
        data (numpy.ndarray): The input data array.
        window_type (str, optional): The type of window to apply 
                                     ('hann', 'hamming', 'blackman', or None). 
                                     Defaults to 'hann'.
    Returns:
        numpy.ndarray: The FFT of the windowed data.
    """
    n = len(data)
    
    if window_type == 'hann':
        window = np.hanning(n)
    elif window_type == 'hamming':
        window = np.hamming(n)
    elif window_type == 'blackman':
        window = np.blackman(n)
    elif window_type is None:
      window = 1
    else:
        raise ValueError("Invalid window type. Choose from 'hann', 'hamming', 'blackman', or None.")
    
    windowed_data = data * window
    fft_result = np.fft.fft(windowed_data)
    return fft_result

def plotSpec(vector, sampleRate):
    plt.figure(figsize=(12, 6))

    #frequencies, times, spectrogram_data = spectrogram(vector, fs=sampleRate)

    # Spectrogram parameters
    NFFT = 2048
    noverlap = 512


    # Calculate spectrogram
    Pxx, freqs, bins, im = plt.specgram(vector, NFFT=NFFT, Fs=sampleRate, noverlap=noverlap, vmin=-20)

    # Set the minimum intensity of the colorbar
    #cbar = plt.colorbar(im)
    #cbar.ax.get_yaxis().labelpad = 15
    #cbar.ax.set_ylabel('Intensity (dB)', rotation=270)
    #im.set_clim(vmin=-40)

    # Plot the spectrogram
    # plt.pcolormesh(times, frequencies, 10 * np.log10(spectrogram_data), shading='gouraud')  # Convert to dB scale
    plt.xlabel('Time (s)')    
    plt.ylabel('Frequency (Hz)')    
    plt.title('Spectrogram')
    plt.colorbar(label='Intensity (dB)')
    plt.yscale('log')  # Set y-axis to logarithmic scale
    plt.ylim(0.5, 50)    
    plt.show()


def plotLogLog(vector,sampleRate,traceLabel):    
    global ax
    fft_result = np.fft.fft(vector)    
    #fft_result = apply_window_fft(vector, window_type = 'hamming')  # using windowing function
    magnitude1 = np.abs(fft_result)
    bSize = 201 # boxcar-filter size
    magnitude = boxcar_average(magnitude1, bSize)
    frequencies = np.fft.fftfreq(len(vector),d=(1.0/sampleRate))
    # print(frequencies)
    size = len(frequencies)
    magnitude /= np.sqrt(size)  # normalize?
    rs = int(size/2) # only use the positive real frequencies
    #plt.loglog(np.abs(frequencies), magnitude)
    ax.loglog(frequencies[bSize:rs], magnitude[bSize:rs], label=traceLabel)

def plot_miniseed(file_path):
    global ax
    try:
        st = obspy.read(file_path)
        if not st:
            raise Exception(f"No data found in file: {file_path}")
    except Exception as e:
         print(f"Error reading or processing file: {e}")
         return

    fig, ax = plt.subplots(1,1, figsize=(10,6))
    for trace in st:
        tString = "Infrasound Spectrum  %s.%s.%s.%s" % ( 
            trace.stats.network,
            trace.stats.station,
            trace.stats.location,
            trace.stats.channel
        )
        times = trace.times("matplotlib")
        # print(trace.data)
        sampleRate = trace.stats.sampling_rate
        points = len(trace.data)
        pDec = int(points / frac)
        print("Number of points: %d" % points)
        printMetaData(trace)
        for i in range(frac):
            plotLogLog(trace.data[(pDec*i):pDec*(i+1)],sampleRate,i+1)
            # plotSpec(trace.data[(pDec*i):pDec*(i+1)], sampleRate) # spectrogram


        plt.xlabel("Frequency (Hz)")
        plt.ylabel("Magnitude")
        plt.title(tString)
        plt.legend()
        plt.xlim(1E-2, 1E1)  # Set x-axis limits 
        plt.ylim(1E2, 1E6)
        #plt.ylim(1E5, 1E9)
        #plt.ylim(5E-2, 2E2)
        text1 = "Start: %s" % trace.stats.starttime
        plt.text(.053, .15, text1, transform=ax.transAxes)
        text2 = "End:   %s" % trace.stats.endtime
        plt.text(.053, .11, text2, transform=ax.transAxes)
        plt.grid('both')
        plt.minorticks_on()
        plt.grid(which='minor', linestyle=':', alpha=0.5)
        plt.show()

    return


if __name__ == "__main__":
    frac = 1 # divide full dataset into this many parts

    file_path = r"C:\Users\beale\Documents\Tiltmeter\AM.RC93C.00.HDF.D.2025.047"
    #file_path = r"C:\Users\beale\Documents\Tiltmeter\AM.RC93C.00.HDF.D.2025.048"
    #file_path = r"C:\Users\beale\Documents\Tiltmeter\AM.RC93C.00.HDF.D.2025.049"
    #file_path = input("Enter the path to the MiniSEED file: ")
    plot_miniseed(file_path)
    
