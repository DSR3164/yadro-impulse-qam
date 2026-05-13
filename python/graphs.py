import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({
    'axes.facecolor': '#1e1e1e',
    'figure.facecolor': '#1e1e1e',
    'text.color': 'white',
    'axes.labelcolor': 'white',
    'xtick.color': 'white',
    'ytick.color': 'white',
    'grid.color': "#747474",
    'font.size': 9,
    'savefig.facecolor': '#1e1e1e',
})

def apply_dark_theme(fig):
    fig.canvas.get_tk_widget().configure(bg='#1e1e1e')
    fig.canvas.manager.window.configure(bg='#1e1e1e')

_original_figure = plt.figure

def figure(*args, **kwargs):
    fig = _original_figure(*args, **kwargs)
    apply_dark_theme(fig)
    return fig

plt.figure = figure

mods = ["qpsk.csv", "qam16.csv", "qam64.csv"]

plt.figure(figsize=(10, 6))

for x in mods:
    try:
        data = np.loadtxt("files/" + x, delimiter=",")
        stddev = data[:, 0]
        ber = data[:, 1]
        plt.plot(stddev, ber, "o-", label=x.replace(".csv", "").upper())
    except Exception as e:
        print(f"Ошибка при обработке файла {x}: {e}")

plt.yscale("log")
plt.grid(True, which="both", linestyle="--", alpha=0.5)

plt.xlabel("Шум (StdDev / Sigma)")
plt.ylabel("Bit Error Rate (BER)")
plt.title("Зависимость BER от уровня шума для различных модуляций")
plt.legend()
plt.tight_layout()

plt.ylim(bottom=1e-5, top=1.1)

plt.gca().invert_xaxis()
plt.show()
