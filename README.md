
## config-kernel
#### https://docs.google.com/document/d/1i4RrTBTgaWnkRWeqkwbpJBPJBUPATR5ImzkbwvET6_0/

---

### Introduction

config-kernel program recursively reads Kconfig files from a given Linux
source tree and builds a tree structure of CONFIG_ options with their
attributes. This tree can then be traversed to query such attributes'
value or set/reset them.

This program can check and validate a given '.config' kernel configuration
against any given kernel source tree. It supports following options:

  Usage: ./configk [OPTIONS] <source-dir>

  Options:
    -c --config <file>    check configs against the source tree
    -d --depends <option> list option dependencies
    -h --help             show help
    -s --selects <option> list selected options
    -v --version          show version
    -V --verbose          show verbose output

It uses -libfl and -liby libraries from **libfl-devel** and **bison-devel**
packages.

---

### Examples:

  $ dnf install libfl-devel bison-devel

  $ make  <= to build 'configk' binary file.

  $ ./configk ../centos-stream-9/
  $ ./configk ../rhel-8/
  $ **./configk ../linux/**
	...
    Config files: 1523
    Config options: 16784


  $ **./configk -d NO_HZ_FULL /tmp/linux-5.0/**
    NO_HZ_FULL depends on:
     => !ARCH_USES_GETTIMEOFFSET
        SMP
        HAVE_CONTEXT_TRACKING
        HAVE_VIRT_CPU_ACCOUNTING_GEN


  $ **./configk -s X86_64 ../linux/**
    X86_64 selects:
     => ARCH_HAS_GIGANTIC_PAGE
        ARCH_SUPPORTS_INT128
        ARCH_SUPPORTS_PER_VMA_LOCK
        ARCH_USE_CMPXCHG_LOCKREF
        HAVE_ARCH_SOFT_DIRTY
        MODULES_USE_ELF_RELA
        NEED_DMA_MAP_STATE
        SWIOTLB
        ARCH_HAS_ELFCORE_COMPAT
        ZONE_DMA32


  $ **./configk -c /tmp/config-6.3.11-200.fc38.x86_64 ../centos-stream-9/**
    ...
    configk: option 'INIT_ENV_ARG_LIMIT' has invalid int value: 'x32'
    configk: option 'BPF_JIT' has invalid bool value: 'x'
    configk: option 'SCHED_MM_CID' not found in the source tree
    configk: option 'LD_ORPHAN_WARN_LEVEL' not found in the source tree
    ...
