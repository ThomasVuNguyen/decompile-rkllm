
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int rkllm_print_timings(void)

{
  int iVar1;
  double dVar2;
  double dVar3;
  undefined1 auStack_40 [16];
  undefined8 local_30;
  double local_28;
  double dStack_20;
  double local_18;
  uint local_c;
  uint uStack_8;
  
  FUN_0023af14(auStack_40);
  fwrite("I rkllm: --------------------------------------------------------------------------------------"
         ,1,0x5f,_stdout);
  fputc(10,_stdout);
  fflush(_stdout);
  fprintf(_stdout,"I rkllm:  %-12s  %-15s  %-8s  %-23s  %-23s","Stage","Total Time (ms)","Tokens",
          "Time per Token (ms)","Tokens per Second");
  fputc(10,_stdout);
  fflush(_stdout);
  fwrite("I rkllm: --------------------------------------------------------------------------------------"
         ,1,0x5f,_stdout);
  fputc(10,_stdout);
  fflush(_stdout);
  fprintf(_stdout,"I rkllm:  %-12s  %-15.2f  %-8s  %-23s  %-23s",local_30,&DAT_0061c4a0,
          &DAT_006195b0,&DAT_006195b0);
  fputc(10,_stdout);
  fflush(_stdout);
  if (local_c == 0) {
    dVar2 = 0.0;
    dVar3 = dVar2;
  }
  else {
    dVar2 = dStack_20 / (double)(int)local_c;
    dVar3 = (1000.0 / dStack_20) * (double)(int)local_c;
  }
  fprintf(_stdout,"I rkllm:  %-12s  %-15.2f  %-8d  %-23.2f  %-23.2f",dStack_20,dVar2,dVar3,"Prefill"
          ,(ulong)local_c);
  fputc(10,_stdout);
  fflush(_stdout);
  local_28 = local_28 + local_18;
  fprintf(_stdout,"I rkllm:  %-12s  %-15.2f  %-8d  %-23.2f  %-23.2f",local_28,
          local_28 / (double)(int)uStack_8,(1000.0 / local_28) * (double)(int)uStack_8,"Generate",
          (ulong)uStack_8);
  fputc(10,_stdout);
  fflush(_stdout);
  fwrite("I rkllm: --------------------------------------------------------------------------------------"
         ,1,0x5f,_stdout);
  fputc(10,_stdout);
  iVar1 = fflush(_stdout);
  return iVar1;
}

