import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# Data for all paths (1, 4, 8, 16, 32)
all_data = {
    1: """56661.33 Mbps val 300 name ndp_sink_3_14
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
47744.00 Mbps val 291 name ndp_sink_14_8""",

    4: """62080.00 Mbps val 300 name ndp_sink_3_14
49280.00 Mbps val 297 name ndp_sink_9_0
51413.33 Mbps val 294 name ndp_sink_12_1
62592.00 Mbps val 255 name ndp_sink_5_3
81493.33 Mbps val 258 name ndp_sink_1_12
70912.00 Mbps val 261 name ndp_sink_6_13
58709.33 Mbps val 264 name ndp_sink_15_7
61098.67 Mbps val 267 name ndp_sink_4_10
49877.33 Mbps val 270 name ndp_sink_2_11
59520.00 Mbps val 273 name ndp_sink_11_5
51157.33 Mbps val 276 name ndp_sink_8_6
66261.33 Mbps val 279 name ndp_sink_0_9
80298.67 Mbps val 282 name ndp_sink_10_15
51882.67 Mbps val 285 name ndp_sink_13_2
81664.00 Mbps val 288 name ndp_sink_7_4
67456.00 Mbps val 291 name ndp_sink_14_8""",

    8: """75136.00 Mbps val 300 name ndp_sink_3_14
72277.33 Mbps val 297 name ndp_sink_9_0
69760.00 Mbps val 294 name ndp_sink_12_1
71381.33 Mbps val 255 name ndp_sink_5_3
83114.67 Mbps val 258 name ndp_sink_1_12
74368.00 Mbps val 261 name ndp_sink_6_13
89472.00 Mbps val 264 name ndp_sink_15_7
82645.33 Mbps val 267 name ndp_sink_4_10
78890.67 Mbps val 270 name ndp_sink_2_11
93696.00 Mbps val 273 name ndp_sink_11_5
81365.33 Mbps val 276 name ndp_sink_8_6
85589.33 Mbps val 279 name ndp_sink_0_9
78250.67 Mbps val 282 name ndp_sink_10_15
69034.67 Mbps val 285 name ndp_sink_13_2
95829.33 Mbps val 288 name ndp_sink_7_4
84309.33 Mbps val 291 name ndp_sink_14_8""",

    16: """77440.00 Mbps val 300 name ndp_sink_3_14
71296.00 Mbps val 297 name ndp_sink_9_0
76416.00 Mbps val 294 name ndp_sink_12_1
75434.67 Mbps val 255 name ndp_sink_5_3
74666.67 Mbps val 258 name ndp_sink_1_12
78677.33 Mbps val 261 name ndp_sink_6_13
84565.33 Mbps val 264 name ndp_sink_15_7
83498.67 Mbps val 267 name ndp_sink_4_10
75136.00 Mbps val 270 name ndp_sink_2_11
87381.33 Mbps val 273 name ndp_sink_11_5
85162.67 Mbps val 276 name ndp_sink_8_6
67029.33 Mbps val 279 name ndp_sink_0_9
91434.67 Mbps val 282 name ndp_sink_10_15
72064.00 Mbps val 285 name ndp_sink_13_2
93482.67 Mbps val 288 name ndp_sink_7_4
83285.33 Mbps val 291 name ndp_sink_14_8""",

    32: """86016.00 Mbps val 300 name ndp_sink_3_14
81066.67 Mbps val 297 name ndp_sink_9_0
82858.67 Mbps val 294 name ndp_sink_12_1
88746.67 Mbps val 255 name ndp_sink_5_3
79104.00 Mbps val 258 name ndp_sink_1_12
85205.33 Mbps val 261 name ndp_sink_6_13
77482.67 Mbps val 264 name ndp_sink_15_7
85888.00 Mbps val 267 name ndp_sink_4_10
86485.33 Mbps val 270 name ndp_sink_2_11
72874.67 Mbps val 273 name ndp_sink_11_5
79786.67 Mbps val 276 name ndp_sink_8_6
74496.00 Mbps val 279 name ndp_sink_0_9
76842.67 Mbps val 282 name ndp_sink_10_15
81749.33 Mbps val 285 name ndp_sink_13_2
93184.00 Mbps val 288 name ndp_sink_7_4
82602.67 Mbps val 291 name ndp_sink_14_8"""}

def parse_data(data_str):
    sink_names = []
    throughputs = []
    for line in data_str.split('\n'):
        parts = line.split()
        throughput = float(parts[0])
        sink_name = parts[-1]
        sink_names.append(sink_name)
        throughputs.append(throughput)
    return sink_names, throughputs

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
    sink_names, throughputs = parse_data(all_data[path_count])
    y_pos = np.ones_like(x_pos) * path_count
    ax.bar3d(x_pos, y_pos, z_pos, dx, dy, throughputs, 
             color=colors[path_count], alpha=0.7,
             label=f'{path_count} {"Path" if path_count==1 else "Paths"}')

# Customize the plot
ax.set_xlabel('NDP Sink Name')
ax.set_ylabel('Number of Paths')
ax.set_zlabel('Throughput (Mbps)')
ax.set_title('Flow Throughput Distribution by NDP Sink and Path Count')

# Set x-axis ticks with sink names
ax.set_xticks(x_pos + dx/2)
ax.set_xticklabels(sink_names, rotation=45, ha='right')

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