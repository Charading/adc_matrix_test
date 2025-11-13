ANALOG_DRIVER_REQUIRED = yes
ANALOG_DRIVER = rp2040_adc
CUSTOM_MATRIX = lite


# SIMPLIFIED MUX ADC - Direct implementation like shego75_breadboard
# No hallscan module dependency - everything in one file
SRC += mux_adc_simple.c