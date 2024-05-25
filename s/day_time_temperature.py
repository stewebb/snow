import numpy as np
import matplotlib.pyplot as plt

def daily_temperature(minute_of_day, min_temp=5, max_temp=25):
    # Normalize the time to a value between 0 and 2*pi
    radians = (minute_of_day / 1440) * 2 * np.pi
    
    # Use a sinusoidal function shifted to have a minimum around 5:00 (300 minutes)
    temperature = ((np.sin(radians + np.pi * 1) + 1) / 2) * (max_temp - min_temp) + min_temp
    
    return temperature

# Generate a list of minutes from 00:00 to 23:59
minutes = np.arange(0, 1440)

# Calculate temperatures for each minute
temperatures = [daily_temperature(minute) for minute in minutes]

# Plotting
plt.figure(figsize=(10, 5))
plt.plot(minutes, temperatures, label='Temperature through the day', color='blue')
plt.title('Daily Temperature Variation')
plt.xlabel('Minute of the Day')
plt.ylabel('Temperature (°C)')
plt.grid(True)
plt.xticks(np.arange(0, 1500, step=60), [f"{int(x/60):02d}:00" for x in np.arange(0, 1500, step=60)], rotation=45)
plt.tight_layout()
plt.legend()
plt.show()
