"""
Audio Data Processor & Visualizer forr Modulino Microphone Recordings
---------------------------------
This script processes raw 16-bit PCM audio samples dumped from an STM32/Arduino 
serial monitor text file. It parses the numeric data, generates a waveform plot 
using Matplotlib, and exports the audio into a standard CD-quality-compatible 
WAV file for playback.

Requirements:
    - Python 3.x
    - NumPy
    - Matplotlib
    - SoundFile

Installation:
    pip install numpy matplotlib soundfile

Usage:
    python listen_audio.py <filename.txt>
    Example: python listen_audio.py audio_data.txt
"""

import sys
import os
import numpy as np
import matplotlib.pyplot as plt
import soundfile as sf

# 1. Audio parameter configuration
# 64 samples every 4096 microseconds -> Sample Rate = 16000 Hz (Standard for Modulino Microphone)
SAMPLE_RATE = 16000.0 

# Check if a filename parameter was passed from command line
if len(sys.argv) > 1:
    input_filename = sys.argv[1]
else:
    # Fallback to default if no argument is provided
    input_filename = "audio_data.txt"
    print(f"No input file specified. Using default: '{input_filename}'")

# Check if the file actually exists
if not os.path.exists(input_filename):
    print(f"Error: The file '{input_filename}' does not exist.")
    print("Usage: python listen_audio.py <filename.txt>")
    sys.exit(1)

# 2. Load data from the text file
print(f"Loading data from '{input_filename}'...")
with open(input_filename, "r") as f:
    # Read lines, strip whitespace, and filter out non-numeric/empty lines
    samples = [int(line.strip()) for line in f if line.strip().replace('-', '').isdigit()]

# Convert to a 16-bit signed integer NumPy array
audio_array = np.array(samples, dtype=np.int16)

if len(audio_array) == 0:
    print("Error: No valid audio samples found in the file.")
    sys.exit(1)

print(f"Total samples loaded: {len(audio_array)}")
print(f"Estimated duration: {len(audio_array) / SAMPLE_RATE:.2f} seconds")

# 3. GRAPH / PLOT GENERATION
plt.figure(figsize=(10, 4))
plt.plot(audio_array, color='blue', linewidth=0.5)
plt.title(f"Recorded Audio Waveform ({input_filename})")
plt.xlabel("Samples")
plt.ylabel("Amplitude (int16)")
plt.grid(True)

# 4. SAVE TO REALISTIC .WAV AUDIO FILE
# For optimal playback on PC, normalize the signal to float32
audio_float = audio_array.astype(np.float32) / 32768.0
output_filename = 'recorded_audio.wav'
sf.write(output_filename, audio_float, int(SAMPLE_RATE))
print(f"Audio file '{output_filename}' successfully created!")

# Display the plot window
plt.show()