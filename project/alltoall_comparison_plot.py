import numpy as np
import matplotlib.pyplot as plt

# Data
stages = np.array([100, 1000, 10000])

# RDMA data
rdma_data = {
    100: [1.332, 0.625, 0.933],
    1000: [4.923, 8.165, 6.903],
    10000: [68.447, 65.470, 67.065]
}

# MPI data
mpi_data = {
    100: [1.718, 1.734, 1.749],
    1000: [4.468, 3.923, 4.131],
    10000: [27.979, 27.669, 31.357]
}

# Calculate means and standard deviations
rdma_means = np.array([np.mean(rdma_data[s]) for s in stages])
rdma_std = np.array([np.std(rdma_data[s]) for s in stages])
mpi_means = np.array([np.mean(mpi_data[s]) for s in stages])
mpi_std = np.array([np.std(mpi_data[s]) for s in stages])

# Create the plot
plt.figure(figsize=(10, 6))

# Plot RDMA data with error bars
plt.errorbar(stages, rdma_means, yerr=rdma_std, fmt='o-', label='SoftRoCE', 
             capsize=5, capthick=1.5, elinewidth=1.5, markersize=8, 
             color='blue', ecolor='lightblue')

# Plot MPI data with error bars
plt.errorbar(stages, mpi_means, yerr=mpi_std, fmt='s-', label='MPI',
             capsize=5, capthick=1.5, elinewidth=1.5, markersize=8,
             color='red', ecolor='lightcoral')

# Set logarithmic scale for x-axis
plt.xscale('log')

# Customize the plot
plt.grid(True, which="both", ls="-", alpha=0.2)
plt.xlabel('Number of Stages', fontsize=12)
plt.ylabel('Completion Time (seconds)', fontsize=12)
plt.title('SoftRoCE vs MPI All-to-All Performance Comparison', fontsize=14)
plt.legend(fontsize=10)

# Adjust layout
plt.tight_layout()

# Save plot to file
plt.savefig('alltoall_comparison.png', dpi=300, bbox_inches='tight')

# Show plot
plt.show()