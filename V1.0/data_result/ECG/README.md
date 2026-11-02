## Signal Processing and Visualization

### Software

The acquired ECG data is imported into **Origin** for waveform visualization and preliminary signal processing.

### Processing Workflow

1. Import the raw ECG data collected by the ADS1299 and STM32 acquisition system into Origin.
2. Plot the original ECG waveform to observe the acquired signal.
3. Apply the built-in FFT filtering function in Origin to reduce unwanted frequency components.
4. Export the filtered waveform for visualization and comparison.

### Experimental Results

The following figures show the ECG signal before and after FFT-based filtering.

- **Raw ECG:** Original signal acquired by the hardware.
- **Filtered ECG:** Signal after preliminary FFT-based filtering in Origin.

### Notes and Limitations

- Sampling rate: 1000 Hz.
- Signal processing is currently performed offline using Origin.
- The FFT filtering is performed using Origin's built-in tools; a custom digital filtering algorithm has not yet been implemented.
- The acquired signal still exhibits baseline drift and environmental noise. Further improvements to the hardware and signal processing methods are planned.