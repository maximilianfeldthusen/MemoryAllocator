 ## MemoryAllocator

This project is developed as a low-level systems programming and operating-system learning project.

The purpose of the project is to demonstrate practical understanding of low-level concepts including:

    C and assembly
    Memory management
    Physical and virtual memory
    Page tables
    Kernel/user separation
    Interrupts and exceptions
    System calls
    Process address spaces
    x86-64 architecture
    ARM64 architecture
    Kernel heap allocation
    Hardware/software boundaries

AI assistance does not replace understanding of the underlying implementation. The code is intended to be studied, tested, modified, and extended by the author.

### kernel — Project Wiki

A small x86_64 (arm64 planned) kernel. Bare metal, no libc, GRUB/multiboot2.
Current state: boot → serial → PMM → VMM → heap → slab. Processes and
syscalls are stubbed.

## Documentation

Full documentation is available in the [project wiki](https://github.com/maximilianfeldthusen/MemoryAllocator/wiki).


