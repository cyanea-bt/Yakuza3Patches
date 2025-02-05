
void Calls_UpdateHeat_140438b70
               (char **param_1,undefined8 param_2,undefined8 param_3,undefined (*param_4) [16])

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  undefined (*pauVar4) [16];
  bool bVar5;
  bool bVar6;
  undefined auVar7 [16];
  undefined auVar8 [16];
  undefined in_YMM6 [32];
  undefined auVar9 [16];
  undefined4 local_38 [8];
  undefined local_18 [16];
  
  auVar7 = in_YMM6._0_16_;
  FUN_14043ab10((longlong *)param_1);
  UpdateHeat_14043afc0(param_1,param_2,param_3,param_4);
  if (((*(uint *)(param_1[0x27a] + 0x354) & 0x4000000) == 0) && (*(int *)(DAT_14122cde8 + 8) == 0))
  {
    FUN_14043ac80((longlong *)param_1);
  }
  FUN_14043b740((longlong)param_1);
  FUN_14043b9e0((longlong *)param_1);
  FUN_14043c4a0((longlong *)param_1);
  iVar2 = (**(code **)(*param_1 + 0x508))(param_1);
  auVar9 = vmovss_avx(0x4724d300);
  bVar6 = iVar2 == 0;
  if (bVar6) {
    iVar2 = (**(code **)(*param_1 + 0x510))(param_1);
    bVar6 = iVar2 == 0;
    if (!bVar6) goto LAB_140438bf4;
LAB_140438c65:
    uVar3 = (**(code **)(*param_1 + 0x508))(param_1);
    bVar6 = uVar3 == 0;
    if (bVar6) {
      uVar3 = (**(code **)(*param_1 + 0x510))(param_1);
      bVar6 = uVar3 == 0;
      if (bVar6) goto LAB_140438cac;
    }
    auVar7 = vmovss_avx(*(undefined4 *)(param_1[0x2a2] + 0x78));
    vucomiss_avx(auVar7,auVar9);
    if ((((POPCOUNT(uVar3 & 0xff) & 1U) == 0) || (!bVar6)) || ((DAT_141812c68 & 0x7f) != 0))
    goto LAB_140438cac;
  }
  else {
LAB_140438bf4:
    bVar5 = false;
    auVar9 = vcomiss_avx(ZEXT416(*(uint *)(param_1[0x2a2] + 0x78)));
    if (bVar6) goto LAB_140438c65;
    auVar8 = *(undefined (*) [16])((longlong)param_1[0x68] + 0x900);
    local_18 = auVar7;
    pauVar4 = (undefined (*) [16])FUN_14078c8b0((longlong *)param_1[0x68],local_38);
    auVar7 = vsubps_avx(*pauVar4,auVar8);
    auVar7 = vdpps_avx(auVar7,auVar7,0x5f);
    auVar7 = vsqrtps_avx(auVar7);
    auVar7 = vaddss_avx(auVar7,ZEXT416(*(uint *)(param_1[0x2a2] + 0x78)));
    uVar1 = vmovss_avx(auVar7);
    *(undefined4 *)(param_1[0x2a2] + 0x78) = uVar1;
    vmovss_avx(*(undefined4 *)(param_1[0x2a2] + 0x78));
    vcomiss_avx(auVar9);
    if (bVar5) goto LAB_140438cac;
    *(undefined4 *)(param_1[0x2a2] + 0x78) = 0x4724d300;
  }
  FUN_140929bd0(0x2b);
LAB_140438cac:
  FUN_14041c520((longlong)param_1);
  return;
}

