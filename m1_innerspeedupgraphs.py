import matplotlib.pyplot as plt

# Data extracted from the text file
threads = [1, 2, 4, 8, 16, 32, 64]

# Speedup data for different algorithms at Depth 1
depth1_ybwc = [1.00, 1.60, 2.46, 3.00, 2.81, 2.18, 1.70]
depth1_naive_ybwc = [1.00, 1.81, 2.71, 3.11, 3.09, 2.46, 1.88]
depth1_naive_ab = [1.00, 1.67, 2.68, 3.66, 3.42, 2.63, 1.74]
depth1_minimax = [1.00, 1.79, 2.83, 3.39, 3.45, 2.48, 1.97]
depth1_pvs = [1.00, 1.58, 2.41, 2.83, 2.81, 2.11, 1.77]

# Speedup data for different algorithms at Depth 2
depth2_ybwc = [1.00, 1.64, 2.51, 3.14, 3.28, 2.62, 2.01]
depth2_naive_ybwc = [1.00, 1.77, 2.72, 3.26, 3.40, 2.84, 2.12]
depth2_naive_ab = [1.00, 1.60, 2.58, 3.17, 3.43, 2.90, 2.24]
depth2_minimax = [1.00, 1.81, 3.05, 4.17, 5.00, 4.81, 3.76]
depth2_pvs = [1.00, 1.61, 2.48, 2.93, 2.99, 2.38, 2.03]

# Plot Depth 1 results
plt.figure(figsize=(10, 6))
plt.plot(threads, depth1_ybwc, marker='o', label="YBWC")
plt.plot(
    threads, depth1_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth1_naive_ab, marker='o', label="Naive Parallel Alpha Beta")
plt.plot(threads, depth1_minimax, marker='o', label="Parallel Minimax")
plt.plot(threads, depth1_pvs, marker='o', label="PVS")
plt.title("Inner Speedup Results for Mate in 1 (Depth 1)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xscale("log", base=2)
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()

# Plot Depth 2 results
plt.figure(figsize=(10, 6))
plt.plot(threads, depth2_ybwc, marker='o', label="YBWC")
plt.plot(
    threads, depth2_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth2_naive_ab, marker='o', label="Naive Parallel Alpha Beta")
plt.plot(threads, depth2_minimax, marker='o', label="Parallel Minimax")
plt.plot(threads, depth2_pvs, marker='o', label="PVS")
plt.title("Inner Speedup Results for Mate in 1 (Depth 2)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xscale("log", base=2)
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()
