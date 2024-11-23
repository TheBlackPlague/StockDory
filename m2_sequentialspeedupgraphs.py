import matplotlib.pyplot as plt

# Threads remain consistent across algorithms
threads = [1, 2, 4, 8, 16, 32, 64]

# Speedup data for Depth 3
depth3_naive_ab = [1.03, 1.72, 2.88, 4.15, 5.86, 6.55, 3.78]
depth3_naive_ybwc = [1.00, 1.78, 2.91, 4.10, 5.11, 5.50, 4.55]
depth3_ybwc = [0.98, 1.75, 2.84, 3.92, 4.51, 4.26, 3.11]
depth3_pvs = [0.97, 1.67, 2.83, 3.77, 4.40, 4.13, 3.27]
depth3_minimax = [1.03, 1.92, 3.40, 5.12, 6.04, 7.17, 5.98]

# Speedup data for Depth 4
depth4_naive_ab = [1.05, 1.61, 2.63, 3.86, 5.16, 5.92, 4.30]
depth4_naive_ybwc = [1.00, 1.82, 2.95, 4.24, 5.24, 5.64, 4.05]
depth4_ybwc = [0.98, 1.78, 2.84, 4.07, 4.83, 4.76, 3.43]
depth4_pvs = [0.93, 1.66, 2.78, 3.83, 4.16, 4.01, 3.05]
depth4_minimax = [1.04, 1.93, 3.30, 4.74, 5.62, 6.72, 5.72]

# Plot Depth 3 results
plt.figure(figsize=(10, 6))
plt.plot(threads, depth3_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(
    threads, depth3_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth3_ybwc, marker="o", label="YBWC")
plt.plot(threads, depth3_pvs, marker="o", label="PVS")
plt.plot(threads, depth3_minimax, marker="o", label="Parallel Minimax")
plt.title("Relative to Sequential Speedup Results (Depth 3)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xscale("log", base=2)
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()

# Plot Depth 4 results
plt.figure(figsize=(10, 6))
plt.plot(threads, depth4_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(
    threads, depth4_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth4_ybwc, marker="o", label="YBWC")
plt.plot(threads, depth4_pvs, marker="o", label="PVS")
plt.plot(threads, depth4_minimax, marker="o", label="Parallel Minimax")
plt.title("Relative to Sequential Speedup Results (Depth 4)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xscale("log", base=2)
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()
