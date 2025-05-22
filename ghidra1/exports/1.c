
void entry(long param_1)

{
  undefined8 *puVar1;
  
  puVar1 = (undefined8 *)__cxa_allocate_exception(0x28);
  *(undefined4 *)(puVar1 + 1) = *(undefined4 *)(param_1 + 8);
  *puVar1 = &PTR_FUN_0076eed8;
  std::runtime_error::runtime_error((runtime_error *)(puVar1 + 2),(runtime_error *)(param_1 + 0x10))
  ;
  *puVar1 = &PTR_FUN_0076ef00;
  puVar1[4] = *(undefined8 *)(param_1 + 0x20);
                    /* WARNING: Subroutine does not return */
  __cxa_throw(puVar1,&PTR_vtable_0076ed38,FUN_001e0980);
}

