import numpy as np
import pd


def generate_sine_wave(frequency, amplitude, phase, num_samples, sampling_rate):
    angular_frequency = 2 * np.pi * frequency
    t = np.arange(num_samples) / sampling_rate
    sine_wave = amplitude * np.sin(angular_frequency * t + phase)
    last_phase = phase + angular_frequency * t[-1]
    return sine_wave, last_phase


def mksenoide(freqs, amps, phases, vectorsize, samplerate):
    n = len(freqs)
    nframes = vectorsize
    out = np.zeros((n, nframes))  # Array with 2 dimensions: channels by frames
    new_phases = np.zeros(n)  # Array to store the new phases
    for i in range(n):
        out[i], new_phases[i] = generate_sine_wave(
            freqs[i], amps[i], phases[i], nframes, samplerate
        )
    return out, new_phases


def sinusoids(freqs, amps):
    if freqs is None or amps is None:
        return None
    if len(freqs) != len(amps):
        return None

    phases = pd.get_obj_var("PHASE", initial_value=np.zeros(len(freqs)))
    freqs = np.array(freqs, dtype=np.float64)
    amps = np.array(amps, dtype=np.float64)
    out, new_phases = mksenoide(freqs, amps, phases, 64, pd.get_sample_rate())
    pd.set_obj_var("PHASE", new_phases)
    return out


# ============================================
# ==================== PD ====================
# ============================================


def py4pdLoadObjects():
    pd.add_object(sinusoids, "sinusoids~", objtype=pd.AUDIOOUT)
