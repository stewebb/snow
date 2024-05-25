import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import CubicSpline
import pandas as pd

'''
    Simulate snow effects in a place during winter.
    The place typically is located in temperate regions in the Northern Hemisphere.
'''

PLOTTING = True

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

"""
    Calculate the solar elevation angle based on latitude, declination of the sun, and time of day.

    Parameters:
        latitude (float): Latitude of the location in degrees. Positive for the northern hemisphere.
        declination (float): Sun's declination in degrees on the specified day. Negative in the northern winter.
        minutes (int): Minutes passed since midnight, ranging from 0 (00:00) to 1439 (23:59).

    Returns:
        float: Solar elevation angle in degrees above the horizon.

    Formula:
        Solar Elevation = arcsin(sin(latitude) * sin(declination) + cos(latitude) * cos(declination) * cos(H)),
    where H is the Hour Angle, calculated as (minutes/4 - 180) degrees, assuming 15 degrees per hour, centered at solar noon.
    """

def solar_elevation(latitude, declination, minutes):

    # Convert latitude and declination to radians
    latitude_rad = np.radians(latitude)
    declination_rad = np.radians(declination)

    # Convert minutes to hour angle, where 720 minutes (noon) corresponds to 0 degrees
    hour_angle = (minutes / 4) - 180

    # Calculate solar elevation angle in radians
    elevation_rad = np.arcsin(np.sin(latitude_rad) * np.sin(declination_rad) +
                              np.cos(latitude_rad) * np.cos(declination_rad) * np.cos(np.radians(hour_angle)))
    
    return np.degrees(elevation_rad)

def interpolate_color(angle, day_color, twilight_color, night_color):
    # Interpolate based on angle thresholds
    if angle >= 10:
        return day_color
    elif 0 < angle < 10:
        return np.interp(angle, [0, 10], [twilight_color, day_color], axis=0)
    else:
        return night_color

def interpolate_intensity(angle):
    """
    Calculate the intensity of sunlight based on the solar elevation angle.
    The intensity is maximum when the sun is at zenith (angle = 90 degrees) and decreases 
    towards zero as the sun approaches the horizon.

    Parameters:
        angle (float): Solar elevation angle in degrees.

    Returns:
        float: Normalized intensity of sunlight, between 0 and 1, where 0 is no intensity
        and 1 is the maximum intensity.
    """
    bias = 0.05
    zenith_angle = 90 - angle  # Convert elevation angle to zenith angle
    return max(-bias, np.cos(np.radians(zenith_angle))) + bias
    
    if angle > -10:
        # Calculate intensity as the cosine of the zenith angle, which is a realistic 
        # approximation of sunlight intensity variation.
        return max(0, np.cos(np.radians(zenith_angle)))
    else:
        return 0  # No sunlight if the sun is below the horizon



def interpolate_ambient_color(angle, day_sky, twilight_sky, night_sky):
    # Interpolate ambient sky color similar to sun color
    if angle >= 20:
        return day_sky
    elif 0 < angle < 20:
        return np.interp(angle, [0, 20], [twilight_sky, day_sky], axis=0)
    else:
        return night_sky

# Parameters
day_color = np.array([1.0, 1.0, 0.9])
twilight_color = np.array([1.0, 0.8, 0.6])
night_color = np.array([0.0, 0.0, 0.0])

day_sky = np.array([0.6, 0.7, 1.0])
twilight_sky = np.array([0.3, 0.2, 0.5])
night_sky = np.array([0.05, 0.05, 0.1])

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

latitude = 0.0  # Approximate latitude for Inverness, Scotland
declination = 23.5  # Declination on June 21
sun_elevations = [solar_elevation(latitude, declination, minute) for minute in minute_times]

# Mapping properties through interpolation
#sun_colors = [interpolate_color(angle, day_color, twilight_color, night_color) for angle in sun_elevations]
sun_intensities = [interpolate_intensity(angle) for angle in sun_elevations]
#ambient_colors = [interpolate_ambient_color(angle, day_sky, twilight_sky, night_sky) for angle in sun_elevations]


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
data.to_csv('data.csv', index=False)

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
    plt.xlabel('Time')
    plt.ylabel('Temperature (°C)')
    plt.title('Smooth Cubic Spline Daily Temperature Variation in Inverness, Scotland During Winter')
    plt.grid(True)
    plt.legend()
    #plt.show()

    # Plotting the results
    plt.figure(figsize=(10, 5))
    plt.plot(minute_times, snow_amounts, label='Snow Amount vs Time', color='blue')
    plt.title('Estimated Snow Amount Throughout the Day')
    plt.xlabel('Time')
    plt.ylabel('Snow Amount (0.0 - 1.0)')
    plt.xticks(ticks=xtick_values, labels=xtick_labels)

    plt.grid(True)
    plt.legend()

    plt.figure(figsize=(10, 5))
    plt.plot(minute_times, sun_elevations, label='Solar Elevation Angle', color='blue')
    plt.title('Sun Elevation Angle on June 21st in Inverness, Scotland')
    plt.xlabel('Time')
    plt.ylabel('Solar Elevation Angle (degrees)')
    #plt.axhline(0, color='grey', lw=0.5)  # Horizon line

    plt.xticks(ticks=xtick_values, labels=xtick_labels)
    plt.grid(True)
    plt.legend()

    # Plotting example: Sun Intensity
    plt.figure(figsize=(10, 5))
    plt.plot(minute_times, sun_intensities, label='Sun Intensity', color='blue')
    plt.title('Interpolated Sun Intensity Throughout the Day')
    plt.xlabel('Time (minutes from midnight)')
    plt.ylabel('Intensity')
    plt.grid(True)
    plt.xticks(ticks=xtick_values, labels=xtick_labels)

    plt.legend()
    plt.show()

    plt.show()