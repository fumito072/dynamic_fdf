import numpy as np
from scipy.io.wavfile import write
from scipy import signal

def generate_pink_noise(size):
    # ピンクノイズ生成
    white = np.random.normal(0, 1, size)
    # 低周波を強調するフィルター
    b = signal.firwin(101, cutoff=0.1)
    pink = signal.lfilter(b, 1, white)
    return pink

# パラメータ
samplerate = 44100  # サンプリングレート
duration = 10       # 秒数
volume = 0.3        # 音量（0.0〜1.0）

# 左右チャンネル用のノイズ生成
samples_left = generate_pink_noise(int(samplerate * duration))
samples_right = generate_pink_noise(int(samplerate * duration))

# 音量の変動を追加（風の強さの変化を表現）
t = np.linspace(0, duration, len(samples_left))
wind_variation = 0.3 * np.sin(2 * np.pi * 0.1 * t) + 0.7  # ゆっくりとした変動
samples_left *= wind_variation
samples_right *= wind_variation

# フェードイン・フェードアウト
fade = np.linspace(0, 1, int(samplerate * 2))  # 2秒フェード
samples_left[:fade.size] *= fade
samples_left[-fade.size:] *= fade[::-1]
samples_right[:fade.size] *= fade
samples_right[-fade.size:] *= fade[::-1]

# ステレオ化（左右で少し異なる音にする）
samples = np.vstack((samples_left, samples_right)).T

# 音量調整 & 16bit整数化
samples = (samples * (32767 * volume)).astype(np.int16)

# WAVファイルとして保存
write("assets/noise.wav", samplerate, samples)

print("assets/noise.wav に風の音を保存しました。")