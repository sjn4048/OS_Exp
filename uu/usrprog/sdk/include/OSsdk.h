#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#define SYSCALL_DEFINE1(name,arg1) ((unsigned int (*)(unsigned int func, unsigned int entry, void * arg1))(TRANSFORM)((unsigned int)sys##name,ENTRY_ADDR,arg1))
// #define SYSCALL_DEFINE1(name, ...) SYSCALL_DEFINEx(1, _##name, __VA_ARGS__)
// #define SYSCALL_DEFINE2(name, ...) SYSCALL_DEFINEx(2, _##name, __VA_ARGS__)
// #define SYSCALL_DEFINE3(name, ...) SYSCALL_DEFINEx(3, _##name, __VA_ARGS__)
// #define SYSCALL_DEFINE4(name, ...) SYSCALL_DEFINEx(4, _##name, __VA_ARGS__)
// #define SYSCALL_DEFINE5(name, ...) SYSCALL_DEFINEx(5, _##name, __VA_ARGS__)
// #define SYSCALL_DEFINE6(name, ...) SYSCALL_DEFINEx(6, _##name, __VA_ARGS__)

#define test4(arg1) SYSCALL_DEFINE1(_test4, arg1)


int ENTRY_ADDR = 0;

unsigned int sys_test4(int arg);

const unsigned int TRANSFORM = 0x80000900;

int (*transform)(unsigned int func, unsigned int entry, void * arg1, void * arg2) = (int (*)(unsigned int func, unsigned int entry, void * arg1, void * arg2))(TRANSFORM);

void sdk_init(unsigned int argc, void *args, unsigned int entry_point);



#endif