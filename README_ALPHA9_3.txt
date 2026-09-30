CampMonitor Alpha 9.3
=====================

Power-flow current correction
-----------------------------

The SmartShunt current shown on the battery card is NET battery current:

  net battery current = solar charge current - battery-side house demand

It is not the total house current by itself.

The home-page house flow now uses:

  house power = total charger power - net battery power
  house current = house power / SmartShunt battery voltage

Deriving house current from the calculated house watts avoids small errors
caused by SmartSolar and SmartShunt current rounding and by measurements being
reported at slightly different points in the charging system.

The SmartSolar load-output value remains on the Charge page only.
