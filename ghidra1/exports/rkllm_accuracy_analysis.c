
int rkllm_accuracy_analysis
              (undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4)

{
  uint uVar1;
  char *__s;
  int iVar2;
  uint uVar3;
  void *pvVar4;
  size_t sVar5;
  ulong uVar6;
  long lVar7;
  int iVar8;
  ulong uVar9;
  long lVar10;
  ulong uVar11;
  ulong uVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  double dVar17;
  double dVar18;
  void *local_230;
  undefined8 uStack_228;
  undefined8 local_220;
  void *local_210;
  undefined8 uStack_208;
  undefined8 local_200;
  void *local_1f0;
  undefined8 uStack_1e8;
  undefined8 local_1e0;
  char acStack_1d0 [64];
  char local_190 [128];
  undefined8 local_110;
  undefined8 uStack_108;
  undefined8 local_100;
  undefined8 uStack_f8;
  undefined8 local_f0;
  undefined8 uStack_e8;
  undefined8 local_e0;
  undefined8 uStack_d8;
  undefined8 local_d0;
  undefined8 local_c8;
  undefined4 local_c0;
  
  pvVar4 = operator_new(0x248);
  memset(pvVar4,0,0x248);
                    /* try { // try from 001b9b58 to 001b9b5b has its CatchHandler @ 001ba0f0 */
  FUN_001ba3c0(pvVar4);
  FUN_001b9540("rkllm_accuracy_dump");
  iVar2 = access("rkllm_accuracy_dump",4);
  if (iVar2 != 0) {
    iVar2 = mkdir("rkllm_accuracy_dump",0x1ed);
    if (iVar2 == -1) {
      iVar2 = printf("create dir %s fail\n","rkllm_accuracy_dump");
    }
    else {
      iVar2 = printf("create dir %s\n","rkllm_accuracy_dump");
    }
  }
  rkllm_createDefaultParam(&local_d0,iVar2);
  DAT_007fbc54 = 0x65;
  local_c8 = 0x100000200;
  local_c0 = 1;
  local_d0 = param_3;
  iVar2 = FUN_001bbb20(pvVar4,&local_d0,0);
  if (iVar2 == 0) {
    iVar2 = FUN_001be860(pvVar4,param_1,param_2,0);
    if (iVar2 != 0) goto LAB_001b9ff0;
    iVar2 = FUN_001be624(pvVar4);
    if (iVar2 == 0) {
      DAT_007fbc54 = 0x66;
      local_d0 = param_4;
      iVar2 = FUN_001bbb20(pvVar4,&local_d0,0);
      if (iVar2 == 0) {
        iVar2 = FUN_001be860(pvVar4,param_1,param_2,0);
        if (iVar2 != 0) {
LAB_001b9ff0:
          FUN_001bb5a0(pvVar4);
          operator_delete(pvVar4);
          return iVar2;
        }
        iVar2 = FUN_001be624(pvVar4);
        if (iVar2 == 0) {
          DAT_007fbc54 = 0x67;
          local_d0 = param_4;
          iVar2 = FUN_001bbb20(pvVar4,&local_d0,0);
          if (iVar2 == 0) {
            iVar2 = FUN_001be860(pvVar4,param_1,param_2,0);
            if ((iVar2 == 0) && (iVar2 = FUN_001be624(pvVar4), iVar2 == 0)) {
              FUN_001bb5a0(pvVar4);
              operator_delete(pvVar4);
              puts("Analysis:");
              puts(
                  "    ID  name                               Type                 entire                                    single"
                  );
              puts(
                  "                                                             cos       euc                             cos       euc"
                  );
              puts(
                  "--------------------------------------------------------------------------------------------------------------------------"
                  );
              pvVar4 = calloc(0x20000,1);
              uVar3 = FUN_001b9630("rkllm_accuracy_dump/rkllm_dump_quant_entire",pvVar4,0x800);
              if (0 < (int)uVar3) {
                uVar11 = 0;
                lVar10 = 0;
LAB_001b9d40:
                do {
                  uVar12 = 0;
                  iVar2 = 0;
                  local_190[0] = '\0';
                  local_190[1] = '\0';
                  local_190[2] = '\0';
                  local_190[3] = '\0';
                  local_190[4] = '\0';
                  local_190[5] = '\0';
                  local_190[6] = '\0';
                  local_190[7] = '\0';
                  local_190[8] = '\0';
                  local_190[9] = '\0';
                  local_190[10] = '\0';
                  local_190[0xb] = '\0';
                  local_190[0xc] = '\0';
                  local_190[0xd] = '\0';
                  local_190[0xe] = '\0';
                  local_190[0xf] = '\0';
                  local_190[0x40] = '\0';
                  local_190[0x41] = '\0';
                  local_190[0x42] = '\0';
                  local_190[0x43] = '\0';
                  local_190[0x44] = '\0';
                  local_190[0x45] = '\0';
                  local_190[0x46] = '\0';
                  local_190[0x47] = '\0';
                  local_190[0x48] = '\0';
                  local_190[0x49] = '\0';
                  local_190[0x4a] = '\0';
                  local_190[0x4b] = '\0';
                  local_190[0x4c] = '\0';
                  local_190[0x4d] = '\0';
                  local_190[0x4e] = '\0';
                  local_190[0x4f] = '\0';
                  __s = (char *)((long)pvVar4 + lVar10);
                  local_190[0x10] = '\0';
                  local_190[0x11] = '\0';
                  local_190[0x12] = '\0';
                  local_190[0x13] = '\0';
                  local_190[0x14] = '\0';
                  local_190[0x15] = '\0';
                  local_190[0x16] = '\0';
                  local_190[0x17] = '\0';
                  local_190[0x18] = '\0';
                  local_190[0x19] = '\0';
                  local_190[0x1a] = '\0';
                  local_190[0x1b] = '\0';
                  local_190[0x1c] = '\0';
                  local_190[0x1d] = '\0';
                  local_190[0x1e] = '\0';
                  local_190[0x1f] = '\0';
                  local_190[0x50] = '\0';
                  local_190[0x51] = '\0';
                  local_190[0x52] = '\0';
                  local_190[0x53] = '\0';
                  local_190[0x54] = '\0';
                  local_190[0x55] = '\0';
                  local_190[0x56] = '\0';
                  local_190[0x57] = '\0';
                  local_190[0x58] = '\0';
                  local_190[0x59] = '\0';
                  local_190[0x5a] = '\0';
                  local_190[0x5b] = '\0';
                  local_190[0x5c] = '\0';
                  local_190[0x5d] = '\0';
                  local_190[0x5e] = '\0';
                  local_190[0x5f] = '\0';
                  local_190[0x20] = '\0';
                  local_190[0x21] = '\0';
                  local_190[0x22] = '\0';
                  local_190[0x23] = '\0';
                  local_190[0x24] = '\0';
                  local_190[0x25] = '\0';
                  local_190[0x26] = '\0';
                  local_190[0x27] = '\0';
                  local_190[0x28] = '\0';
                  local_190[0x29] = '\0';
                  local_190[0x2a] = '\0';
                  local_190[0x2b] = '\0';
                  local_190[0x2c] = '\0';
                  local_190[0x2d] = '\0';
                  local_190[0x2e] = '\0';
                  local_190[0x2f] = '\0';
                  local_190[0x60] = '\0';
                  local_190[0x61] = '\0';
                  local_190[0x62] = '\0';
                  local_190[99] = '\0';
                  local_190[100] = '\0';
                  local_190[0x65] = '\0';
                  local_190[0x66] = '\0';
                  local_190[0x67] = '\0';
                  local_190[0x68] = '\0';
                  local_190[0x69] = '\0';
                  local_190[0x6a] = '\0';
                  local_190[0x6b] = '\0';
                  local_190[0x6c] = '\0';
                  local_190[0x6d] = '\0';
                  local_190[0x6e] = '\0';
                  local_190[0x6f] = '\0';
                  local_190[0x30] = '\0';
                  local_190[0x31] = '\0';
                  local_190[0x32] = '\0';
                  local_190[0x33] = '\0';
                  local_190[0x34] = '\0';
                  local_190[0x35] = '\0';
                  local_190[0x36] = '\0';
                  local_190[0x37] = '\0';
                  local_190[0x38] = '\0';
                  local_190[0x39] = '\0';
                  local_190[0x3a] = '\0';
                  local_190[0x3b] = '\0';
                  local_190[0x3c] = '\0';
                  local_190[0x3d] = '\0';
                  local_190[0x3e] = '\0';
                  local_190[0x3f] = '\0';
                  local_190[0x70] = '\0';
                  local_190[0x71] = '\0';
                  local_190[0x72] = '\0';
                  local_190[0x73] = '\0';
                  local_190[0x74] = '\0';
                  local_190[0x75] = '\0';
                  local_190[0x76] = '\0';
                  local_190[0x77] = '\0';
                  local_190[0x78] = '\0';
                  local_190[0x79] = '\0';
                  local_190[0x7a] = '\0';
                  local_190[0x7b] = '\0';
                  local_190[0x7c] = '\0';
                  local_190[0x7d] = '\0';
                  local_190[0x7e] = '\0';
                  local_190[0x7f] = '\0';
                  sVar5 = strlen(__s);
                  uVar6 = 1;
                  uVar9 = 1;
                  if (1 < sVar5) {
                    do {
                      iVar8 = (int)uVar9;
                      iVar2 = (int)uVar12;
                      if (__s[uVar6] == '-') {
                        if (iVar2 != 0) {
                          lVar7 = uVar6 + 1;
                          goto LAB_001b9db4;
                        }
                        uVar12 = uVar6 & 0xffffffff;
                      }
                      iVar2 = (int)uVar12;
                      uVar6 = uVar6 + 1;
                      uVar9 = uVar6 & 0xffffffff;
                    } while (uVar6 < sVar5);
                  }
                  iVar8 = 0;
                  lVar7 = 1;
LAB_001b9db4:
                  strcpy(local_190,(char *)((long)pvVar4 + lVar10 + 1 + (long)iVar2));
                  strcpy(local_190 + 0x40,(char *)((long)pvVar4 + lVar10 + lVar7));
                  local_110 = 0;
                  uStack_108 = 0;
                  local_100 = 0;
                  uStack_f8 = 0;
                  local_f0 = 0;
                  uStack_e8 = 0;
                  local_e0 = 0;
                  uStack_d8 = 0;
                  local_190[(iVar8 - iVar2) + -1] = '\0';
                  strcpy((char *)&local_110,local_190);
                  sprintf(acStack_1d0,"%s/%s","rkllm_accuracy_dump/rkllm_dump_golden",__s);
                  local_230 = (void *)0x0;
                  uStack_228 = 0;
                  local_220 = 0;
                    /* try { // try from 001b9e44 to 001b9e47 has its CatchHandler @ 001ba09c */
                  FUN_001b9a50(acStack_1d0,&local_230);
                  sprintf(acStack_1d0,"%s/%s","rkllm_accuracy_dump/rkllm_dump_quant_entire",__s);
                  local_210 = (void *)0x0;
                  uStack_208 = 0;
                  local_200 = 0;
                    /* try { // try from 001b9e78 to 001b9e9f has its CatchHandler @ 001ba0e8 */
                  FUN_001b9a50(acStack_1d0,&local_210);
                  fVar13 = (float)FUN_001b9740(__s,&local_230,&local_210);
                  fVar14 = (float)FUN_001b9870(__s,&local_230,&local_210);
                  if (((int)local_110 == 0x646461) ||
                     ((((int)local_110 == 0x6d726f6e && (local_110._4_1_ == '\0')) ||
                      (local_110 == 0x6d726f6e736d72)))) {
                    dVar18 = 0.0;
                    dVar17 = 1.0;
                  }
                  else {
                    sprintf(acStack_1d0,"%s/%s","rkllm_accuracy_dump/rkllm_dump_quant_single",__s);
                    local_1f0 = (void *)0x0;
                    uStack_1e8 = 0;
                    local_1e0 = 0;
                    /* try { // try from 001b9f14 to 001b9f3b has its CatchHandler @ 001ba0b8 */
                    FUN_001b9a50(acStack_1d0,&local_1f0);
                    fVar15 = (float)FUN_001b9740(__s,&local_230,&local_1f0);
                    fVar16 = (float)FUN_001b9870(__s,&local_230,&local_1f0);
                    if (local_1f0 != (void *)0x0) {
                      operator_delete(local_1f0);
                    }
                    dVar17 = (double)fVar16;
                    dVar18 = (double)fVar15;
                  }
                    /* try { // try from 001b9f74 to 001b9f77 has its CatchHandler @ 001ba0e8 */
                  printf("%6d  %-34s%-18s%.5f | %-32.1f%.5f | %.1f\n",(double)fVar14,(double)fVar13,
                         dVar17,dVar18,uVar11,local_190 + 0x40,&local_110);
                  if (local_210 != (void *)0x0) {
                    operator_delete(local_210);
                  }
                  uVar1 = (int)uVar11 + 1;
                  uVar11 = (ulong)uVar1;
                  if (local_230 != (void *)0x0) {
                    operator_delete(local_230);
                    lVar10 = lVar10 + 0x40;
                    if (uVar3 == uVar1) break;
                    goto LAB_001b9d40;
                  }
                  lVar10 = lVar10 + 0x40;
                } while (uVar3 != uVar1);
              }
              putchar(10);
              free(pvVar4);
              return 0;
            }
            goto LAB_001b9ff0;
          }
        }
      }
    }
  }
  FUN_001bb5a0(pvVar4);
  operator_delete(pvVar4);
  return iVar2;
}

