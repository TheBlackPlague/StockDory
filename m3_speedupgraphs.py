import matplotlib.pyplot as plt

# Threads remain consistent across algorithms
threads = [1, 2, 4, 8, 16, 32, 64]

# Inner Speedup Results (Depth 5)
depth5_ybwc = [1.00, 1.98, 3.39, 5.61, 6.98, 5.79, 4.74]
depth5_naive_ybwc = [1.00, 2.01, 3.46, 5.77, 7.72, 9.44, 8.05]
depth5_minimax = [1.00, 1.94, 3.73, 6.45, 10.67, 13.69, 12.55]
depth5_naive_ab = [1.00, 1.95, 3.37, 5.24, 7.45, 9.34, 8.15]
depth5_pvs = [1.00, 1.98, 3.41, 5.64, 6.34, 5.47, 4.62]

# Inner Speedup Results (Depth 6)
depth6_ybwc = [1.00, 1.99, 3.41, 5.59, 7.22, 6.84, 5.68]
depth6_naive_ybwc = [1.00, 2.01, 3.46, 5.70, 7.63, 9.23, 7.94]
depth6_minimax = [1.00, 1.95, 3.74, 6.40, 10.39, 13.35, 12.62]
depth6_naive_ab = [1.00, 1.92, 3.36, 5.12, 7.17, 9.01, 7.89]
depth6_pvs = [1.00, 1.99, 3.42, 5.56, 6.11, 5.39, 4.48]

# Relative to Sequential Speedup Results (Depth 5)
rel_depth5_ybwc = [0.97, 1.93, 3.31, 5.47, 6.81, 5.65, 4.63]
rel_depth5_naive_ybwc = [1.00, 2.00, 3.46, 5.77, 7.72, 9.45, 8.05]
rel_depth5_minimax = [1.01, 1.95, 3.76, 6.49, 10.73, 13.78, 12.62]
rel_depth5_naive_ab = [1.00, 1.95, 3.38, 5.26, 7.47, 9.37, 8.18]
rel_depth5_pvs = [0.96, 1.91, 3.28, 5.43, 6.11, 5.27, 4.45]

# Relative to Sequential Speedup Results (Depth 6)
rel_depth6_ybwc = [0.98, 1.94, 3.33, 5.46, 7.05, 6.68, 5.55]
rel_depth6_naive_ybwc = [1.00, 2.01, 3.46, 5.70, 7.63, 9.23, 7.94]
rel_depth6_minimax = [1.00, 1.95, 3.75, 6.42, 10.42, 13.39, 12.67]
rel_depth6_naive_ab = [1.00, 1.92, 3.36, 5.13, 7.19, 9.03, 7.91]
rel_depth6_pvs = [0.94, 1.87, 3.22, 5.23, 5.75, 5.07, 4.21]

# Plot Inner Speedup Results (Depth 5)
plt.figure(figsize=(10, 6))
plt.plot(threads, depth5_ybwc, marker="o", label="YBWC")
plt.plot(
    threads, depth5_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth5_minimax, marker="o", label="Parallel Minimax")
plt.plot(threads, depth5_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(threads, depth5_pvs, marker="o", label="PVS")
plt.title("Inner Speedup Results (Depth 5)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xscale("log", base=2)
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()

# Plot Inner Speedup Results (Depth 6)
plt.figure(figsize=(10, 6))
plt.plot(threads, depth6_ybwc, marker="o", label="YBWC")
plt.plot(
    threads, depth6_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth6_minimax, marker="o", label="Parallel Minimax")
plt.plot(threads, depth6_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(threads, depth6_pvs, marker="o", label="PVS")
plt.title("Inner Speedup Results (Depth 6)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xscale("log", base=2)
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()

# Plot Relative to Sequential Speedup Results (Depth 5)
plt.figure(figsize=(10, 6))
plt.plot(threads, rel_depth5_ybwc, marker="o", label="YBWC")
plt.plot(
    threads,
    rel_depth5_naive_ybwc,
    marker="o",
    label="Naive Parallel Alpha Beta with PV",
)
plt.plot(threads, rel_depth5_minimax, marker="o", label="Parallel Minimax")
plt.plot(threads, rel_depth5_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(threads, rel_depth5_pvs, marker="o", label="PVS")
plt.title("Relative to Sequential Speedup Results (Depth 5)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xscale("log", base=2)
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()

# Plot Relative to Sequential Speedup Results (Depth 6)
plt.figure(figsize=(10, 6))
plt.plot(threads, rel_depth6_ybwc, marker="o", label="YBWC")
plt.plot(
    threads,
    rel_depth6_naive_ybwc,
    marker="o",
    label="Naive Parallel Alpha Beta with PV",
)
plt.plot(threads, rel_depth6_minimax, marker="o", label="Parallel Minimax")
plt.plot(threads, rel_depth6_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(threads, rel_depth6_pvs, marker="o", label="PVS")
plt.title("Relative to Sequential Speedup Results (Depth 6)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xscale("log", base=2)
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()
