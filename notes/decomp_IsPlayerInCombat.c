
/* Returns whether player is in combat (0 = not in combat, 1 = in combat) */

uint IsPlayerInCombat_14076ab80(void)

{
  uint uVar1;
  
  if (DAT_141228a50 != 0) {
    uVar1 = FUN_1407815f0(DAT_141228a50);
    return uVar1 >> 0x1e & 1;
  }
  return 1;
}

