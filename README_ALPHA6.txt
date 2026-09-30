CampMonitor V0.8 Alpha 5

This build preserves the V0.5 visual baseline and adds a scalable functional UI:
- Home
- Chargers
- Loads
- Battery
- Status

The device registry is provisioned for up to 8 devices. Only provisioned devices
with matching capabilities are displayed. Up to 3 cards appear at once; tap the
Chargers or Loads content area to advance through additional pages.

Important current limitation:
The existing live decoder still decodes SmartShunt Instant Readout packets only.
The Chargers and Loads architecture/UI are ready, and demo mode demonstrates the
layout, but live SmartSolar/Orion charger and load-output decoding is not yet
implemented in this build.

Alpha 6 TFT refresh patch
---------------------------
- Dashboard values update in small regions instead of repainting whole cards.
- Chargers, Loads and Status redraw only when their displayed data changes.
- UI remains capped at 250 ms while BLE processing continues at full speed.
- Full-screen redraws are reserved for page changes and mode transitions.
- TFT transactions are grouped with startWrite/endWrite to reduce tearing.


Alpha 6 display engine:
Battery, Chargers, and Loads now use incremental dirty-region updates to minimise TFT flicker.
