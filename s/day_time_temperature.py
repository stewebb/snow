import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import CubicSpline
import pandas as pd

#from matplotlib.ticker import FuncFormatter

# Time formatter for the plot
#def time_formatter(x, pos):
#    hour = int(x // 60)
#    minute = int(x % 60)
#    return f'{hour:02d}:{minute:02d}'


# Set x-axis to show 0-24 with integer labels
xtick_values = np.linspace(0, 1440, 25)  # Positions where ticks will be placed
xtick_labels = [int(label) for label in np.linspace(0, 24, 25)]  # Integer labels from 0 to 24

# Convert times to minutes in a day
times_segments = np.array([0.0, 1.0, 2.0,  3.0,  4.0,  5.0,  6.0,  7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0]) * 60
temps_segments = np.array([1.0, 0.0, 0.0, -1.0, -3.0, -4.0, -5.0, -2.0, 1.0, 3.0,  7.0, 10.0, 12.0, 13.0, 15.0, 16.0, 14.0, 12.0,  9.0,  7.0,  5.0,  4.0,  3.0,  2.0]) - 5.0

# Use CubicSpline for a smoother interpolation
cs = CubicSpline(times_segments, temps_segments, bc_type='natural')

# Generate times in minutes
minute_times = np.linspace(0, 1440, 1440)  # Time array for minutes in a day
minute_temperatures = cs(minute_times)

# Plotting
plt.figure(figsize=(10, 5))
plt.plot(minute_times, minute_temperatures, label='Temperature vs. Time', color='blue')
#plt.gca().xaxis.set_major_formatter(FuncFormatter(time_formatter))

plt.xticks(ticks=xtick_values, labels=xtick_labels)

plt.scatter(times_segments, temps_segments, color='blue')
plt.xlabel('Time (minutes)')
plt.ylabel('Temperature (°C)')
plt.title('Smooth Cubic Spline Daily Temperature Variation in a City During Winter')
plt.grid(True)
plt.legend()
#plt.show()

# Define the snow amount calculation function
def calculate_snow_amount(temperature):
    if temperature <= 0:
        return 1.0
    elif 0 < temperature < 5:
        return 1 - (temperature / 5.0)
    else:
        return 0.0

# Calculate snow amounts for the interpolated temperature values
snow_amounts = np.vectorize(calculate_snow_amount)(minute_temperatures)

# Plotting the results
plt.figure(figsize=(12, 6))
plt.plot(minute_times, snow_amounts, label='Snow Amount vs Time', color='blue')
plt.title('Estimated Snow Amount Throughout the Day')
plt.xlabel('Time (minutes)')
plt.ylabel('Snow Amount (0.0 - 1.0)')
plt.xticks(ticks=xtick_values, labels=xtick_labels)

plt.grid(True)
plt.legend()

# Create a DataFrame
data = pd.DataFrame({
    'Minute': minute_times,
    'Temperature': minute_temperatures,
    'SnowAmount': snow_amounts
})

# Save to CSV
data.to_csv('minute_data.csv', index=False)


plt.show()