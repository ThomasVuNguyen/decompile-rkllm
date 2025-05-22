
int rkllm_init(undefined8 *param_1,undefined8 param_2,undefined8 param_3)

{
  int iVar1;
  void *__s;
  
  __s = operator_new(0x248);
  memset(__s,0,0x248);
                    /* try { // try from 001b9348 to 001b934b has its CatchHandler @ 001b93a0 */
  FUN_001ba3c0(__s);
  iVar1 = FUN_001bbb20(__s,param_2,param_3);
  if (iVar1 == 0) {
    *param_1 = __s;
    return 0;
  }
  FUN_001bb5a0(__s);
  operator_delete(__s);
  return iVar1;
}

