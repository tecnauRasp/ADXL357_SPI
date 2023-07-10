import pandas as pd
import matplotlib.pyplot as plt
from plotly.subplots import make_subplots
import plotly.graph_objects as go
import numpy as np
import scipy.fftpack
import os

dir_path = "CsvRecords/"

csvFilename = input('CSV file name: ')

csv_path = os.path.join(dir_path, csvFilename + ".csv")

params = ['Time', 'X', 'Y','Z', 'temp']
df = pd.read_csv(csv_path, names=params, header=1, sep=';', index_col=False, decimal=".")

# for p in params:
#     df[p] = pd.to_numeric(df[p], errors='corece')



df = df.dropna()


# PLOTLY
fig_1 = make_subplots(rows=3, cols=1, subplot_titles=("X [g]", "Y [g]", "Z [g]"))
fig_1.append_trace(go.Scatter(x=df['Time'], y=df['X'], mode='lines'), row=1, col=1)
fig_1.append_trace(go.Scatter(x=df['Time'], y=df['Y'], mode='lines'), row=2, col=1)
fig_1.append_trace(go.Scatter(x=df['Time'], y=df['Z'], mode='lines'), row=3, col=1)

fig_1.update_layout(height=768, width=1024, title_text="")
fig_1.show()



# FFT
N = len(df)     # number of samplepoints
Fs = 4000;      # frequency
freq = np.linspace(0.0, Fs/2, num=N//2)
xdft = scipy.fftpack.fft(df['X'].values)
ydft = scipy.fftpack.fft(df['Y'].values)
zdft = scipy.fftpack.fft(df['Z'].values)

fig_2 = make_subplots(rows=3, cols=1, subplot_titles=("Fft(X)", "Fft(Y)", "Fft(Z)"))
fig_2.append_trace(go.Scatter(x=freq, y=2.0/N * np.abs(xdft[:N//2]), mode='lines'), row=1, col=1)
fig_2.append_trace(go.Scatter(x=freq, y=2.0/N * np.abs(ydft[:N//2]), mode='lines'), row=2, col=1)
fig_2.append_trace(go.Scatter(x=freq, y=2.0/N * np.abs(zdft[:N//2]), mode='lines'), row=3, col=1)

fig_2.update_layout(height=768, width=1024, title_text="")
fig_2.show()
