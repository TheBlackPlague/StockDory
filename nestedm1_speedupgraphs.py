import matplotlib.pyplot as plt

# Threads remain consistent across all algorithms
threads = [1, 2, 4, 8]

# Inner Speedup Results (Depth 1)
depth1_minimax = [1.00, 1.88, 2.94, 3.49]
depth1_naive_ybwc = [1.00, 1.66, 2.54, 3.04]
depth1_ybwc = [1.00, 1.60, 2.44, 2.98]
depth1_pvs = [1.00, 1.60, 2.46, 3.12]
depth1_naive_ab = [1.00, 1.67, 2.65, 3.26]

# Inner Speedup Results (Depth 2)
depth2_minimax = [1.00, 1.84, 3.05, 4.48]
depth2_naive_ybwc = [1.00, 1.68, 2.62, 3.37]
depth2_ybwc = [1.00, 1.64, 2.53, 2.95]
depth2_pvs = [1.00, 1.33, 1.59, 1.65]
depth2_naive_ab = [1.00, 1.64, 2.61, 3.40]

# Relative to Sequential Speedup Results (Depth 1)
rel_depth1_minimax = [0.87, 1.52, 2.47, 3.08]
rel_depth1_naive_ybwc = [0.98, 1.62, 2.50, 2.99]
rel_depth1_ybwc = [1.01, 1.62, 2.46, 3.02]
rel_depth1_pvs = [1.01, 1.62, 2.48, 3.15]
rel_depth1_naive_ab = [0.97, 1.62, 2.58, 3.17]

# Relative to Sequential Speedup Results (Depth 2)
rel_depth2_minimax = [0.94, 1.69, 2.86, 4.32]
rel_depth2_naive_ybwc = [0.98, 1.64, 2.58, 3.32]
rel_depth2_ybwc = [0.99, 1.63, 2.51, 2.94]
rel_depth2_pvs = [0.97, 1.30, 1.57, 1.66]
rel_depth2_naive_ab = [0.98, 1.60, 2.54, 3.32]

# Plot Inner Speedup Results (Depth 1)
plt.figure(figsize=(10, 6))
plt.plot(threads, depth1_minimax, marker="o", label="Parallel Minimax")
plt.plot(
    threads, depth1_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth1_ybwc, marker="o", label="YBWC")
plt.plot(threads, depth1_pvs, marker="o", label="PVS")
plt.plot(threads, depth1_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.title("Nested Inner Speedup Results (Depth 1)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()

# Plot Inner Speedup Results (Depth 2)
plt.figure(figsize=(10, 6))
plt.plot(threads, depth2_minimax, marker="o", label="Parallel Minimax")
plt.plot(
    threads, depth2_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth2_ybwc, marker="o", label="YBWC")
plt.plot(threads, depth2_pvs, marker="o", label="PVS")
plt.plot(threads, depth2_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.title("Nested Inner Speedup Results (Depth 2)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()

# Plot Relative to Sequential Speedup Results (Depth 1)
plt.figure(figsize=(10, 6))
plt.plot(threads, rel_depth1_minimax, marker="o", label="Parallel Minimax")
plt.plot(
    threads,
    rel_depth1_naive_ybwc,
    marker="o",
    label="Naive Parallel Alpha Beta with PV",
)
plt.plot(threads, rel_depth1_ybwc, marker="o", label="YBWC")
plt.plot(threads, rel_depth1_pvs, marker="o", label="PVS")
plt.plot(threads, rel_depth1_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.title("Nested Relative to Sequential Speedup Results (Depth 1)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()

# Plot Relative to Sequential Speedup Results (Depth 2)
plt.figure(figsize=(10, 6))
plt.plot(threads, rel_depth2_minimax, marker="o", label="Parallel Minimax")
plt.plot(
    threads,
    rel_depth2_naive_ybwc,
    marker="o",
    label="Naive Parallel Alpha Beta with PV",
)
plt.plot(threads, rel_depth2_ybwc, marker="o", label="YBWC")
plt.plot(threads, rel_depth2_pvs, marker="o", label="PVS")
plt.plot(threads, rel_depth2_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.title("Nested Relative to Sequential Speedup Results (Depth 2)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()
