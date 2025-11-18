ANALOG_DRIVER_REQUIRED = yes
ANALOG_DRIVER = rp2040_adc
CUSTOM_MATRIX = lite


# SIMPLIFIED MUX ADC - Direct implementation like shego75_breadboard
# No hallscan module dependency - everything in one file
SRC += hallscan/hallscan.c
SRC += hallscan/hallscan_reports.c

RAW_ENABLE = yes
VENDOR_DRIVER_ENABLE = yes
# Enable console and debug prints so uprintf() messages appear in QMK Toolbox

DEBUG = yes
# Ensure USB serial console is available for uprintf() debugging
CONSOLE_ENABLE = yes
# CONSOLE_ENABLE = yes (commented out as already enabled in keyboard.json)