import numpy as np
import pandas as pd

def interpolate_rgb(color1, color2, mix):
    return tuple([int(color1[i] * (1 - mix) + color2[i] * mix) for i in range(3)])

def interpolate_angle(angle1, angle2, mix):
    if angle1 is None or angle2 is None:
        return None
    return angle1 * (1 - mix) + angle2 * mix

def interpolate_sunlight_rgb(time, sunrise, noon, sunset):
    if time <= sunrise:  # Before sunrise
        return (255, 204, 153)  # Orange
    elif sunrise < time < noon:  # Sunrise to noon
        progress = (time - sunrise) / (noon - sunrise)
        return interpolate_rgb((255, 204, 153), (255, 255, 255), progress)  # Orange to white
    elif noon <= time < sunset:  # Noon to sunset
        progress = (time - noon) / (sunset - noon)
        return interpolate_rgb((255, 255, 255), (255, 204, 153), progress)  # White to orange
    else:  # After sunset
        return (255, 204, 153)  # Orange

def get_sky_color(minute, sunrise, noon, sunset):
    if minute <= sunrise or minute >= sunset:
        return interpolate_rgb((25, 25, 112), (255, 204, 153), np.sin(np.pi * (minute - 0) / (sunrise - 0))) if minute <= sunrise else (25, 25, 112)
    elif sunrise < minute < noon:
        return interpolate_rgb((255, 204, 153), (135, 206, 235), np.sin(np.pi * (minute - sunrise) / (noon - sunrise)))
    else:
        return interpolate_rgb((135, 206, 235), (255, 204, 153), np.sin(np.pi * (minute - noon) / (sunset - noon)))

minutes_in_day = 1440
sunrise = 360
noon = 720
sunset = 1080
intensities = np.zeros(minutes_in_day)
angles = np.zeros(minutes_in_day)
sky_colors = np.zeros((minutes_in_day, 3), dtype=int)
sunlight_colors = np.zeros((minutes_in_day, 3), dtype=int)

for minute in range(minutes_in_day):
    sky_colors[minute] = get_sky_color(minute, sunrise, noon, sunset)
    sunlight_colors[minute] = interpolate_sunlight_rgb(minute, sunrise, noon, sunset)

results = [(minute, intensity, 'None' if np.isnan(angle) else angle, tuple(sky_color), tuple(sunlight_color)) 
           for minute, intensity, angle, sky_color, sunlight_color in zip(range(minutes_in_day), intensities, angles, sky_colors, sunlight_colors)]

df = pd.DataFrame(results, columns=['Minute', 'Light Intensity', 'Light Direction', 'Sky Color RGB', 'Sunlight Color RGB'])
df.to_csv('Simulated_Daylight_Cycle.csv', index=False)
