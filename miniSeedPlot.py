# display spectra of MiniSEED time series (eg. infrasound)
# 16-Feb-2025 J.Beale

import numpy as np
import obspy
import matplotlib.pyplot as plt

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

def boxcarAverage1(data, n):
    """
    Calculates the average of N neighboring elements for each element in a given array.     
    Args:
        data: Input array. 
        n: Number of neighbors to consider on each side (inclusive).    
    Returns:
        A new array with the calculated averages.
    """
    output = np.zeros_like(data)
    
    for i in range(len(data)):
        left_index = max(0, i - n)
        right_index = min(len(data), i + n + 1)
        neighbors = data[left_index:right_index]
        output[i] = np.mean(neighbors)
    
    return output

def plotLogLog(vector,sampleRate,traceLabel):    
    global ax
    fft_result = np.fft.fft(vector)    
    magnitude1 = np.abs(fft_result)
    bSize = 201 # boxcar-filter size
    magnitude = boxcar_average(magnitude1, bSize)
    frequencies = np.fft.fftfreq(len(vector),d=(1.0/sampleRate))
    # print(frequencies)
    size = len(frequencies)
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

    fig, ax = plt.subplots(1,1, figsize=(15,6))
    frac = 4 # divide full dataset into this many parts
    for trace in st:
        tString = "%s.%s.%s.%s" % ( trace.stats.network,
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

        plt.xlabel("Frequency (Hz)")
        plt.ylabel("Magnitude")
        plt.title(tString)
        plt.legend()
        text1 = "Start: %s" % trace.stats.starttime
        plt.text(.1, .24, text1, transform=ax.transAxes)
        text2 = "End:   %s" % trace.stats.endtime
        plt.text(.1, .2, text2, transform=ax.transAxes)
        plt.grid('both')
        plt.show()

    return


if __name__ == "__main__":

    file_path = r"C:\Users\beale\Documents\Tiltmeter\AM.RC93C.00.HDF.D.2025.047"
    #file_path = input("Enter the path to the MiniSEED file: ")
    plot_miniseed(file_path)
  
