import matplotlib.pyplot as plt

# Threads remain consistent across all algorithms
threads = [1, 2, 4, 8]

# Inner Speedup Results (Depth 3)
depth3_ybwc = [1.00, 2.51, 3.72, 1.39]
depth3_naive_ab = [1.00, 1.65, 2.68, 3.70]
depth3_naive_ybwc = [1.00, 1.80, 2.88, 4.01]
depth3_minimax = [1.00, 1.86, 3.21, 4.80]
depth3_pvs = [1.00, 1.86, 1.33, 0.24]

# Inner Speedup Results (Depth 4)
depth4_ybwc = [1.00, 2.57, 3.70, 1.22]
depth4_naive_ab = [1.00, 1.52, 2.42, 3.44]
depth4_naive_ybwc = [1.00, 1.80, 2.84, 4.01]
depth4_minimax = [1.00, 1.86, 3.15, 4.47]
depth4_pvs = [1.00, 1.44, 0.73, 0.13]

# Relative to Sequential Speedup Results (Depth 3)
rel_depth3_ybwc = [0.97, 2.43, 3.60, 1.35]
rel_depth3_naive_ab = [1.00, 1.65, 2.68, 3.71]
rel_depth3_naive_ybwc = [1.00, 1.80, 2.88, 4.02]
rel_depth3_minimax = [1.23, 2.29, 3.95, 5.89]
rel_depth3_pvs = [0.85, 1.59, 1.14, 0.21]

# Relative to Sequential Speedup Results (Depth 4)
rel_depth4_ybwc = [0.96, 2.45, 3.49, 1.16]
rel_depth4_naive_ab = [1.00, 1.52, 2.42, 3.45]
rel_depth4_naive_ybwc = [1.00, 1.79, 2.84, 4.00]
rel_depth4_minimax = [1.15, 2.14, 3.63, 5.16]
rel_depth4_pvs = [0.80, 1.16, 0.62, 0.11]

# Plot Nested Inner Speedup Results (Depth 3)
plt.figure(figsize=(10, 6))
plt.plot(threads, depth3_ybwc, marker="o", label="YBWC")
plt.plot(threads, depth3_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(
    threads, depth3_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth3_minimax, marker="o", label="Parallel Minimax")
plt.plot(threads, depth3_pvs, marker="o", label="PVS")
plt.title("Nested Inner Speedup Results (Depth 3)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xticks(threads)
plt.legend()
plt.grid(True)
plt.show()

# Plot Nested Inner Speedup Results (Depth 4)
plt.figure(figsize=(10, 6))
plt.plot(threads, depth4_ybwc, marker="o", label="YBWC")
plt.plot(threads, depth4_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(
    threads, depth4_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth4_minimax, marker="o", label="Parallel Minimax")
plt.plot(threads, depth4_pvs, marker="o", label="PVS")
plt.title("Nested Inner Speedup Results (Depth 4)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xticks(threads)
plt.legend()
plt.grid(True)
plt.show()

# Plot Nested Relative to Sequential Speedup Results (Depth 3)
plt.figure(figsize=(10, 6))
plt.plot(threads, rel_depth3_ybwc, marker="o", label="YBWC")
plt.plot(threads, rel_depth3_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(
    threads,
    rel_depth3_naive_ybwc,
    marker="o",
    label="Naive Parallel Alpha Beta with PV",
)
plt.plot(threads, rel_depth3_minimax, marker="o", label="Parallel Minimax")
plt.plot(threads, rel_depth3_pvs, marker="o", label="PVS")
plt.title("Nested Relative to Sequential Speedup Results (Depth 3)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xticks(threads)
plt.legend()
plt.grid(True)
plt.show()

# Plot Nested Relative to Sequential Speedup Results (Depth 4)
plt.figure(figsize=(10, 6))
plt.plot(threads, rel_depth4_ybwc, marker="o", label="YBWC")
plt.plot(threads, rel_depth4_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(
    threads,
    rel_depth4_naive_ybwc,
    marker="o",
    label="Naive Parallel Alpha Beta with PV",
)
plt.plot(threads, rel_depth4_minimax, marker="o", label="Parallel Minimax")
plt.plot(threads, rel_depth4_pvs, marker="o", label="PVS")
plt.title("Nested Relative to Sequential Speedup Results (Depth 4)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xticks(threads)
plt.legend()
plt.grid(True)
plt.show()
