CampMonitor 0.8.0-a7 - Victron device decoder expansion

Added Instant Readout parsing for:
- SmartSolar / BlueSolar MPPT (record 0x01): state, error, battery voltage/current, solar watts, today's yield, load-output current.
- SmartShunt / BMV battery monitors (record 0x02): existing validated decoder retained.
- Orion-Tr Smart DC/DC (record 0x04): state, error, input/output voltage and off reason. Instant Readout does not provide current in this record layout, so displayed power may be zero.
- Blue Smart AC charger (record 0x08): state, error, channel-1 voltage/current and AC input current.
- Smart BatteryProtect (record 0x09): state, input/output voltage and output on/off status. This protocol does not advertise load current.
- Orion XS (record 0x0F): state, error, input/output voltage/current and calculated output power.

Important weekend test notes:
1. Enable Instant Readout in VictronConnect for every device.
2. Obtain each device's own 32-character advertisement key.
3. CampMonitor will open its setup AP for each newly discovered MAC without a saved key.
4. Do not leave VictronConnect actively connected to a device while testing; connected devices stop advertising Instant Readout.
5. SmartSolar is the priority validation target. Compare battery voltage/current, solar watts, charge state and load-output current with VictronConnect.

This source was structurally audited but not compiled against the user's exact Arduino library versions in this environment.
