import matplotlib.pyplot as plt
import numpy as np

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

plt.ylim(bottom=1e-5, top=1.1)

plt.gca().invert_xaxis()
plt.show()
