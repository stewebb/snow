'''
    Simulate snow effects based on location and temperature.
    It generates relevant data to a CSV file, and the OpenGL C++ renderer will read the file.
'''

import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import CubicSpline
import pandas as pd

PLOTTING = True

'''
    Parameters
'''

# Sun (light source) color
day_color = np.array([1.0, 0.9, 0.5])
twilight_color = np.array([1.0, 0.5, 0.0])
night_color = np.array([0.0, 0.0, 0.0])

day_sky = np.array([0.53, 0.81, 0.92])
twilight_sky = np.array([0.99, 0.76, 0.52])
night_sky = np.array([0.10, 0.05, 0.10])

# The time-temperature relationship
times_segments = np.array([ 0,  1,  2,  3,   4,   5,   6,   7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23]) * 60
temps_segments = np.array([-7, -8, -8, -9, -11, -12, -13, -10, -9, -5, -1,  2,  4,  6,  9,  7,  6,  4,  1, -1, -3, -4, -5, -6])



"""
    Calculate the amount of snow based on the temperature.

    This function computes the amount of snow as a normalized value between 0 and 1, where 0 indicates no snow and 1 indicates full snow coverage. The calculation is based on the temperature using the following rules:
    - If the temperature is less than or equal to 0 degrees Celsius, full snow coverage (1.0) is returned.
    - If the temperature is between 0 and 5 degrees Celsius, the snow amount decreases linearly from 1 to 0 as temperature increases. The formula used is: 1 - (temperature / 5).
    - Temperatures 5 degrees Celsius or higher result in no snow (0.0).

    Parameters:
    - temperature (float): The current temperature in degrees Celsius.

    Returns:
    - float: A float between 0 and 1 representing the snow amount.

    Formula:
    - For temperature <= 0: snow_amount = 1.0
    - For 0 < temperature < 5: snow_amount = 1 - (temperature / 5)
    - For temperature >= 5: snow_amount = 0.0
"""

def snow_amount(temperature):
    if temperature <= 0:
        return 1.0
    elif 0 < temperature < 5:
        return 1 - (temperature / 5.0)
    else:
        return 0.0
    
"""
    Convert a number of minutes into a time string formatted as HH:MM.

    This function takes an integer representing the total number of minutes and converts it into a string formatted as hours and minutes (HH:MM). The function calculates hours as the integer division of minutes by 60 and the remainder as the minutes. It ensures that single-digit hours and minutes are zero-padded to maintain a consistent two-digit format.

    Parameters:
    - mins (int): Total number of minutes to convert.

    Returns:
    - str: A string representing the time in HH:MM format.

    Example:
    - Input: 150
    - Output: '02:30'
"""

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

"""
    Calculate the intensity of sunlight based on the solar elevation angle.
    The intensity is maximum when the sun is at zenith (angle = 90 degrees) and decreases 
    towards zero as the sun approaches the horizon. This function uses an exponential 
    decay function based on the transformed zenith angle, adjusted to ensure that the 
    intensity strictly lies between 0 and 1.

    Parameters:
        angle (float): Solar elevation angle in degrees.

    Returns:
        float: Normalized intensity of sunlight, between 0 and 1, where 0 indicates no 
        intensity and 1 indicates maximum intensity.

    Formula:
        intensity = exp(-((90 - angle) * (1 - bias))^8),
        where bias is set to 0.25 to slightly shift the curve for practical adjustments.
"""

def interpolate_intensity(angle):

    bias = 0.25
    zenith_angle = np.radians(90 - angle) * (1.00 - bias)
    unbiased_intensity = np.exp(-1 * np.power(zenith_angle, 8))
    return np.clip(unbiased_intensity, 0.0, 1.0)

"""
    Convert solar elevation and azimuth angles to a light direction vector suitable for use in 3D graphics.

    This function transforms the solar angles into a directional vector that indicates the direction from which light 
    would be coming in a 3D scene. The elevation angle is measured from the horizon upwards, while the azimuth is 
    measured from North going clockwise. The calculations return a vector for a right-handed coordinate system, 
    where the x-axis points East, the y-axis points North, and the z-axis points Up.

    Parameters:
    elevation_deg (float): The solar elevation angle in degrees.
    azimuth_deg (float): The solar azimuth angle in degrees, measured clockwise from North.

    Returns:
    numpy.ndarray: A 3D vector representing the light direction. The components of this vector are:
        - x: Negative sine of the azimuth times the cosine of the elevation, representing the Eastward component.
        - y: Negative cosine of the azimuth times the cosine of the elevation, representing the Northward component.
        - z: Sine of the elevation, representing the upward component towards the zenith.
"""

def solar_to_light_direction(elevation_deg, azimuth_deg):

    # Convert degrees to radians
    elevation_rad = np.radians(elevation_deg)
    azimuth_rad = np.radians(azimuth_deg)
    
    # Calculate the components of the direction vector
    x = -np.sin(azimuth_rad) * np.cos(elevation_rad)
    y = -np.cos(azimuth_rad) * np.cos(elevation_rad)
    z = np.sin(elevation_rad)
    
    return np.array([x, y, z])

"""
    Interpolates the ambient sky color based on the solar elevation angle to simulate the changing sky colors from day to night.

    This function returns a color that transitions smoothly between specified colors for daytime, twilight, and nighttime 
    based on the solar elevation angle. The transition ranges are defined to reflect typical conditions during those times:
    - Daytime color is used when the angle is 20 degrees or more.
    - Twilight color gradually blends into daytime as the angle changes from 5 to 20 degrees.
    - Nighttime color transitions to twilight color as the angle goes from -10 to 5 degrees.
    - Strict nighttime color is used when the angle is less than -10 degrees.

    Parameters:
        angle (float): Solar elevation angle in degrees.
        day_sky (numpy.ndarray): RGB color for the daytime sky.
        twilight_sky (numpy.ndarray): RGB color for the twilight sky.
        night_sky (numpy.ndarray): RGB color for the nighttime sky.

    Returns:
        numpy.ndarray: The interpolated RGB color of the sky based on the solar elevation angle.
"""

def interpolate_sky_color(angle, day_sky, twilight_sky, night_sky):
    if angle >= 20:
        return day_sky
    elif 5 <= angle < 20:
        return np.array([
            np.interp(angle, [5, 20], [twilight_sky[i], day_sky[i]])
            for i in range(len(day_sky))
        ])
    elif -10 <= angle < 10:
        return np.array([
            np.interp(angle, [-10, 10], [night_sky[i], twilight_sky[i]])
            for i in range(len(day_sky))
        ])
    else:
        return night_sky


# Time array for minutes in a day
minute_times = np.linspace(0, 1440, 1441) 

# Perform interpolation for temperatures
cs = CubicSpline(times_segments, temps_segments, bc_type='natural')
minute_temperatures = cs(minute_times)

# Calculate snow amounts for the interpolated temperature values
snow_amounts = np.vectorize(snow_amount)(minute_temperatures)

latitude = 10.5  # Approximate latitude for Inverness, Scotland
declination = 0.0  # Declination on June 21
sun_elevations = [solar_elevation(latitude, declination, minute) for minute in minute_times]

# Mapping properties through interpolation
sun_colors = [interpolate_sky_color(angle, day_color, twilight_color, night_color) for angle in sun_elevations]
sun_intensities = [interpolate_intensity(angle) for angle in sun_elevations]
sky_colors = [interpolate_sky_color(angle, day_sky, twilight_sky, night_sky) for angle in sun_elevations]

#print(sky_colors)

#elevation = 30  # degrees above the horizon
azimuth = 90    # degrees from North going clockwise (East)
light_directions = [solar_to_light_direction(elevation, azimuth) for elevation in sun_elevations]
#print(Light_directions)

# Extract components of light directions
light_direction_x = [ld[0] for ld in light_directions]
light_direction_y = [ld[1] for ld in light_directions]
light_direction_z = [ld[2] for ld in light_directions]


sky_color_r = [sc[0] for sc in sky_colors]
sky_color_g = [sc[1] for sc in sky_colors]
sky_color_b = [sc[2] for sc in sky_colors]

sun_color_r = [sc[0] for sc in sun_colors]
sun_color_g = [sc[1] for sc in sun_colors]
sun_color_b = [sc[2] for sc in sun_colors]

#print(sky_color_x)

# Create DataFrame in Pandas
data = pd.DataFrame({
    'Minute': minute_times,
    'Temperature': minute_temperatures,
    'SnowAmount': snow_amounts,
    'LightIntensity': sun_intensities,
    'ElevationAngle': sun_elevations,
    'lightDirectionX': light_direction_x,
    'lightDirectionY': light_direction_y,
    'lightDirectionZ': light_direction_z,

    'SkyColorR': sky_color_r,
    'SkyColorG': sky_color_g,
    'SkyColorB': sky_color_b,

    'SunColorR': sun_color_r,
    'SunColorG': sun_color_g,
    'SunColorB': sun_color_b
})

# Round 'Minute' to integers and other columns to two decimal places
data['Minute'] = data['Minute'].astype(int)
data['Temperature'] = data['Temperature'].round(2)
data['SnowAmount'] = data['SnowAmount'].round(2)
data['lightDirectionX'] = data['lightDirectionX'].round(2)
data['lightDirectionY'] = data['lightDirectionY'].round(2)
data['lightDirectionZ'] = data['lightDirectionZ'].round(2)

data['SkyColorR'] = data['SkyColorR'].round(2)
data['SkyColorG'] = data['SkyColorG'].round(2)
data['SkyColorB'] = data['SkyColorB'].round(2)

data['SunColorR'] = data['SunColorR'].round(2)
data['SunColorG'] = data['SunColorG'].round(2)
data['SunColorB'] = data['SunColorB'].round(2)

data['ElevationAngle'] = data['ElevationAngle'].round(2)

# Convert numeric columns to string format with two decimal places
data['Temperature'] = data['Temperature'].apply(lambda x: f"{x:.2f}")
data['SnowAmount'] = data['SnowAmount'].apply(lambda x: f"{x:.2f}")

data['lightDirectionX'] = data['lightDirectionX'].apply(lambda x: f"{x:.2f}")
data['lightDirectionY'] = data['lightDirectionY'].apply(lambda x: f"{x:.2f}")
data['lightDirectionZ'] = data['lightDirectionZ'].apply(lambda x: f"{x:.2f}")

data['SkyColorR'] = data['SkyColorR'].apply(lambda x: f"{x:.2f}")
data['SkyColorG'] = data['SkyColorG'].apply(lambda x: f"{x:.2f}")
data['SkyColorB'] = data['SkyColorB'].apply(lambda x: f"{x:.2f}")

data['SunColorR'] = data['SunColorR'].apply(lambda x: f"{x:.2f}")
data['SunColorG'] = data['SunColorG'].apply(lambda x: f"{x:.2f}")
data['SunColorB'] = data['SunColorB'].apply(lambda x: f"{x:.2f}")

# Apply the function to the 'Minute' column and create a new 'Time' column
data['Time'] = data['Minute'].apply(minutes_to_time)

data['LightIntensity'] = data['LightIntensity'].apply(lambda x: f"{x:.2f}")
data['ElevationAngle'] = data['ElevationAngle'].apply(lambda x: f"{x:.2f}")


# Save to CSV for OpenGL C++ renderer
data = data[[
    'Time', 
    'Minute', 
    'Temperature', 
    'SnowAmount', 
    'LightIntensity',
    'ElevationAngle', 
    'lightDirectionX',
    'lightDirectionY',
    'lightDirectionZ',
    'SkyColorR',
    'SkyColorG',
    'SkyColorB',
    'SunColorR',
    'SunColorG',
    'SunColorB'
]]

data.to_csv('data.csv', index=False)

# Plotting
if PLOTTING:
        
    # Set x-axis to show 0-24 with integer labels
    xtick_values = np.linspace(0, 1440, 25)
    xtick_labels = [int(label) for label in np.linspace(0, 24, 25)]

    fig, axs = plt.subplots(2, 2, figsize=(15, 10))  # 2x2 grid of plots

    # Temperature vs. Time Plot
    axs[0, 0].plot(minute_times, minute_temperatures, label='Temperature vs. Time', color='blue')
    axs[0, 0].scatter(times_segments, temps_segments, color='blue')
    axs[0, 0].set_xlabel('Time (hours)')
    axs[0, 0].set_ylabel('Temperature (°C)')
    axs[0, 0].set_title('Daily Temperature Variation')
    axs[0, 0].set_xticks(xtick_values)
    axs[0, 0].set_xticklabels(xtick_labels)
    axs[0, 0].grid(True)
    axs[0, 0].legend()

    # Snow Amount vs Time Plot
    axs[0, 1].plot(minute_times, snow_amounts, label='Snow Amount vs Time', color='blue')
    axs[0, 1].set_title('Estimated Snow Amount')
    axs[0, 1].set_xlabel('Time (hours)')
    axs[0, 1].set_ylabel('Snow Amount (0.0 - 1.0)')
    axs[0, 1].set_xticks(xtick_values)
    axs[0, 1].set_xticklabels(xtick_labels)
    axs[0, 1].grid(True)
    axs[0, 1].legend()

    # Solar Elevation Angle Plot
    axs[1, 0].plot(minute_times, sun_elevations, label='Solar Elevation Angle', color='blue')
    axs[1, 0].set_title('Sun Elevation Angle')
    axs[1, 0].set_xlabel('Time (hours)')
    axs[1, 0].set_ylabel('Solar Elevation Angle (degrees)')
    axs[1, 0].set_xticks(xtick_values)
    axs[1, 0].set_xticklabels(xtick_labels)
    axs[1, 0].axhline(0, color='grey', lw=0.5)  # Horizon line
    axs[1, 0].grid(True)
    axs[1, 0].legend()

    # Sun Intensity Plot
    axs[1, 1].plot(minute_times, sun_intensities, label='Sun Intensity', color='blue')
    axs[1, 1].set_title('Sun Intensity Throughout the Day')
    axs[1, 1].set_xlabel('Time (hours)')
    axs[1, 1].set_ylabel('Intensity')
    axs[1, 1].set_xticks(xtick_values)
    axs[1, 1].set_xticklabels(xtick_labels)
    axs[1, 1].grid(True)
    axs[1, 1].legend()

    # Adjust layout to prevent overlap
    plt.tight_layout()
    plt.show()