
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

char * rkllm_print_memorys(void)

{
  long lVar1;
  uint uVar2;
  FILE *__stream;
  char *pcVar3;
  long local_108;
  char acStack_100 [256];
  
  __stream = fopen("/proc/self/status","r");
  if (__stream == (FILE *)0x0) {
    pcVar3 = "Failed to open /proc/self/status";
    perror("Failed to open /proc/self/status");
    return pcVar3;
  }
  local_108 = -1;
  do {
    pcVar3 = fgets(acStack_100,0x100,__stream);
    if (pcVar3 == (char *)0x0) {
      fclose(__stream);
      lVar1 = local_108;
      goto joined_r0x0023b340;
    }
    pcVar3 = strstr(acStack_100,"VmHWM:");
  } while (pcVar3 == (char *)0x0);
  __isoc99_sscanf(acStack_100,"%*s %ld",&local_108);
  fclose(__stream);
  lVar1 = local_108;
joined_r0x0023b340:
  local_108 = lVar1;
  if (lVar1 == -1) {
    fwrite("I rkllm:  Memory Usage not found in /proc/self/status",1,0x35,_stdout);
  }
  else {
    fprintf(_stdout,"I rkllm:  %-12s","Memory Usage (GB)");
    fputc(10,_stdout);
    fflush(_stdout);
    fprintf(_stdout,"I rkllm:  %-12.2f",(double)lVar1 / 1048576.0);
  }
  fputc(10,_stdout);
  fflush(_stdout);
  fwrite("I rkllm: --------------------------------------------------------------------------------------"
         ,1,0x5f,_stdout);
  fputc(10,_stdout);
  uVar2 = fflush(_stdout);
  return (char *)(ulong)uVar2;
}

