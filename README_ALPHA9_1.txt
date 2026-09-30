CampMonitor Freenove 4in Alpha 9.1 - Field Diagnostics Fix

Changes from Alpha 9:
- Corrected the Victron Instant Readout record-type offset from byte 3 to byte 6.
  This specifically fixes SmartSolar packets being classified from an unknown header byte.
- Added portal diagnostics for record type, packet length, and encryption-key identifier match.
- Reduced the pause between BLE scan windows from 1500 ms to 250 ms for fresher readings.
- Kept display, touch, PWM brightness, and sleep/wake behavior unchanged.

Expected Camp Solar status sequence:
Detected -> Key stored -> Key identifier matched -> Live data

If the key identifier matches but decoding still fails, capture the portal diagnostics and serial output.
