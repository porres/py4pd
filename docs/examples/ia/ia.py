import torch 
import librosa
import soundfile
import pd



def renderAudio_nn(audio, model):
    model = pd.home() + '/' + model
    audio = pd.home() + '/' +  audio
    torch.set_grad_enabled(False)
    model = torch.jit.load(model).eval()
    x = librosa.load(audio)[0]
    max_x = max(x)
    min_x = min(x)
    x_for = torch.from_numpy(x).reshape(1, 1, -1)
    z = model.encode(x_for)
    z[:, 0] += torch.linspace(-2, 2, z.shape[-1])
    y = model.decode(z).numpy().reshape(-1)
    pd.tabwrite('iaAudio', y.tolist(), resize=True)
    pd.print('Audio rendered')





