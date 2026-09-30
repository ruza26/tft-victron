CampMonitor Alpha 9.4

Load totals are no longer inferred from solar output and net battery power.

Home power flow:
- Blue values: direct sum of charger output packets.
- Red values: SmartShunt discharge magnitude plus all SmartSolar load-output values.

Loads page:
- Camp Shunt appears as its own source.
- Each solar-controller load output remains a separate source.
- Footer is the direct sum of those displayed sources.

Chargers page:
- Each charging source remains separate.
- Footer now shows total charging amps and watts going toward the battery.

No solar-minus-battery inferred house-load calculation is used.
