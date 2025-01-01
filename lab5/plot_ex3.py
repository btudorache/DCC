import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# Data for NDP 1 path
ndp_data = """56661.33 Mbps val 300 name ndp_sink_3_14
34432.00 Mbps val 297 name ndp_sink_9_0
48554.67 Mbps val 294 name ndp_sink_12_1
95232.00 Mbps val 255 name ndp_sink_5_3
49066.67 Mbps val 258 name ndp_sink_1_12
47530.67 Mbps val 261 name ndp_sink_6_13
47658.67 Mbps val 264 name ndp_sink_15_7
35669.33 Mbps val 267 name ndp_sink_4_10
23765.33 Mbps val 270 name ndp_sink_2_11
30464.00 Mbps val 273 name ndp_sink_11_5
95573.33 Mbps val 276 name ndp_sink_8_6
36224.00 Mbps val 279 name ndp_sink_0_9
30592.00 Mbps val 282 name ndp_sink_10_15
48213.33 Mbps val 285 name ndp_sink_13_2
95402.67 Mbps val 288 name ndp_sink_7_4
47744.00 Mbps val 291 name ndp_sink_14_8"""

# Data for Swift
swift_data = """23325.30 Mbps val 328 name swift_sink_3_14
59694.78 Mbps val 322 name swift_sink_9_0
24096.39 Mbps val 316 name swift_sink_12_1
51469.88 Mbps val 238 name swift_sink_5_3
74152.61 Mbps val 244 name swift_sink_1_12
18698.80 Mbps val 250 name swift_sink_6_13
58152.61 Mbps val 256 name swift_sink_15_7
42827.31 Mbps val 262 name swift_sink_4_10
42955.82 Mbps val 268 name swift_sink_2_11
71582.33 Mbps val 274 name swift_sink_11_5
53012.05 Mbps val 280 name swift_sink_8_6
73991.97 Mbps val 286 name swift_sink_0_9
26859.44 Mbps val 292 name swift_sink_10_15
73285.14 Mbps val 298 name swift_sink_13_2
77076.31 Mbps val 304 name swift_sink_7_4
91437.75 Mbps val 310 name swift_sink_14_8"""

def parse_data(data_str, prefix):
    sink_names = []
    throughputs = []
    sink_numbers = []  # To help with sorting
    
    for line in data_str.split('\n'):
        parts = line.split()
        throughput = float(parts[0])
        # Extract X_Y from sink name to maintain consistent ordering
        x_y = parts[-1].replace(f"{prefix}_sink_", "")
        x, y = map(int, x_y.split('_'))
        
        sink_names.append(f"{x}_{y}")
        throughputs.append(throughput)
        sink_numbers.append((x, y))
    
    # Sort by x then y coordinates
    sorted_data = sorted(zip(sink_numbers, sink_names, throughputs))
    return [x[1] for x in sorted_data], [x[2] for x in sorted_data]

# Create figure and 3D axes
fig = plt.figure(figsize=(15, 10))
ax = fig.add_subplot(111, projection='3d')

# Parse both datasets
ndp_names, ndp_throughputs = parse_data(ndp_data, "ndp")
swift_names, swift_throughputs = parse_data(swift_data, "swift")

# Create the 3D bar plots
x_pos = np.arange(len(ndp_names))
dx = 0.8  # Width of bars
dy = 0.8  # Depth of bars
z_pos = np.zeros_like(x_pos)

# Plot bars for NDP (blue)
y_pos_ndp = np.ones_like(x_pos)
ax.bar3d(x_pos, y_pos_ndp, z_pos, dx, dy, ndp_throughputs, 
         color='blue', alpha=0.7, label='NDP (1 Path)')

# Plot bars for Swift (red)
y_pos_swift = np.ones_like(x_pos) * 2
ax.bar3d(x_pos, y_pos_swift, z_pos, dx, dy, swift_throughputs, 
         color='red', alpha=0.7, label='Swift')

# Customize the plot
ax.set_xlabel('Sink ID (X_Y)')
ax.set_ylabel('Protocol')
ax.set_zlabel('Throughput (Mbps)')
ax.set_title('Throughput Comparison: NDP vs Swift')

# Set x-axis ticks with sink IDs
ax.set_xticks(x_pos + dx/2)
ax.set_xticklabels(ndp_names, rotation=45, ha='right')

# Set y-axis ticks for protocols
ax.set_yticks([1, 2])
ax.set_yticklabels(['NDP', 'Swift'])

# Add legend
ax.legend()

# Adjust view angle for better visibility
ax.view_init(elev=20, azim=45)

# Add a grid for better depth perception
ax.grid(True)

# Adjust layout to prevent label cutoff
plt.tight_layout()

plt.show()