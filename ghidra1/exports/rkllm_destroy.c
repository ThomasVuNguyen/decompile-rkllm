
undefined8 rkllm_destroy(void *param_1)

{
  if (param_1 != (void *)0x0) {
    FUN_001be624();
    FUN_001bb5a0(param_1);
    operator_delete(param_1);
    return 0;
  }
  return 0xffffffff;
}

