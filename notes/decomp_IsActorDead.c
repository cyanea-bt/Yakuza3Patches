
/* Returns 0 while the calling actor (param_1) is alive.
   Called by HeatUpdate() and probably others.
   Almost all of HeatUpdate()'s code is skipped if this returns != 0 */

uint IsActorDead_1403f59c0(longlong param_1)

{
  return *(uint *)(param_1 + 0x13d8) >> 0x1e & 1;
}

