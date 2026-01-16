import subprocess
import time

# Параметры
PORT = "12345"
FILES = ["test.txt", "video.mp4", "arhiv.zip", "pasport.pdf", "seksi.jpg"]

procs = []

print(f"Запускаю {len(FILES)} клиентов параллельно...")

# Засекаем время начала
start_time = time.time()

# 1. Быстрый запуск всех процессов
for filename in FILES:
    p = subprocess.Popen(
        ["./client", "localhost", PORT, filename],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    procs.append(p)
    print(f"  → Клиент для '{filename}' запущен (PID: {p.pid})")

print("\nОжидание завершения всех клиентов...")

# 2. Ожидание завершения всех
for p in procs:
    p.wait()

# Время окончания
end_time = time.time()
elapsed = end_time - start_time

print(f"\n✅ Все клиенты завершили работу за {elapsed:.2f} секунд")
print(f"📊 Среднее время на клиента: {elapsed / len(FILES):.2f} секунд")
