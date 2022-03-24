import matplotlib.pyplot as plt
import numpy as np
import serial
import time

Fs = 2000.0  # sampling rate
Ts = 1.0/Fs  # sampling interval

serdev = '/COM7'
s = serial.Serial(serdev)
print("OK")
while (1):
    line = s.readline()
    print(line)
    if (line == b'Start transmission.\r\n'):
        break

print("Get original signal")
numOfSamples = int(s.readline())
durationOfSamples = float(s.readline()) / 1000000
y = np.linspace(0, 1, num = numOfSamples)  # signal vector; create numOfSamples samples
print("numOfSamples: ", numOfSamples)
print("durationOfSamples: ",durationOfSamples)
for x in range(0, numOfSamples):
    line = s.readline()  # Read an echo string from B_L4S5I_IOT01A terminated with '\n'
    y[x] = float(line)
    if (x % 100 == 0):
        print("\r", x,"\r", end="")

# time vector; create Fs samples between 0 and 1.0 sec.
t = np.linspace(0, durationOfSamples, num = numOfSamples)
n = len(t)  # length of the signal
k = np.arange(n)
#frq = range(2000)  # focus on frequency of interest

print("FFT")
Y = np.fft.fft(y) / n  # fft computing and normalization
Y = Y[range(int(n/2))]  # focus on frequency of interest

freq = np.fft.fftfreq(n, d=durationOfSamples/numOfSamples)
freq = freq[range(int(n/2))]

#############################################
fig, ax = plt.subplots(2, 1)
ax[0].plot(t, y)
ax[0].set_xlabel('Time')
ax[0].set_ylabel('Amplitude')
ax[1].plot(freq, abs(Y), 'r')  # plotting the spectrum
ax[1].set_xlabel('Freq (Hz)')
ax[1].set_ylabel('|Y(freq)|')
plt.suptitle("Frequency of interest")
plt.subplots_adjust(bottom=0.1, top=0.9, wspace=None, hspace=0.25)
# # inset axes....
# axins = ax[1].inset_axes([0.5, 0.2, 0.47, 0.77])
# axins.plot(frq, abs(Y), 'r')
# # sub region of the original image
# x1, x2, y1, y2 = -5, 30, -0.1, 1.1
# axins.set_xlim(x1, x2)
# axins.set_ylim(y1, y2)
# axins.set_xticklabels([])
# axins.set_yticklabels([])
# ax[1].indicate_inset_zoom(axins, edgecolor="black")

plt.show()
s.close()
