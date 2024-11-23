import matplotlib.pyplot as plt

# Threads remain consistent across algorithms
threads = [1, 2, 4, 8, 16, 32, 64]

# Speedup data for Depth 1
depth1_ybwc = [1.08, 1.72, 2.58, 3.13, 2.95, 2.25, 1.76]
depth1_naive_ybwc = [0.95, 1.71, 2.57, 2.96, 2.94, 2.35, 1.80]
depth1_naive_ab = [0.96, 1.61, 2.58, 3.53, 3.30, 2.54, 1.69]
depth1_minimax = [0.92, 1.56, 2.47, 3.08, 3.20, 2.36, 1.92]
depth1_pvs = [1.08, 1.69, 2.55, 2.97, 2.95, 2.16, 1.83]

# Speedup data for Depth 2
depth2_ybwc = [1.03, 1.68, 2.53, 3.16, 3.29, 2.61, 2.00]
depth2_naive_ybwc = [0.95, 1.68, 2.58, 3.10, 3.25, 2.71, 2.03]
depth2_naive_ab = [0.97, 1.55, 2.50, 3.07, 3.33, 2.83, 2.19]
depth2_minimax = [0.96, 1.70, 2.87, 4.02, 4.88, 4.75, 3.74]
depth2_pvs = [0.99, 1.58, 2.41, 2.84, 2.89, 2.27, 1.95]

# Plot Depth 1 results
plt.figure(figsize=(10, 6))
plt.plot(threads, depth1_ybwc, marker="o", label="YBWC")
plt.plot(
    threads, depth1_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth1_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(threads, depth1_minimax, marker="o", label="Parallel Minimax")
plt.plot(threads, depth1_pvs, marker="o", label="PVS")
plt.title("Relative to Sequential Speedup Results (Depth 1)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xscale("log", base=2)
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()

# Plot Depth 2 results
plt.figure(figsize=(10, 6))
plt.plot(threads, depth2_ybwc, marker="o", label="YBWC")
plt.plot(
    threads, depth2_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth2_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(threads, depth2_minimax, marker="o", label="Parallel Minimax")
plt.plot(threads, depth2_pvs, marker="o", label="PVS")
plt.title("Relative to Sequential Speedup Results (Depth 2)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xscale("log", base=2)
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()
