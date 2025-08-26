#!/usr/bin/env python3
"""
DFRobot SEN0366 Laser Rangefinder Serial Communication
Communicates with a laser rangefinder over serial port to read distance measurements.
Logs distance readings to a CSV file with timestamp, average, standard deviation, range, 
and trend.

"""

import serial
import time
import sys
import csv
import os
from collections import deque
from datetime import datetime
import statistics

class LaserRangefinder:
    def __init__(self, port, baudrate=9600, timeout=1.5):
        """
        Initialize the laser rangefinder connection.
        
        Args:
            port (str): Serial port name (e.g., 'COM3' on Windows, '/dev/ttyUSB0' on Linux)
            baudrate (int): Baud rate (default: 9600)
            timeout (float): Read timeout in seconds (default: 2)
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None
        
        # Command to read distance: [80 06 02 78]
        self.read_command = bytes([0x80, 0x06, 0x02, 0x78])
        
        # Command to set resolution to 0.1mm: [FA 04 0C 02 F4]
        self.config_command = bytes([0xFA, 0x04, 0x0C, 0x02, 0xF4])
        
    def connect(self):
        """Open the serial connection and configure the device."""
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=self.timeout
            )
            print(f"Connected to {self.port} at {self.baudrate} bps")
            
            # Give device time to initialize
            time.sleep(0.8)
            
            # Send configuration command to set 0.1mm resolution
            if self.configure_resolution():
                print("Resolution configured to 0.1mm")
                return True
            else:
                print("Warning: Failed to configure resolution, continuing anyway...")
                return True  # Still allow operation even if config fails
                
        except serial.SerialException as e:
            print(f"Error connecting to {self.port}: {e}")
            return False
    
    def configure_resolution(self):
        """
        Send configuration command to set resolution to 0.1mm.
        
        Returns:
            bool: True if configuration appears successful
        """
        if not self.ser or not self.ser.is_open:
            print("Serial port not connected")
            return False
        
        try:
            # Clear any existing data in buffers
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            
            # Send the configuration command
            print("Sending resolution configuration command...")
            self.ser.write(self.config_command)
            
            # Wait for potential response (some devices send acknowledgment)
            time.sleep(3)
            
            # Read any response (optional, device may or may not respond)
            response = self.ser.read(20)
            if len(response) > 0:
                print(f"Configuration response: {response.hex()}")
            
            return True
            
        except serial.SerialException as e:
            print(f"Error sending configuration command: {e}")
            return False
        except Exception as e:
            print(f"Unexpected error during configuration: {e}")
            return False
    
    def disconnect(self):
        """Close the serial connection."""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("Disconnected")
    
    def read_distance(self):
        """
        Send read command and parse the distance response.
        
        Returns:
            str: Distance reading in meters, or None if error
        """
        if not self.ser or not self.ser.is_open:
            print("Serial port not connected")
            return None
        
        try:
            # Clear any existing data in buffers
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            
            # Send the read distance command
            self.ser.write(self.read_command)
            
            # Read the response - exactly 12 bytes
            response = self.ser.read(12)
            
            if len(response) < 4:
                print("Invalid response: too short")
                return None
            
            # Check for valid header [80 06 xx]
            if response[0] != 0x80 or response[1] != 0x06:
                print(f"Invalid header: {response[:3].hex()}")
                return None
            
            # Verify checksum
            if not self.verify_checksum(response):
                print(f"Checksum failed for response: {response.hex()}")
                return None
            
            # Extract ASCII distance data (skip header, exclude last checksum byte)
            ascii_data = response[3:-1]
            
            # Convert to string
            distance_str = ascii_data.decode('ascii', errors='ignore')
            
            return distance_str
            
        except serial.SerialException as e:
            print(f"Serial communication error: {e}")
            return None
        except Exception as e:
            print(f"Error reading distance: {e}")
            return None
    
    def verify_checksum(self, data):
        """
        Verify the checksum of the received data.
        Checksum algorithm: add up all previous bytes, invert, and add 1
        
        Args:
            data (bytes): Complete response data
            
        Returns:
            bool: True if checksum is valid
        """
        if len(data) < 4:
            return False
        
        # Calculate checksum of all bytes except the last one
        checksum_calc = sum(data[:-1]) & 0xFF  # Keep only low 8 bits
        checksum_calc = ((~checksum_calc) + 1) & 0xFF  # Invert and add 1
        
        received_checksum = data[-1]
        
        return checksum_calc == received_checksum

def calculate_trend(historical_averages, current_time, lookback_seconds=20):
    """
    Calculate the trend by comparing current average with average from lookback_seconds ago.
    
    Args:
        historical_averages (deque): Deque of (timestamp, average) tuples
        current_time (float): Current timestamp
        lookback_seconds (int): How many seconds to look back
        
    Returns:
        float: Trend in meters (positive = increasing, negative = decreasing), or None if insufficient data
    """
    if len(historical_averages) < 2:
        return None
    
    target_time = current_time - lookback_seconds
    
    # Find the closest historical average to the target time
    closest_entry = None
    min_time_diff = float('inf')
    
    for timestamp, avg in historical_averages:
        time_diff = abs(timestamp - target_time)
        if time_diff < min_time_diff:
            min_time_diff = time_diff
            closest_entry = (timestamp, avg)
    
    if closest_entry is None:
        return None
    
    # Get current average (most recent entry)
    current_avg = historical_averages[-1][1]
    historical_avg = closest_entry[1]
    
    # Calculate trend (change over time period)
    trend = current_avg - historical_avg
    
    return trend

def main():
    # Configuration
    # SERIAL_PORT = input("Enter serial port (e.g., COM3, /dev/ttyUSB0): ").strip()
    SERIAL_PORT = "COM17"

    if not SERIAL_PORT:
        print("No port specified, exiting.")
        return
    
    MEASUREMENT_INTERVAL = 0  # added delay between measurements (0 => 0.75s per reading)
    WINDOW_SIZE = 60  # Number of recent readings for statistics
    TREND_LOOKBACK = 60  # seconds to look back for trend calculation
    
    # Create CSV filename with current date and time
    current_time = datetime.now()    
    outDir = r"C:\Users\beale\Documents\Rangefinder"
    logname = f"rangelog_{current_time.strftime('%Y%m%d_%H%M')}.csv"
    csv_filename = os.path.join(outDir, logname)
    
    print(f"Logging data to: {csv_filename}")
    
    # Create rangefinder instance
    rangefinder = LaserRangefinder(SERIAL_PORT)
    
    # Connect to device
    if not rangefinder.connect():
        return
    
    # Storage for rolling statistics
    recent_readings = deque(maxlen=WINDOW_SIZE)
    
    # Storage for historical averages (timestamp, average) for trend calculation
    # Keep about 30 seconds of history (assuming ~0.75s intervals, that's ~40 entries)
    historical_averages = deque(maxlen=150)
    
    try:
        # Open CSV file for writing
        with open(csv_filename, 'w', newline='', encoding='utf-8') as csvfile:
            csv_writer = csv.writer(csvfile)
            
            # Write CSV header
            csv_writer.writerow(['timestamp', 'count', 'distance', 'avg', 'std_dev_mm', 'range_mm', 'trend_20s'])
            
            print(f"Starting distance measurements every {MEASUREMENT_INTERVAL} seconds...")
            print("Press Ctrl+C to stop")
            print("Output format: timestamp,count,distance,avg,std_dev_mm,range_mm,trend_20s")
            print("-" * 70)
            
            measurement_count = 0
            consecutive_failures = 0
            MAX_CONSECUTIVE_FAILURES = 20  # Exit after 5 consecutive failures
            
            while True:
                measurement_count += 1
                
                # Read distance
                distance = rangefinder.read_distance()
                epoch_time = round(time.time(), 1)
                
                if distance:
                    # Reset failure counter on successful reading
                    consecutive_failures = 0
                    
                    try:
                        # Convert distance to float for statistics
                        distance_value = float(distance)
                        recent_readings.append(distance_value)
                        
                        # Calculate statistics if we have readings
                        if len(recent_readings) >= 1:
                            avg = statistics.mean(recent_readings)
                            
                            # Store current average with timestamp for trend calculation
                            historical_averages.append((epoch_time, avg))
                            
                            if len(recent_readings) >= 2:
                                std_dev = statistics.stdev(recent_readings)
                                range_val = max(recent_readings) - min(recent_readings)
                            else:
                                std_dev = 0.0
                                range_val = 0.0
                        else:
                            avg = 0.0
                            std_dev = 0.0
                            range_val = 0.0
                            # Still store the average for trend calculation
                            historical_averages.append((epoch_time, avg))
                        
                        # Calculate 20-second trend
                        trend = calculate_trend(historical_averages, epoch_time, TREND_LOOKBACK)
                        
                        range_mm = range_val * 1000  # Convert to mm
                        std_mm = std_dev * 1000  # Convert to mm
                        trend_mm = trend * 1000 if trend is not None else None  # Convert to mm
                        
                        # Format trend output
                        trend_str = f"{trend_mm:.3f}" if trend_mm is not None else "0"
                        
                        # Print to console
                        print(f"{epoch_time},{measurement_count},{distance_value:.4f},{avg:.6f},{std_mm:.3f},{range_mm:.1f},{trend_str}")
                        
                        # Write to CSV file
                        csv_writer.writerow([
                            epoch_time,
                            measurement_count,
                            f"{distance_value:.4f}",
                            f"{avg:.6f}",
                            f"{std_mm:.3f}",
                            f"{range_mm:.3f}",
                            trend_str
                        ])
                        csvfile.flush()  # Ensure data is written immediately
                        
                    except ValueError:
                        # If distance can't be converted to float, treat as error
                        consecutive_failures += 1
                        error_line = f"{epoch_time},{measurement_count},ERROR,,,,"
                        print(error_line)
                        # csv_writer.writerow([epoch_time, measurement_count, "ERROR", "", "", "", ""])
                        csvfile.flush()
                        time.sleep(2)  # Wait a bit longer after error
                else:
                    # Increment failure counter
                    consecutive_failures += 1
                    error_line = f"{epoch_time},{measurement_count},ERROR,,,,"
                    print(error_line)
                    # csv_writer.writerow([epoch_time, measurement_count, "ERROR", "", "", "", ""])
                    csvfile.flush()
                    time.sleep(2)  # Wait a bit longer after error

                    # Check if we should exit due to too many consecutive failures
                    if consecutive_failures >= MAX_CONSECUTIVE_FAILURES:
                        print(f"\nExiting: {MAX_CONSECUTIVE_FAILURES} consecutive communication failures detected.")
                        print("This usually indicates the device has been disconnected.")
                        break
                
                # Wait before next measurement
                time.sleep(MEASUREMENT_INTERVAL)
            
    except KeyboardInterrupt:
        print("\nStopping measurements...")
        print(f"Data saved to: {csv_filename}")
    except Exception as e:
        print(f"Unexpected error: {e}")
        print(f"Data saved to: {csv_filename}")
    finally:
        rangefinder.disconnect()
        print(f"Final log file: {os.path.abspath(csv_filename)}")

if __name__ == "__main__":
    print("Laser Rangefinder Distance Reader with 20-Second Trend Analysis")
    print("=" * 60)
    main()
    
