
/* Returns whether combat is inactive (0 = active, 1 = inactive)
   Will be inactive e.g. during Heat moves/cutscenes. */

undefined8 IsCombatInactive_14047a330(void)

{
  int iVar1;
  
  if ((((DAT_141198218 != 0) && (*(longlong *)(DAT_141198218 + 0x538) != 0)) &&
      (iVar1 = *(int *)(*(longlong *)(DAT_141198218 + 0x538) + 0x1a8), iVar1 != 7)) && (iVar1 != 0))
  {
    return 1;
  }
  return 0;
}

