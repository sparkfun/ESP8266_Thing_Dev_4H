

# 4H Incredible Wearables WiFi Update

The latest software for the 4H Incredible Wearables devices changes how the WiFi SSID is determined for each THING. This document outlines these changes and the modifications for the Incredible Wearables Facilitator Guide.

## Determining the WiFi SSID for a Thing
To ensure each THING has a unique WiFi SSID, the following happens at startup of a THING.
1. The THING is turned on.
2. The THING waits for a few, random seconds to prevent two THINGS from using the same SSID.
3. The THING scans all current WiFi networks, looking for other 4H THINGS.
  * THING SSID names follow the pattern of "IncredibleWearableN", where "N" is a number.
  * If multiple THINGS are being used, each use different number for "N" in their SSID
4. Once the THING selects a SSID, it flashes is LED "N" times to indicate what SSID number it is.
  * This value is flashed twice at startup.

For a majority of cases, this SSID selection methodology will determine a unique SSID for each 4H THING. When more than one THING is started at the same time, there is a small chance that two THINGS will select the same SSID number. If this occurs, the solution is to restart one of the THINGS, causing the THING to select a new SSID number.

## Facilitator Guide Corrections
### Page 17
#### STEP 2
The 4H Incredible Wearables network name is now "IncredibleWearableN", where N is a number
#### IMPORTANT!
This section is no longer valid and should be ignored.

### Page 18
All parts of this page except the TROUBLESHOOTING section are no longer valid and should be ignored.
