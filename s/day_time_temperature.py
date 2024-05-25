import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import CubicSpline

# Convert times to minutes in a day
times_segments = np.array([0.0, 1.0, 2.0,  3.0,  4.0,  5.0,  6.0,  7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0]) * 60
temps_segments = np.array([1.0, 0.0, 0.0, -1.0, -3.0, -4.0, -5.0, -2.0, 1.0, 3.0,  7.0, 10.0, 12.0, 13.0, 15.0, 16.0, 14.0, 12.0,  9.0,  7.0,  5.0,  4.0,  3.0,  2.0])

# Use CubicSpline for a smoother interpolation
cs = CubicSpline(times_segments, temps_segments, bc_type='natural')

# Generate times in minutes
minute_times = np.linspace(0, 1440, 1440)  # Time array for minutes in a day
minute_temperatures = cs(minute_times)

# Plotting
plt.figure(figsize=(10, 5))
plt.plot(minute_times, minute_temperatures, label='Temperature vs. Time', color='blue')
plt.scatter(times_segments, temps_segments, color='blue')
plt.xlabel('Time (minutes)')
plt.ylabel('Temperature (°C)')
plt.title('Smooth Cubic Spline Daily Temperature Variation in Canberra During Winter')
plt.grid(True)
plt.legend()
plt.show()