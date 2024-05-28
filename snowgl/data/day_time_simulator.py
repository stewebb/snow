'''
    Simulate snow effects based on location and temperature.
    It generates relevant data to a CSV file, and the OpenGL C++ renderer will read the file.
'''

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from scipy.interpolate import CubicSpline
import pandas as pd

PLOTTING = True

'''
    Parameters
'''

# Sun (light source) color
sun_color_day      = np.array([1.00, 1.00, 0.90])
sun_color_twilight = np.array([1.00, 0.50, 0.00])
sun_color_night    = np.array([0.00, 0.00, 0.00])

# Sky (background) color
sky_color_day      = np.array([0.53, 0.81, 0.92])
sky_color_twilight = np.array([1.00, 0.76, 0.52])
sky_color_night    = np.array([0.10, 0.05, 0.10])

# Time and temperature segments in a day
times_segments = np.array([ 0,  1,  2,  3,   4,   5,   6,   7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23]) * 60
temps_segments = np.array([-7, -8, -8, -9, -11, -12, -13, -10, -9, -5, -1,  2,  4,  6,  9,  7,  6,  4,  1, -1, -3, -4, -5, -6])

# Location
latitude = -35.0
azimuth = 0

# Season
declination = 23.5  # 23.5 -> Jun 21st, -23.5 -> Dec 22nd, 0.0 -> Mar 21st / Sep 23rd

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
    Calculate the intensity of sunlight based on the solar elevation angle, taking into account
    the duration of daylight. The intensity is modeled to decrease exponentially as the angle
    decreases from zenith towards the horizon, modified by the length of the daylight.

    Parameters:
        angle (float): Solar elevation angle in degrees.
        daylight_minutes (int): Total minutes of daylight for the day.

    Returns:
        float: Normalized intensity of sunlight, between 0 and 1, where 0 indicates no 
               intensity and 1 indicates maximum intensity.

    Description:
        The formula uses an exponential decay based on the transformed zenith angle. The exponent
        in the decay function is modified by the daylight hours to adjust the curve based on 
        how long the sun is up. This helps model seasonal changes in sun intensity.
    Formula:
        intensity = exp(- (90 - angle) ^ (daylight_minutes / 60 + 1) / bias),
        where bias is set to 0.25 to slightly shift the curve for practical adjustments.
"""

def interpolate_intensity(angle, daylight_minutes):
    
    zenith_angle = np.radians(90 - angle)
    exponent = daylight_minutes / 60 + 1
    bias = np.power(np.pi * 0.5, exponent) / np.log(10)

    unbiased_intensity = np.exp(-1 * np.power(zenith_angle, exponent) / bias)
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
    elif -10 <= angle < 5:
        return np.array([
            np.interp(angle, [-10, 5], [night_sky[i], twilight_sky[i]])
            for i in range(len(day_sky))
        ])
    else:
        return night_sky

"""
    Calculate sunrise and sunset times from a series of sun elevation measurements and the duration of daylight.

    This function determines the times at which the sun rises and sets based on elevation data, along with the duration
    of daylight. It also identifies special conditions such as the Midnight Sun and Polar Night by checking if the sun 
    never sets or never rises, respectively, during a 24-hour period.

    Parameters:
    - sun_elevations (list or np.array): A sequence of sun elevation angles measured at consistent time intervals.
    - minute_times (list or np.array): The corresponding times in minutes from midnight for each sun elevation measurement.

    Returns:
    - tuple: 
        - sunrise_time (float): The time in minutes from midnight when the sun rises, or np.inf if it never rises (Polar Night).
        - sunset_time (float): The time in minutes from midnight when the sun sets, or -np.inf if it never sets (Midnight Sun).
        - daylight_time (int): The duration of daylight in minutes. For Midnight Sun, returns the total minutes in a day (1440).
          For Polar Night, returns 0.

    Special cases:
        - Midnight Sun: sunrise_time = -np.inf, sunset_time = np.inf, daylight_time is 1440
        - Polar Night: sunrise_time = np.inf, sunset_time = -np.inf, daylight_time is 0

    Note: For accurate results, ensure sun_elevations and minute_times have the same length and cover a full 24-hour period.
"""

def sunrise_sunset(sun_elevations, minute_times):

    sunrise_time = None
    sunset_time = None
    sun_never_sets = True
    sun_never_rises = True

    # Check elevations to update flags and find sunrise/sunset times
    for i in range(1, len(sun_elevations)):
        if sun_elevations[i-1] < 0 <= sun_elevations[i]:
            sunrise_time = minute_times[i]
            sun_never_rises = False
        elif sun_elevations[i-1] >= 0 > sun_elevations[i]:
            sunset_time = minute_times[i]
            sun_never_sets = False

        # Update flags for continuous conditions
        if sun_elevations[i] > 0:
            sun_never_rises = False
        if sun_elevations[i] < 0:
            sun_never_sets = False

    # Midnight Sun: sunrise at -inf, sunset at inf, daylight_time is 1440
    if sun_never_sets:
        return (-np.inf, np.inf, len(minute_times) - 1)      

    # Polar Night: sunrise at inf, sunset at -inf, daylight_time is 0
    elif sun_never_rises:
        return (np.inf, -np.inf, 0)   

    # Normal day sunrise and sunset times        
    else:
        return (sunrise_time, sunset_time, sunset_time - sunrise_time)  

# Time array for minutes in a day
minute_times = np.linspace(0, 1440, 1441) 

# Perform interpolation for temperature values
cs = CubicSpline(times_segments, temps_segments, bc_type='natural')
minute_temperatures = cs(minute_times)

# Calculate snow amounts based on temperature values
snow_amounts = np.vectorize(snow_amount)(minute_temperatures)

# Calculate solar elevation angle and the corresponding light directions
sun_elevations = [solar_elevation(latitude, declination, minute) for minute in minute_times]
light_directions = [solar_to_light_direction(elevation, azimuth) for elevation in sun_elevations]

# Calculate sunrise and sunset time
(sunrise_time, sunset_time, daylight_minutes) = sunrise_sunset(sun_elevations, minute_times)
#print(sunrise_time, sunset_time, daylight_minutes)

# Mapping properties through interpolation
sun_intensities = [interpolate_intensity(angle, daylight_minutes) for angle in sun_elevations]
sun_colors      = [interpolate_sky_color(angle, sun_color_day, sun_color_twilight, sun_color_night) for angle in sun_elevations]
sky_colors      = [interpolate_sky_color(angle, sky_color_day, sky_color_twilight, sky_color_night) for angle in sun_elevations]

# Extract components
light_direction_x = [ld[0] for ld in light_directions]
light_direction_y = [ld[1] for ld in light_directions]
light_direction_z = [ld[2] for ld in light_directions]

sun_color_r = [sc[0] for sc in sun_colors]
sun_color_g = [sc[1] for sc in sun_colors]
sun_color_b = [sc[2] for sc in sun_colors]

sky_color_r = [sc[0] for sc in sky_colors]
sky_color_g = [sc[1] for sc in sky_colors]
sky_color_b = [sc[2] for sc in sky_colors]

# Create DataFrame in Pandas
data = pd.DataFrame({
    'Minute': minute_times,
    'Temperature': minute_temperatures,
    'SnowAmount': snow_amounts,

    'LightIntensity': sun_intensities,
    'ElevationAngle': sun_elevations,

    'LightDirectionX': light_direction_x,
    'LightDirectionY': light_direction_y,
    'LightDirectionZ': light_direction_z,

    'SkyColorR': sky_color_r,
    'SkyColorG': sky_color_g,
    'SkyColorB': sky_color_b,

    'SunColorR': sun_color_r,
    'SunColorG': sun_color_g,
    'SunColorB': sun_color_b
})

# Round values
data['Minute']          = data['Minute'].astype(int)
data['Temperature']     = data['Temperature'].round(2)
data['SnowAmount']      = data['SnowAmount'].round(2)

data['LightIntensity'] = data['LightIntensity'].round(2)
data['ElevationAngle'] = data['ElevationAngle'].round(2)

data['LightDirectionX'] = data['LightDirectionX'].round(2)
data['LightDirectionY'] = data['LightDirectionY'].round(2)
data['LightDirectionZ'] = data['LightDirectionZ'].round(2)

data['SkyColorR'] = data['SkyColorR'].round(2)
data['SkyColorG'] = data['SkyColorG'].round(2)
data['SkyColorB'] = data['SkyColorB'].round(2)

data['SunColorR'] = data['SunColorR'].round(2)
data['SunColorG'] = data['SunColorG'].round(2)
data['SunColorB'] = data['SunColorB'].round(2)

# Output format
data['Time']            = data['Minute'].apply(minutes_to_time)
data['Temperature']     = data['Temperature'].apply(lambda x: f"{x:.2f}")
data['SnowAmount']      = data['SnowAmount'].apply(lambda x: f"{x:.2f}")

data['LightIntensity']  = data['LightIntensity'].apply(lambda x: f"{x:.2f}")
data['ElevationAngle']  = data['ElevationAngle'].apply(lambda x: f"{x:.2f}")

data['LightDirectionX'] = data['LightDirectionX'].apply(lambda x: f"{x:.2f}")
data['LightDirectionY'] = data['LightDirectionY'].apply(lambda x: f"{x:.2f}")
data['LightDirectionZ'] = data['LightDirectionZ'].apply(lambda x: f"{x:.2f}")

data['SkyColorR'] = data['SkyColorR'].apply(lambda x: f"{x:.2f}")
data['SkyColorG'] = data['SkyColorG'].apply(lambda x: f"{x:.2f}")
data['SkyColorB'] = data['SkyColorB'].apply(lambda x: f"{x:.2f}")

data['SunColorR'] = data['SunColorR'].apply(lambda x: f"{x:.2f}")
data['SunColorG'] = data['SunColorG'].apply(lambda x: f"{x:.2f}")
data['SunColorB'] = data['SunColorB'].apply(lambda x: f"{x:.2f}")

# Save to CSV for OpenGL C++ renderer
data = data[[
    'Time', 
    'Minute', 
    'Temperature', 
    'SnowAmount', 

    'LightIntensity',
    'ElevationAngle', 

    'LightDirectionX',
    'LightDirectionY',
    'LightDirectionZ',

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

    # Initialize sub-plots
    fig, axs = plt.subplots(2, 3, figsize=(20, 7.5))

    # Temperature Plot
    axs[0, 0].plot(minute_times, minute_temperatures, color='blue')
    axs[0, 0].scatter(times_segments, temps_segments, color='blue')
    axs[0, 0].set_xlabel('Time (hours)')
    axs[0, 0].set_ylabel('Temperature (°C)')
    axs[0, 0].set_title('Temperature Throughout the Day')
    axs[0, 0].set_xticks(xtick_values)
    axs[0, 0].set_xticklabels(xtick_labels)
    axs[0, 0].grid(True)
    axs[0, 0].legend()

    # Snow Amount Plot
    axs[0, 1].plot(minute_times, snow_amounts, color='blue')
    axs[0, 1].set_title('Snow Amount Throughout the Day')
    axs[0, 1].set_xlabel('Time (hours)')
    axs[0, 1].set_ylabel('Snow Amount (0.0 - 1.0)')
    axs[0, 1].set_xticks(xtick_values)
    axs[0, 1].set_xticklabels(xtick_labels)
    axs[0, 1].grid(True)
    axs[0, 1].legend()

    # Solar Elevation Angle Plot
    axs[0, 2].plot(minute_times, sun_elevations, color='blue')
    axs[0, 2].set_title('Solar Elevation Angle')
    axs[0, 2].set_xlabel('Time (hours)')
    axs[0, 2].set_ylabel('Solar Elevation Angle (degrees)')
    axs[0, 2].set_xticks(xtick_values)
    axs[0, 2].set_xticklabels(xtick_labels)
    axs[0, 2].scatter([sunrise_time, sunset_time], [0, 0], color='red', zorder=5)
    axs[0, 2].grid(True)
    axs[0, 2].legend()

    # Sun Intensity Plot
    axs[1, 0].plot(minute_times, sun_intensities, color='blue')
    axs[1, 0].set_title('Sunlight Intensity Throughout the Day')
    axs[1, 0].set_xlabel('Time (hours)')
    axs[1, 0].set_ylabel('Intensity')
    axs[1, 0].set_xticks(xtick_values)
    axs[1, 0].set_xticklabels(xtick_labels)
    axs[1, 0].grid(True)
    axs[1, 0].legend()

    # Sun Color Plot
    sun_colors = np.column_stack((sun_color_r, sun_color_g, sun_color_b))
    sun_sm = plt.cm.ScalarMappable(cmap=ListedColormap(sun_colors), norm=plt.Normalize(0, len(minute_times) - 1))
    sun_sm.set_array([])
    sun_cbar = plt.colorbar(sun_sm, ax=axs[1, 1], orientation='horizontal', ticks=xtick_values)
    sun_cbar.set_label('Time of Day')
    sun_cbar.ax.set_xticks(xtick_values)
    sun_cbar.ax.set_xticklabels(xtick_labels)
    axs[1, 1].set_title('Sun Color Throughout the Day')

    
    # Sky Color Plot
    sky_colors = np.column_stack((sky_color_r, sky_color_g, sky_color_b))
    sky_sm = plt.cm.ScalarMappable(cmap=ListedColormap(sky_colors), norm=plt.Normalize(0, len(minute_times) - 1))
    sky_sm.set_array([])
    sky_cbar = plt.colorbar(sky_sm, ax=axs[1, 2], orientation='horizontal', ticks=xtick_values)
    sky_cbar.set_label('Time of Day')
    sky_cbar.ax.set_xticks(xtick_values)
    sky_cbar.ax.set_xticklabels(xtick_labels)
    axs[1, 2].set_title('Sky Color Throughout the Day')    

    # Adjust layout to prevent overlap
    plt.tight_layout()
    plt.show()