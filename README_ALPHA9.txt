CampMonitor Freenove 4in Alpha 9
================================

Field-test provisioning update:

- Replaces the single "current device" key form with a stable list of detected
  Victron devices.
- Shows the BLE custom name when advertised (for example Camp Solar and Camp
  Shunt), plus MAC address and RSSI.
- Saves each 32-character Instant Readout key against the selected full MAC
  address.
- Adds per-device Replace Key and Remove Saved Key actions.
- Shows per-device Detected, Key Required, Key Stored, Key Rejected and Live
  Data states.
- Enables active BLE scanning to improve capture of custom device names.
- Migrates Alpha 8 keys that used the shortened legacy MAC storage key.
- Removes automatic demo readings from normal startup. The dashboard now waits
  for genuine Victron data.
- Expands the on-device Status page to show up to three detected devices.

Field test procedure
--------------------
1. Flash Alpha 9.
2. Open Status and start the setup portal if it did not start automatically.
3. Join the CampMonitor-XXXXXX Wi-Fi network and open 192.168.4.1.
4. Confirm Camp Solar and Camp Shunt appear as separate rows.
5. Enter each key only in its matching row.
6. Look for Live data. Key rejected after repeated packets indicates the key is
   assigned incorrectly or was copied incorrectly.
