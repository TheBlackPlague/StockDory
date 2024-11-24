import matplotlib.pyplot as plt

# Threads are consistent across algorithms
threads = [1, 2, 4, 8, 16, 32, 64]

# Speedup data for different algorithms at Depth 3
depth3_naive_ab = [1.00, 1.66, 2.77, 4.00, 5.64, 6.28, 3.63]
depth3_naive_ybwc = [1.00, 1.78, 2.91, 4.10, 5.10, 5.51, 4.55]
depth3_ybwc = [1.00, 1.79, 2.89, 4.00, 4.60, 4.34, 3.17]
depth3_pvs = [1.00, 1.73, 2.93, 3.90, 4.55, 4.26, 3.37]
depth3_minimax = [1.00, 1.88, 3.32, 5.02, 5.92, 7.05, 5.89]

# Speedup data for different algorithms at Depth 4
depth4_naive_ab = [1.00, 1.53, 2.50, 3.66, 4.91, 5.60, 4.04]
depth4_naive_ybwc = [1.00, 1.81, 2.94, 4.22, 5.23, 5.62, 4.04]
depth4_ybwc = [1.00, 1.81, 2.89, 4.13, 4.89, 4.83, 3.48]
depth4_pvs = [1.00, 1.79, 3.01, 4.19, 4.54, 4.39, 3.34]
depth4_minimax = [1.00, 1.87, 3.19, 4.59, 5.45, 6.56, 5.59]

# Plot Depth 3 results
plt.figure(figsize=(10, 6))
plt.plot(threads, depth3_naive_ab, marker="o", label="Naive Parallel Alpha Beta")
plt.plot(
    threads, depth3_naive_ybwc, marker="o", label="Naive Parallel Alpha Beta with PV"
)
plt.plot(threads, depth3_ybwc, marker="o", label="YBWC")
plt.plot(threads, depth3_pvs, marker="o", label="PVS")
plt.plot(threads, depth3_minimax, marker="o", label="Parallel Minimax")
plt.title("Inner Speedup Results for Mate in 2 (Depth 3)")
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
plt.title("Inner Speedup Results for Mate in 2 (Depth 4)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.xscale("log", base=2)
plt.xticks(threads, threads)
plt.legend()
plt.grid(True)
plt.show()
