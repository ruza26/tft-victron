CampMonitor Alpha 9.2

Home power-flow changes:
- Solar-to-battery flow now shows both SmartSolar watts and charge amps.
- Battery-to-house flow now shows inferred total house watts and amps.
- Total house demand is calculated from SmartSolar output minus the SmartShunt net battery flow.
- SmartSolar load-output current is no longer used as the house total.
- SmartSolar load-output current is shown on the Charge page.
- Corrected Victron charge-stage labels (3=Bulk, 4=Absorption, 5=Float, etc.).

Notes:
The SmartShunt measures net current into/out of the battery, not the complete house load while solar is active. CampMonitor combines the shunt and charger readings to infer total DC demand.
