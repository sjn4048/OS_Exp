#ifndef _SYSCALLS_H
#define _SYSCALLS_H

unsigned int syscall4(unsigned int status, unsigned int cause, context* pt_context);
unsigned int kernel_printf_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int kmalloc_syscall(unsigned int status, unsigned int cause, context* pt_context);


unsigned int fs_open_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int fs_read_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int fs_close_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int fs_create_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int fs_lseek_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int fs_write_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int fs_open_dir_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int fs_read_dir_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int get_filename_syscall(unsigned int status, unsigned int cause, context* pt_context);

unsigned int kernel_putchar_at_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int kernel_getchar_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int kernel_set_cursor_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int kernel_clear_screen_syscall(unsigned int status, unsigned int cause, context* pt_context);


#endif  // ! _SYSCALLS_H