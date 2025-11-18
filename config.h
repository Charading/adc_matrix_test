/* Keyboard-specific configuration for adc_matrix_test
 * Ensure the Raw HID interface uses the expected usage page/ID and report size
 * so host apps (like the Electron tool) can reliably find and talk to the
 * device.
 */

#pragma once

/* Match QMK Raw HID defaults; set explicitly to be clear. */
#define RAW_USAGE_PAGE 0xFF60
#define RAW_USAGE_ID   0x61

/* Raw HID report size (bytes). QMK's RAW_EPSIZE is 32 by default; define
 * explicitly so it's obvious and consistent with host code. */
#define RAW_EPSIZE 32

/* Optionally define the hallscan LED pin here (if this board has a transistor
 * on GP8). Uncomment and change if the keyboard wiring supports it:
 * #define HALLSCAN_LED_PIN GP8
 */

/* Enable the vendor bulk interface so a vendor-specific (libusb/WinUSB)
 * endpoint pair is available for custom host comms. This compiles in the
 * VENDOR_INTERFACE / VENDOR_IN/OUT endpoints in the global USB descriptors.
 * Use `VENDOR_BULK_ENABLE` to select the bulk endpoint option, or define
 * `VENDOR_HID_ENABLE` to enable a vendor-HID (interrupt/report) interface
 * instead. Defining the legacy `VENDOR_ENABLE` will still enable the bulk
 * interface (kept for compatibility).
 */

