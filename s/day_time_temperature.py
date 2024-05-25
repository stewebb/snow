import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import CubicSpline
import pandas as pd

'''
    Simulate snow effects in a place during winter.
    The place typically is located in temperate regions in the Northern Hemisphere.
'''

PLOTTING = False

# Calculate snow amount based on temperature
# Output range: [0, 1]. 0 -> No snow 1 -> Full snow
def snow_amount(temperature):
    if temperature <= 0:
        return 1.0
    elif 0 < temperature < 5:
        return 1 - (temperature / 5.0)
    else:
        return 0.0
    
# Function to convert minutes to HH:MM format
def minutes_to_time(mins):
    hours = mins // 60
    minutes = mins % 60
    return f"{hours:02}:{minutes:02}"

# The time-temperature relationship
times_segments = np.array([ 0,  1,  2,  3,  4,  5,   6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23]) * 60
temps_segments = np.array([-4, -5, -5, -6, -8, -9, -10, -7, -6, -2,  2,  5,  7,  9, 12, 10,  9,  7,  4,  2,  0, -1, -2, -3])

# Time array for minutes in a day
minute_times = np.linspace(0, 1440, 1441) 

# Perform interpolation for temperatures
cs = CubicSpline(times_segments, temps_segments, bc_type='natural')
minute_temperatures = cs(minute_times)

# Calculate snow amounts for the interpolated temperature values
snow_amounts = np.vectorize(snow_amount)(minute_temperatures)

# Create DataFrame in Pandas
data = pd.DataFrame({
    'Minute': minute_times,
    'Temperature': minute_temperatures,
    'SnowAmount': snow_amounts
})

# Round 'Minute' to integers and other columns to two decimal places
data['Minute'] = data['Minute'].astype(int)
data['Temperature'] = data['Temperature'].round(2)
data['SnowAmount'] = data['SnowAmount'].round(2)

# Convert numeric columns to string format with two decimal places
data['Temperature'] = data['Temperature'].apply(lambda x: f"{x:.2f}")
data['SnowAmount'] = data['SnowAmount'].apply(lambda x: f"{x:.2f}")

# Apply the function to the 'Minute' column and create a new 'Time' column
data['Time'] = data['Minute'].apply(minutes_to_time)

# Save to CSV for OpenGL C++ renderer
data = data[['Time', 'Minute', 'Temperature', 'SnowAmount']]
data.to_csv('minute_data.csv', index=False)


# Plotting
if PLOTTING:
        
    # Set x-axis to show 0-24 with integer labels
    xtick_values = np.linspace(0, 1440, 25)
    xtick_labels = [int(label) for label in np.linspace(0, 24, 25)]

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

    plt.show()