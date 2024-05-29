import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap

# Define the colors and their boundaries for both mappings
colors = [
    (1.00, 0.50, 0.50),  # Red
    (1.00, 0.65, 0.50),  # Orange
    (1.00, 1.00, 0.60),  # Yellow
    (0.60, 1.00, 0.60),  # Green
    (0.50, 0.70, 1.00),  # Blue
    (0.40, 0.40, 0.70),  # Indigo
    (0.80, 0.60, 1.00)   # Violet
]
vis_boundaries = [0.0, 0.1666, 0.3333, 0.5000, 0.6666, 0.8333, 1.0]
angle_boundaries = [0.0, 0.2588, 0.5000, 0.7071, 0.8660, 0.9659, 1.0]

# Create smooth transition colormaps for both mappings
vis_cmap = LinearSegmentedColormap.from_list("Visibility", colors)
angle_cmap = LinearSegmentedColormap.from_list("AngleMap", colors)

# Create a figure and subplots
fig, axes = plt.subplots(nrows=2, ncols=1, figsize=(5, 3))

# Color bar for visibility
cb1 = fig.colorbar(plt.cm.ScalarMappable(cmap=vis_cmap), cax=axes[0], orientation='horizontal', label='Exposure value')
cb1.set_ticks(np.linspace(0, 1, 11))
cb1.set_ticklabels([f"{i:.1f}" for i in np.linspace(0, 1, 11)])

# Color bar for angle color mapping
cb2 = fig.colorbar(plt.cm.ScalarMappable(cmap=angle_cmap), cax=axes[1], orientation='horizontal', label='Angle between N and U')
cb2.set_ticks(angle_boundaries)
cb2.set_ticklabels(["90°", "75°", "60°", "45°", "30°", "15°", "0°"])

plt.tight_layout()
plt.show()
