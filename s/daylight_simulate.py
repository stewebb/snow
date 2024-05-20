import pandas as pd
import numpy as np

# Set up the time range
times = pd.date_range("00:00", "23:59", freq='T').strftime('%H:%M')
minute_count = np.arange(1440)

# Sunlight intensity and color calculations
intensity_R = np.where(((minute_count >= 360) & (minute_count <= 420)) | ((minute_count >= 1020) & (minute_count <= 1080)),
                       255 * (np.sin(np.pi * (minute_count % 60) / 60)),  # Sunrise/Sunset transition
                       np.where(((minute_count > 420) & (minute_count < 1020)), 255, 0))  # Daytime peak
#print(intensity_R)
for m in minute_count:
    #print(m)

    
    if ((m >= 360) & (m <= 420)) or ((m >= 1020) & (m <= 1080)):
        # Sunrise/Sunset transition
        intensity_R[m] = 255 * np.sin(np.pi * (m % 60) / 60)
    elif (m > 420) & (m < 1020):
        # Daytime peak
        intensity_R[m] = 255
    else:
        # Nighttime
        intensity_R[m] = 0
    


intensity_G = intensity_R
intensity_B = np.where(intensity_R == 255, 255, intensity_R * 0.5)

# Light source angle calculations
light_source_angle = np.where(((minute_count >= 360) & (minute_count <= 1080)),
                              (minute_count - 360) * (180 / (1080 - 360)),  # from 0 at sunrise to 180 at sunset
                              np.where(minute_count < 360, 0, 180))

# Night sky color considering light pollution
night_color_R, night_color_G, night_color_B = 32, 32, 64

# Transition values for dawn and dusk
transition_R_start = np.linspace(night_color_R, 255 * 0.75, 31)
transition_G_start = np.linspace(night_color_G, 255 * 0.85, 31)
transition_B_start = np.linspace(night_color_B, 255 * 1.5, 31)

transition_R_end = np.linspace(255 * 0.75, night_color_R, 31)
transition_G_end = np.linspace(255 * 0.85, night_color_G, 31)
transition_B_end = np.linspace(255 * 1.5, night_color_B, 31)

background_R = np.zeros(1440)
background_G = np.zeros(1440)
background_B = np.zeros(1440)

# Apply transitions
for idx, minute in enumerate(minute_count):
    if 330 <= minute <= 360:
        index = minute - 330
        background_R[idx] = transition_R_start[index]
        background_G[idx] = transition_G_start[index]
        background_B[idx] = transition_B_start[index]
    elif 1080 <= minute <= 1110:
        index = minute - 1080
        background_R[idx] = transition_R_end[index]
        background_G[idx] = transition_G_end[index]
        background_B[idx] = transition_B_end[index]
    elif minute < 330 or minute > 1110:
        background_R[idx] = night_color_R
        background_G[idx] = night_color_G
        background_B[idx] = night_color_B
    elif 360 < minute < 1080:
        background_R[idx] = 255 * 0.75
        background_G[idx] = 255 * 0.85
        background_B[idx] = 255 * 1.5

# Create DataFrame
df = pd.DataFrame({
    'Time': times,
    'Minute': minute_count,
    'LightIntensityR': intensity_R.astype(int),
    'LightIntensityG': intensity_G.astype(int),
    'LightIntensityB': intensity_B.astype(int),
    'lightAngle': light_source_angle.astype(int),
    'BackgroundColorR': background_R.astype(int),
    'BackgroundColorG': background_G.astype(int),
    'BackgroundColorB': background_B.astype(int)
})

# Save to CSV
file_path = 'Simulated_Sky_Light_Intensity_and_Color.csv'
df.to_csv(file_path, index=False)
