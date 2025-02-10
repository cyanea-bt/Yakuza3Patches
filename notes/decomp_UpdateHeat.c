
void UpdateHeat_14043afc0
               (char **param_1,undefined8 param_2,undefined8 param_3,undefined (*param_4) [16])

{
  undefined4 uVar1;
  byte bVar2;
  char *pcVar3;
  undefined auVar4 [16];
  uint uVar5;
  int iVar6;
  int iVar7;
  undefined8 uVar8;
  ulonglong uVar9;
  ushort uVar10;
  ushort uVar11;
  short sVar12;
  short sVar13;
  undefined uVar14;
  undefined uVar15;
  bool bVar16;
  undefined8 extraout_XMM0_Qa;
  undefined8 extraout_XMM0_Qa_00;
  undefined8 extraout_XMM0_Qa_01;
  undefined8 extraout_XMM0_Qa_02;
  undefined8 extraout_XMM0_Qa_03;
  undefined8 extraout_XMM0_Qa_04;
  undefined8 extraout_XMM0_Qa_05;
  undefined8 extraout_XMM0_Qa_06;
  undefined auVar17 [16];
  undefined auVar20 [32];
  undefined extraout_var_00 [24];
  undefined extraout_var_01 [24];
  undefined extraout_var_02 [24];
  undefined extraout_var_03 [24];
  undefined extraout_var_04 [24];
  undefined auVar26 [32];
  undefined auVar19 [16];
  undefined extraout_var_06 [24];
  undefined extraout_var_07 [24];
  undefined auVar29 [32];
  undefined auVar30 [32];
  undefined auVar31 [16];
  undefined auVar32 [16];
  undefined auVar33 [16];
  undefined in_YMM8 [32];
  undefined auVar34 [16];
  undefined auVar35 [16];
  undefined auVar36 [16];
  undefined auVar37 [16];
  undefined in_YMM13 [32];
  undefined auVar38 [16];
  undefined auVar39 [16];
  undefined in_YMM14 [32];
  undefined auVar21 [32];
  undefined auVar22 [32];
  undefined auVar23 [32];
  undefined auVar24 [32];
  undefined auVar25 [32];
  undefined auVar27 [32];
  undefined auVar28 [32];
  undefined auVar18 [16];
  undefined extraout_var [24];
  undefined extraout_var_05 [24];
  undefined extraout_var_08 [24];
  undefined extraout_var_09 [24];
  
  auVar33 = in_YMM8._0_16_;
  auVar38 = in_YMM14._0_16_;
  auVar36 = in_YMM13._0_16_;
                    /* Checks if player is in combat (0 = not in combat, 1 = in combat) */
  uVar5 = IsPlayerInCombat_14076ab80();
  if (uVar5 == 0) {
    return;
  }
                    /* Checks if combat is active (0 = active, 1 = inactive)
                       Will be inactive e.g. during Heat moves/cutscenes. */
  uVar8 = IsCombatInactive_14047a330();
                    /* (param_1 + 0x27b) & 0x10000) is != 0 while combat is paused by a tutorial
                       popup message */
                    /* (*param_1 + 0x268)) = IsActorDead(param_1)
                       Checks if the player is dead (0 = alive, 1 = dead) */
                    /* (param_1[0x27a] + 0x354) & 0x4000000) is == 0 while combat is ongoing and ==
                       1 during combat transitions (combat intro/fade to black when the player dies)
                        */
                    /* (*(int *)(DAT_14122cde8 + 8) points to a variable that is == 0 while combat
                       is ongoing and == 1 when combat is finished (i.e. either the player or all
                       enemies are knocked out) */
  if (((((*(uint *)(param_1 + 0x27b) & 0x10000) != 0) ||
       (iVar6 = (**(code **)(*param_1 + 0x268))(param_1), iVar6 != 0)) ||
      ((*(uint *)(param_1[0x27a] + 0x354) & 0x4000000) != 0)) || (*(int *)(DAT_14122cde8 + 8) != 0))
  goto LAB_14043b71d;
  iVar6 = (int)uVar8;
  if (iVar6 == 0) {
    auVar20._0_8_ = (**(code **)(*param_1 + 0x338))(param_1);
    auVar20._8_24_ = extraout_var;
    auVar34 = vmovss_avx(0x3f800000);
    auVar32 = vmovss_avx(*(undefined4 *)((longlong)param_1 + 0x14c4));
    uVar11 = *(ushort *)(param_1 + 0x299);
    sVar13 = *(short *)((longlong)param_1 + 0x14ca);
    sVar12 = *(short *)((longlong)param_1 + 0x14cc);
    auVar17 = auVar20._0_16_;
    auVar37 = auVar34;
    iVar7 = (**(code **)(*param_1 + 0x290))(param_1);
    auVar21._8_24_ = extraout_var_00;
    auVar21._0_8_ = extraout_XMM0_Qa;
    auVar19 = auVar21._0_16_;
    if (iVar7 != 0) {
      auVar34 = vmovss_avx(0x40000000);
    }
    pcVar3 = param_1[0x27e];
    auVar31 = vmovss_avx(0x40400000);
    if (((ulonglong)pcVar3 >> 0x37 & 1) != 0) {
      auVar34 = vmulss_avx(auVar34,auVar31);
    }
    uVar15 = (*(uint *)((longlong)param_1 + 0x1404) & 4) == 0;
    if ((!(bool)uVar15) || (uVar15 = ((ulonglong)pcVar3 >> 0x25 & 1) == 0, !(bool)uVar15)) {
      auVar17 = vxorps_avx(auVar33,auVar33);
      auVar33 = auVar17;
      goto LAB_14043b515;
    }
    uVar15 = (*(uint *)((longlong)param_1 + 0x1404) & 8) == 0;
    if (((bool)uVar15) && (uVar15 = ((ulonglong)pcVar3 >> 0x28 & 1) == 0, (bool)uVar15)) {
      vmovss_avx(*(undefined4 *)((longlong)param_1 + 0x1cc4));
      auVar39 = vmovss_avx(0x41a00000);
      auVar33 = vxorps_avx(auVar33,auVar33);
      auVar19 = vcomiss_avx(auVar33);
      auVar35 = vmovss_avx(0x3f8ccccd);
      uVar8 = auVar19._0_8_;
      if ((bool)uVar15) {
        if ((*(short *)((longlong)param_1 + 0x1abe) != 0) &&
           ((*(byte *)((longlong)param_1 + 0x1a9c) & 0x10) == 0)) {
          auVar32 = auVar33;
          iVar7 = (**(code **)(*param_1 + 0x288))(param_1);
          auVar22._8_24_ = extraout_var_01;
          auVar22._0_8_ = extraout_XMM0_Qa_01;
          auVar18 = auVar22._0_16_;
          auVar19 = auVar37;
          if (iVar7 != 0) {
            auVar19 = auVar35;
          }
          auVar31 = vxorps_avx(auVar31,auVar31);
          auVar31 = vcvtsi2ss_avx(auVar31,(uint)*(ushort *)((longlong)param_1 + 0x1abe));
          FUN_14043a600((longlong)param_1);
          auVar4 = vmulss_avx(auVar31,auVar39);
          auVar31 = vmovss_avx(0x40400000);
          auVar18 = vmulss_avx(auVar18,auVar4);
          uVar8 = auVar18._0_8_;
          auVar18 = vmulss_avx(auVar18,auVar34);
          auVar19 = vmulss_avx(auVar18,auVar19);
          goto LAB_14043b20e;
        }
      }
      else {
        auVar32 = auVar33;
        iVar7 = (**(code **)(*param_1 + 0x288))(param_1);
        auVar19 = auVar37;
        if (iVar7 != 0) {
          auVar19 = auVar35;
        }
        auVar18 = vmulss_avx(auVar39,ZEXT416(*(uint *)((longlong)param_1 + 0x1cc4)));
        auVar18 = vmulss_avx(auVar18,auVar34);
        auVar19 = vmulss_avx(auVar18,auVar19);
        uVar8 = extraout_XMM0_Qa_00;
LAB_14043b20e:
        uVar11 = 0;
        auVar17 = vaddss_avx(auVar17,auVar19);
      }
      iVar7 = (**(code **)(*param_1 + 0x750))(uVar8,0xdd);
      if ((iVar7 != 0) && (*(short *)(param_1 + 0x358) != 0)) {
        uVar11 = 0;
        auVar32 = auVar33;
        iVar7 = (**(code **)(*param_1 + 0x288))(param_1);
        auVar23._8_24_ = extraout_var_02;
        auVar23._0_8_ = extraout_XMM0_Qa_02;
        if (iVar7 == 0) {
          auVar35 = auVar37;
        }
        auVar19 = vxorps_avx(auVar23._0_16_,auVar23._0_16_);
        auVar19 = vcvtsi2ss_avx(auVar19,(uint)*(ushort *)(param_1 + 0x358));
        auVar19 = vmulss_avx(auVar19,auVar39);
        auVar34 = vmulss_avx(auVar19,auVar34);
        auVar34 = vmulss_avx(auVar34,ZEXT416(0x3fc00000));
        auVar34 = vmulss_avx(auVar34,auVar35);
        auVar17 = vaddss_avx(auVar17,auVar34);
      }
      iVar7 = (**(code **)(*param_1 + 0x750))(param_1);
      auVar24._8_24_ = extraout_var_03;
      auVar24._0_8_ = extraout_XMM0_Qa_03;
      auVar34 = vmovss_avx(0x41200000);
      if ((iVar7 != 0) && (*(int *)(param_1[0x68] + 0xd0) == 0x28f)) {
        uVar11 = 0;
        auVar17 = vaddss_avx(auVar17,auVar34);
        auVar32 = auVar33;
      }
      uVar14 = 0;
      uVar15 = (*(uint *)(param_1[0x27a] + 0x354) & 0x4000000) == 0;
      auVar19 = auVar24._0_16_;
      uVar10 = uVar11;
      if ((bool)uVar15) {
        uVar14 = 0;
        uVar15 = *(int *)(DAT_14122cde8 + 8) == 0;
        if ((bool)uVar15) {
          if (sVar13 != 0) {
            sVar13 = sVar13 + -1;
          }
          if (sVar12 != 0) {
            sVar12 = sVar12 + -1;
            auVar17 = vaddss_avx(auVar17,auVar34);
          }
          if ((*(ushort *)((longlong)param_1 + 0x1abc) == 0) || (sVar13 != 0)) {
            bVar16 = false;
            auVar26._0_8_ = (**(code **)(*param_1 + 0x368))(param_1);
            auVar26._8_24_ = extraout_var_05;
            vmovss_avx(0x41980000);
            auVar19 = auVar26._0_16_;
            auVar39 = vcomiss_avx(auVar19);
            if (bVar16) {
              uVar14 = 0;
              uVar15 = (*(uint *)((longlong)param_1 + 0x13e4) & 0x10000000) == 0;
              if ((bool)uVar15) {
                auVar17 = vcomiss_avx(auVar33);
                if (!(bool)uVar15) {
                  uVar14 = 0;
                  uVar15 = sVar12 == 0;
                  if ((bool)uVar15) {
                    uVar10 = uVar11 + 1;
                    if (0x72 < uVar11) {
                      uVar10 = uVar11;
                    }
                    uVar14 = uVar10 < 0x73;
                    uVar15 = uVar10 == 0x73;
                    if ((bool)uVar15) {
                      uVar10 = 0x73;
                      uVar14 = 0;
                      uVar15 = sVar13 == 0;
                      if ((bool)uVar15) {
                        iVar7 = *(int *)((longlong)param_1 + 0x1a3c);
                        auVar39 = vmovss_avx(0x3f000000);
                        auVar32 = vaddss_avx(auVar32,auVar37);
                        auVar32 = vminss_avx(auVar31,auVar32);
                        auVar31 = auVar32;
                        if (iVar7 == 4) {
                          if ((*(char *)((longlong)param_1 + 0x1a49) == '\x03') &&
                             (*(byte *)(param_1 + 0x349) < 0x3d)) {
LAB_14043b489:
                            auVar31 = vmulss_avx(auVar32,auVar34);
                          }
                        }
                        else if (iVar7 == 9) {
                          auVar34 = vmulss_avx(auVar32,auVar39);
                          auVar31 = vmaxss_avx(auVar37,auVar34);
                        }
                        else if (iVar7 == 10) goto LAB_14043b489;
                        iVar7 = (**(code **)(*param_1 + 0x290))(param_1);
                        auVar28._8_24_ = extraout_var_07;
                        auVar28._0_8_ = extraout_XMM0_Qa_06;
                        auVar19 = auVar28._0_16_;
                        if (iVar7 != 0) {
                          auVar31 = vmulss_avx(auVar31,auVar39);
                        }
                        uVar14 = false;
                        uVar15 = DAT_1411c6524 == 0;
                        if ((bool)uVar15) {
                          auVar19 = vmulss_avx(auVar32,auVar39);
                          auVar32 = vmaxss_avx(auVar37,auVar19);
LAB_14043b4e3:
                          auVar17 = vsubss_avx(auVar17,auVar31);
                          uVar10 = 0x73;
                        }
                        else {
                          uVar14 = DAT_1411c6524 < 2;
                          uVar15 = DAT_1411c6524 == 2;
                          if ((bool)uVar15) {
                            auVar32 = vmulss_avx(auVar32,ZEXT416(0x3fc00000));
                            auVar17 = vsubss_avx(auVar17,auVar31);
                            uVar10 = 0x73;
                          }
                          else {
                            uVar14 = DAT_1411c6524 == 2;
                            uVar15 = DAT_1411c6524 == 3;
                            if (!(bool)uVar15) goto LAB_14043b4e3;
                            auVar32 = vmulss_avx(auVar32,ZEXT416(0x40000000));
                            auVar17 = vsubss_avx(auVar17,auVar31);
                            uVar10 = 0x73;
                          }
                        }
                      }
                    }
                  }
                }
                goto LAB_14043b51d;
              }
            }
            else {
              uVar15 = ((ulonglong)param_1[0x27e] >> 0x29 & 1) == 0;
              if ((bool)uVar15) {
                iVar7 = (**(code **)(*param_1 + 0x750))(param_1,auVar39._0_8_);
                auVar27._8_24_ = extraout_var_06;
                auVar27._0_8_ = extraout_XMM0_Qa_05;
                auVar19 = auVar27._0_16_;
                uVar14 = 0;
                uVar15 = iVar7 == 0;
                if ((bool)uVar15) goto LAB_14043b51d;
                auVar17 = vaddss_avx(auVar17,ZEXT416(0x40800000));
              }
              else {
                auVar17 = vaddss_avx(auVar17,ZEXT416(0x41000000));
              }
            }
LAB_14043b515:
            uVar14 = 0;
            auVar32 = auVar33;
            uVar10 = 0;
          }
          else {
            auVar35 = vmovss_avx(0x3f000000);
            auVar34 = vxorps_avx(auVar31,auVar31);
            auVar19 = vcvtsi2ss_avx(auVar34,(uint)*(ushort *)((longlong)param_1 + 0x1abc));
            auVar34 = auVar33;
            if (((ulonglong)param_1[0x27e] >> 0x24 & 1) != 0) {
              auVar34 = auVar35;
            }
            uVar9 = (ulonglong)param_1[0x27e] >> 0x26;
            bVar16 = (uVar9 & 1) != 0;
            if (bVar16) {
              auVar34 = vaddss_avx(auVar34,ZEXT416(0x3e800000));
            }
            uVar8 = auVar34._0_8_;
            vucomiss_avx(auVar34,auVar33);
            if (((uVar9 & 1) == 0) || (bVar16)) {
              auVar34 = vsubss_avx(auVar37,auVar34);
              uVar8 = auVar34._0_8_;
              auVar19 = vmulss_avx(auVar19,auVar34);
            }
            iVar7 = (**(code **)(*param_1 + 0x290))(uVar8);
            auVar25._8_24_ = extraout_var_04;
            auVar25._0_8_ = extraout_XMM0_Qa_04;
            uVar14 = 0;
            uVar15 = iVar7 == 0;
            if (!(bool)uVar15) {
              vmulss_avx(auVar19,auVar35);
            }
            auVar34 = vcomiss_avx(auVar33);
            auVar19 = auVar25._0_16_;
            if (!(bool)uVar15) {
              auVar19 = vmulss_avx(auVar34,auVar39);
              auVar17 = vsubss_avx(auVar17,auVar19);
            }
          }
        }
      }
    }
    else {
      uVar14 = 0;
      auVar33 = vxorps_avx(auVar33,auVar33);
      uVar10 = 0;
      auVar32 = auVar33;
      auVar29._0_8_ = (**(code **)(*param_1 + 0x340))(param_1);
      auVar29._8_24_ = extraout_var_08;
      auVar17 = auVar29._0_16_;
      auVar19 = auVar17;
    }
LAB_14043b51d:
    auVar34 = vxorps_avx(auVar19,auVar19);
    auVar19 = vmovss_avx(auVar34,auVar17);
    auVar34 = vmaxss_avx(auVar19,auVar33);
    auVar30._0_8_ =
         (**(code **)(*param_1 + 0x340))
                   (param_1,auVar17._0_8_,auVar33._0_8_,auVar19._0_8_,auVar38,auVar36);
    auVar30._8_24_ = extraout_var_09;
    auVar36 = vxorps_avx(auVar17,auVar17);
    auVar36 = vmovss_avx(auVar36,auVar30._0_16_);
    vmovss_avx(0x3a83126f);
    auVar36 = vminss_avx(auVar34,auVar36);
    vcomiss_avx(auVar36);
    if ((!(bool)uVar14 && !(bool)uVar15) &&
       (auVar32 = vcomiss_avx(auVar33), !(bool)uVar14 && !(bool)uVar15)) {
      uVar10 = 0;
      auVar32 = auVar33;
    }
    uVar1 = vmovss_avx(auVar36);
    *(undefined4 *)(param_1[0x2a2] + 8) = uVar1;
    uVar1 = vmovss_avx(auVar32);
    *(undefined4 *)((longlong)param_1 + 0x14c4) = uVar1;
    *(short *)((longlong)param_1 + 0x14ca) = sVar13;
    *(short *)((longlong)param_1 + 0x14cc) = sVar12;
    *(ushort *)(param_1 + 0x299) = uVar10;
  }
  bVar2 = *(byte *)((longlong)param_1 + 0x13db);
  if (((*(uint *)(param_1[0x27a] + 0x354) & 0x4000000) == 0) &&
     (bVar16 = false, *(int *)(DAT_14122cde8 + 8) == 0)) {
    (**(code **)(*param_1 + 0x338))(param_1);
    vcomiss_avx(ZEXT416(0x45d48000));
    if (bVar16) {
      if ((bVar2 & 1) != 0) {
        *(uint *)(param_1 + 0x27b) = *(uint *)(param_1 + 0x27b) & 0xfeffffff;
        goto LAB_14043b654;
      }
    }
    else {
      if ((bVar2 & 1) != 0) goto LAB_14043b6a3;
      *(uint *)(param_1 + 0x27b) = *(uint *)(param_1 + 0x27b) | 0x1000000;
      (**(code **)(*param_1 + 0x670))(param_1);
    }
LAB_14043b658:
    if ((*(uint *)(param_1 + 0x27b) & 0x1000000) == 0) {
      if ((bVar2 & 1) != 0) goto LAB_14043b6a3;
    }
    else {
      FUN_14042bd60(param_1,0,(char *)0x1,param_4);
      if (iVar6 == 0) {
        FUN_14087a960(0x10009,(undefined4 *)0x0,0,-1);
        *(uint *)(param_1 + 0x27b) = *(uint *)(param_1 + 0x27b) & 0xffffffbf;
        goto LAB_14043b71d;
      }
    }
  }
  else {
LAB_14043b654:
    if ((bVar2 & 1) == 0) goto LAB_14043b658;
LAB_14043b6a3:
    if ((*(uint *)(param_1 + 0x27b) >> 0x18 & 1) == 0) {
      FUN_14042bfc0((longlong *)param_1,0,1);
      if (iVar6 == 0) {
        FUN_14087a960(0x2001a,(undefined4 *)0x0,0,-1);
        *(uint *)(param_1 + 0x27b) = *(uint *)(param_1 + 0x27b) & 0xffffffbf;
        goto LAB_14043b71d;
      }
    }
    else if (((bVar2 & 1) != 0) && ((*(uint *)(param_1 + 0x27b) & 0x40) != 0)) {
      FUN_14042bfc0((longlong *)param_1,0,1);
      FUN_14042bd60(param_1,0,(char *)0x1,param_4);
    }
  }
  *(uint *)(param_1 + 0x27b) = *(uint *)(param_1 + 0x27b) & 0xffffffbf;
LAB_14043b71d:
  *(undefined2 *)((longlong)param_1 + 0x1abc) = 0;
  return;
}

