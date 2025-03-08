# pebble-darkroom

## Overview

Pebble Darkroom is a specialized timer application designed for film and print development in darkroom photography. The app provides separate timers for each stage of the development process (Develop, Stop, Fix, Wash), with a user-friendly interface designed for use in low-light darkroom environments.

## Features

- **Dual Timer System**: Run two independent timers simultaneously for managing multiple development processes
- **Film and Print Modes**: Preconfigured timers for both film and print development processes
- **Stage Tracking**: Visual indicators for development stages (Develop, Stop, Fix, Wash)
- **Customizable Timing**: Adjust duration for each development stage
- **Display Options**: Invert colors for better visibility in darkroom conditions
- **Haptic Feedback**: Distinctive vibration patterns for each timer
- **Pause/Resume**: Full control over timer progression
- **Manual Stage Advancement**: Timers don't automatically start the next stage, giving you full control
- **Screen Refresh**: Force screen refresh to address screen tearing

## Using the App

### Basic Controls

- **UP Button**:
  - **Double-Click**: Switch between Timer 1 and Timer 2
  - **Long Press**: Force screen refresh (helps with screen tearing)

- **SELECT Button (Middle)**:
  - **Press**: Open settings menu

- **DOWN Button**:
  - **Press**: Start/Pause/Resume the active timer
  - **Long Press**: Reset the active timer
  - **Double-Click**: Toggle between Film and Print modes

### Timer Operation

1. **Starting a Development Process**:
   - Select desired timer (Timer 1 or Timer 2) using UP double-click
   - Choose Film or Print mode using DOWN double-click
   - Press DOWN button to start the first stage (Develop)

2. **Between Stages**:
   - When a stage completes, the app will:
     - Vibrate with a pattern specific to the active timer
     - Timer 1: Single pulse
     - Timer 2: Double pulse
   - The app will automatically advance to the next stage but will wait for you to start it
   - After 2 seconds, a reminder vibration occurs with the same pattern
   - Press DOWN button to start the next stage when ready

3. **Pausing/Resuming**:
   - Press DOWN button once to pause a running timer
   - Press DOWN button again to resume the timer

### Timer Indicators

- The app displays which timer is active (Timer 1 or Timer 2)
- Mode indicator shows F (Film) or P (Print)
- Stage indicator shows the current development stage (Dev, Stop, Fix, Wash)
- Progress bar at bottom of screen shows current stage position
- Timer display shows remaining time in minutes:seconds format

## Settings Menu

Access the settings menu by pressing the SELECT button.

### Basic Settings

- **Vibration**: Toggle vibration alerts on/off
- **Backlight**: Toggle screen backlight on/off

### Display Settings

- **Invert Timer 1**: Toggle color inversion for Timer 1
- **Invert Timer 2**: Toggle color inversion for Timer 2
- **Invert Menu**: Toggle color inversion for settings menu

### Film Times

Customize seconds for each film development stage:
- Develop (default: 5:00)
- Stop (default: 1:00)
- Fix (default: 5:00)
- Wash (default: 5:00)

### Print Times

Customize seconds for each print development stage:
- Develop (default: 1:00)
- Stop (default: 0:30)
- Fix (default: 5:00)
- Wash (default: 5:00)

## Use Cases

### Developing Film and Prints Simultaneously

1. Set Timer 1 to Film mode
2. Set Timer 2 to Print mode
3. Start Timer 1 for film development
4. When needed, double-click UP to switch to Timer 2
5. Start Timer 2 for print development
6. Switch between timers as needed to monitor both processes

### Working in Dark Conditions

1. Enable Backlight setting if needed
2. Consider inverting display colors for better visibility
3. Use haptic feedback to track timer completion

## Building and Installing the App

### Prerequisites
- Docker
- Make
- Pebble smartphone app installed on your device
- [Rebble web services setup](https://help.rebble.io/setup) (optional)

### Building the App
1. Clone the repository:
```bash
git clone https://github.com/toozej/pebble-darkroom.git
cd pebble-darkroom
```

2. Build the app using Docker:
```bash
make build copy
```

### Installing on Your Pebble

#### Method 1: Using Pebble Phone App

1. Enable Developer Mode in the Pebble smartphone app
2. Ensure your phone and Pebble watch are connected
3. Run the following command:
```bash
cd app/pebble-darkroom/
pebble install --phone YOUR_PHONE_IP
```

#### Method 2: Sideloading

1. Build the app as described above
2. Locate the `.pbw` file in the `build` directory
3. Send this file to your phone
4. Open the file with the Pebble app to install

## Technical Details

### Persistent Storage

The app saves your settings between sessions, including:
- Vibration and backlight preferences
- Color inversion settings
- Custom timer durations for each stage

## Troubleshooting

### Screen Tearing

If you experience screen tearing (horizontal lines across the display):
1. Long-press the UP button to force a screen refresh
2. If the issue persists, try restarting your Pebble watch

### Timer Not Advancing

If the timer completes but doesn't seem to advance to the next stage:
1. Check that vibration is enabled in settings
2. Remember that you need to manually start each stage with the DOWN button
3. Verify that the watch hasn't entered power-saving mode

### Settings Not Saving

If your custom settings aren't persisting between app launches:
1. Make sure to fully exit the settings menu before closing the app
2. Check that your Pebble has sufficient storage space available
3. Try rebuilding and reinstalling the app
