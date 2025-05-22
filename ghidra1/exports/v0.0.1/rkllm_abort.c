
undefined8 rkllm_abort(long param_1)

{
  undefined8 uVar1;
  
  if (param_1 != 0) {
    uVar1 = FUN_001bb580();
    return uVar1;
  }
  return 0xffffffff;
}

