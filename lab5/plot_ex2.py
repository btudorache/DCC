import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# Data for all paths
all_data = {
    1: """Flow ndp_11_10 flow_id 5 finished at 168.498 total bytes 2000000
Flow ndp_3_1 flow_id 9 finished at 173.272 total bytes 2000000
Flow ndp_14_13 flow_id 14 finished at 264.437 total bytes 2000000
Flow ndp_7_0 flow_id 12 finished at 294.914 total bytes 2000000
Flow ndp_6_4 flow_id 3 finished at 307.658 total bytes 2000000
Flow ndp_4_3 flow_id 8 finished at 316.536 total bytes 2000000
Flow ndp_9_5 flow_id 7 finished at 339.335 total bytes 2000000
Flow ndp_8_14 flow_id 1 finished at 344.982 total bytes 2000000
Flow ndp_12_6 flow_id 13 finished at 345.496 total bytes 2000000
Flow ndp_10_15 flow_id 10 finished at 345.683 total bytes 2000000
Flow ndp_2_7 flow_id 11 finished at 346.006 total bytes 2000000
Flow ndp_5_12 flow_id 4 finished at 351.244 total bytes 2000000
Flow ndp_1_2 flow_id 15 finished at 354.898 total bytes 2000000
Flow ndp_13_8 flow_id 2 finished at 476.778 total bytes 2000000
Flow ndp_15_9 flow_id 6 finished at 483.606 total bytes 2000000
Flow ndp_0_11 flow_id 16 finished at 505.064 total bytes 2000000""",

    4: """Flow ndp_11_10 flow_id 5 finished at 170.694 total bytes 2000000
Flow ndp_3_1 flow_id 9 finished at 178.137 total bytes 2000000
Flow ndp_10_15 flow_id 10 finished at 181.541 total bytes 2000000
Flow ndp_6_4 flow_id 3 finished at 185.3 total bytes 2000000
Flow ndp_12_6 flow_id 13 finished at 193.837 total bytes 2000000
Flow ndp_8_14 flow_id 1 finished at 193.917 total bytes 2000000
Flow ndp_14_13 flow_id 14 finished at 197.528 total bytes 2000000
Flow ndp_2_7 flow_id 11 finished at 203.578 total bytes 2000000
Flow ndp_13_8 flow_id 2 finished at 206.156 total bytes 2000000
Flow ndp_4_3 flow_id 8 finished at 211.066 total bytes 2000000
Flow ndp_15_9 flow_id 6 finished at 222.414 total bytes 2000000
Flow ndp_7_0 flow_id 12 finished at 230.57 total bytes 2000000
Flow ndp_9_5 flow_id 7 finished at 230.718 total bytes 2000000
Flow ndp_5_12 flow_id 4 finished at 230.755 total bytes 2000000
Flow ndp_1_2 flow_id 15 finished at 284.116 total bytes 2000000
Flow ndp_0_11 flow_id 16 finished at 295.279 total bytes 2000000""",

    8: """Flow ndp_11_10 flow_id 5 finished at 170.669 total bytes 2000000
Flow ndp_6_4 flow_id 3 finished at 177.661 total bytes 2000000
Flow ndp_10_15 flow_id 10 finished at 181.857 total bytes 2000000
Flow ndp_3_1 flow_id 9 finished at 182.931 total bytes 2000000
Flow ndp_7_0 flow_id 12 finished at 185.21 total bytes 2000000
Flow ndp_12_6 flow_id 13 finished at 199.77 total bytes 2000000
Flow ndp_13_8 flow_id 2 finished at 199.948 total bytes 2000000
Flow ndp_2_7 flow_id 11 finished at 206.721 total bytes 2000000
Flow ndp_5_12 flow_id 4 finished at 212.084 total bytes 2000000
Flow ndp_4_3 flow_id 8 finished at 224.451 total bytes 2000000
Flow ndp_15_9 flow_id 6 finished at 226.041 total bytes 2000000
Flow ndp_0_11 flow_id 16 finished at 236.18 total bytes 2000000
Flow ndp_14_13 flow_id 14 finished at 238.137 total bytes 2000000
Flow ndp_1_2 flow_id 15 finished at 238.697 total bytes 2000000
Flow ndp_8_14 flow_id 1 finished at 267.224 total bytes 2000000
Flow ndp_9_5 flow_id 7 finished at 280.556 total bytes 2000000""",

    16: """Flow ndp_11_10 flow_id 5 finished at 170.51 total bytes 2000000
Flow ndp_10_15 flow_id 10 finished at 183.925 total bytes 2000000
Flow ndp_3_1 flow_id 9 finished at 183.985 total bytes 2000000
Flow ndp_6_4 flow_id 3 finished at 188.97 total bytes 2000000
Flow ndp_8_14 flow_id 1 finished at 196.513 total bytes 2000000
Flow ndp_9_5 flow_id 7 finished at 202.685 total bytes 2000000
Flow ndp_2_7 flow_id 11 finished at 205.132 total bytes 2000000
Flow ndp_7_0 flow_id 12 finished at 206.736 total bytes 2000000
Flow ndp_1_2 flow_id 15 finished at 207.399 total bytes 2000000
Flow ndp_12_6 flow_id 13 finished at 207.674 total bytes 2000000
Flow ndp_4_3 flow_id 8 finished at 208.744 total bytes 2000000
Flow ndp_5_12 flow_id 4 finished at 209.688 total bytes 2000000
Flow ndp_0_11 flow_id 16 finished at 210.295 total bytes 2000000
Flow ndp_13_8 flow_id 2 finished at 211.573 total bytes 2000000
Flow ndp_15_9 flow_id 6 finished at 220.62 total bytes 2000000
Flow ndp_14_13 flow_id 14 finished at 229.249 total bytes 2000000""",

    32: """Flow ndp_11_10 flow_id 5 finished at 169.978 total bytes 2000000
Flow ndp_6_4 flow_id 3 finished at 177.59 total bytes 2000000
Flow ndp_13_8 flow_id 2 finished at 186.861 total bytes 2000000
Flow ndp_4_3 flow_id 8 finished at 188.234 total bytes 2000000
Flow ndp_3_1 flow_id 9 finished at 190.211 total bytes 2000000
Flow ndp_14_13 flow_id 14 finished at 190.468 total bytes 2000000
Flow ndp_9_5 flow_id 7 finished at 193.038 total bytes 2000000
Flow ndp_1_2 flow_id 15 finished at 200.415 total bytes 2000000
Flow ndp_15_9 flow_id 6 finished at 200.508 total bytes 2000000
Flow ndp_5_12 flow_id 4 finished at 204.837 total bytes 2000000
Flow ndp_10_15 flow_id 10 finished at 206.929 total bytes 2000000
Flow ndp_12_6 flow_id 13 finished at 207.082 total bytes 2000000
Flow ndp_8_14 flow_id 1 finished at 210.698 total bytes 2000000
Flow ndp_2_7 flow_id 11 finished at 212.737 total bytes 2000000
Flow ndp_7_0 flow_id 12 finished at 213.743 total bytes 2000000
Flow ndp_0_11 flow_id 16 finished at 226.858 total bytes 2000000"""}

def parse_data(data_str):
    flow_names = []
    finish_times = []
    flow_ids = []
    
    for line in data_str.split('\n'):
        parts = line.split()
        flow_name = parts[1]  # ndp_X_Y
        flow_id = int(parts[3])
        finish_time = float(parts[6])
        
        flow_names.append(flow_name)
        finish_times.append(finish_time)
        flow_ids.append(flow_id)
    
    # Sort by flow_id to maintain consistent ordering
    sorted_data = sorted(zip(flow_ids, flow_names, finish_times))
    return [x[1] for x in sorted_data], [x[2] for x in sorted_data]

# Create figure and 3D axes
fig = plt.figure(figsize=(15, 10))
ax = fig.add_subplot(111, projection='3d')

# Colors for different path counts
colors = {
    1: 'blue',
    4: 'red',
    8: 'green',
    16: 'purple',
    32: 'orange'
}

# Plot bars for each path count
x_pos = np.arange(len(parse_data(all_data[1])[0]))
dx = 0.8  # Width of bars
dy = 0.8  # Depth of bars
z_pos = np.zeros_like(x_pos)

for path_count in [1, 4, 8, 16, 32]:
    flow_names, finish_times = parse_data(all_data[path_count])
    y_pos = np.ones_like(x_pos) * path_count
    ax.bar3d(x_pos, y_pos, z_pos, dx, dy, finish_times, 
             color=colors[path_count], alpha=0.7,
             label=f'{path_count} {"Path" if path_count==1 else "Paths"}')

# Customize the plot
ax.set_xlabel('NDP Flow Name')
ax.set_ylabel('Number of Paths')
ax.set_zlabel('Finish Time (seconds)')
ax.set_title('Flow Finish Times by NDP Flow and Path Count')

# Set x-axis ticks with flow names
ax.set_xticks(x_pos + dx/2)
ax.set_xticklabels(flow_names, rotation=45, ha='right')

# Set y-axis ticks for number of paths
ax.set_yticks([1, 4, 8, 16, 32])
ax.set_yticklabels(['1', '4', '8', '16', '32'])

# Add legend
ax.legend()

# Adjust view angle for better visibility
ax.view_init(elev=20, azim=45)

# Add a grid for better depth perception
ax.grid(True)

# Adjust layout to prevent label cutoff
plt.tight_layout()

plt.show()