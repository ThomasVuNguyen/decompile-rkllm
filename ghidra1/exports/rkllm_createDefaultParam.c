
void rkllm_createDefaultParam(undefined8 *param_1)

{
  long lVar1;
  
  param_1[5] = 0;
  *(undefined2 *)(param_1 + 7) = 1;
  *(undefined2 *)((long)param_1 + 0x5c) = 0x401;
  *param_1 = 0;
  param_1[2] = 0xffffffff00000028;
  param_1[1] = 0xffffffff00000200;
  param_1[4] = 0x3f8ccccd;
  param_1[3] = 0x3f4ccccd3f666666;
  param_1[6] = 0x3dcccccd40a00000;
  param_1[9] = "";
  param_1[8] = "";
  param_1[10] = "";
  *(undefined4 *)(param_1 + 0xb) = 0;
  lVar1 = sysconf(0x54);
  if (lVar1 == 8) {
    *(undefined4 *)(param_1 + 0xc) = 0xf0;
  }
  else {
    lVar1 = sysconf(0x54);
    if (lVar1 == 4) {
      *(undefined4 *)(param_1 + 0xc) = 0xf;
      return;
    }
  }
  return;
}

