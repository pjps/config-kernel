## config-kernel
#### https://docs.google.com/document/d/1i4RrTBTgaWnkRWeqkwbpJBPJBUPATR5ImzkbwvET6_0/

### Introduction

config-kernel program recursively reads Kconfig files from a given source tree
and builds a tree structure of CONFIG_ options with their attributes.
This tree can be traversed to query attributes' value or set/reset them. Also
can be used to validate kernel's '.config' file by checking dependencies
between options and/or their data type & values.

    Kconfig: <#files>, <#options>
     │
     ├── init/Kconfig: 0, 3
     │   ├── CC_VERSION_TEXT
     │   ├── DEFAULT_HOSTNAME
     │   └── NAMESPACES
     ├── kernel/bpf/Kconfig: 1, 3
     │   ├── BPF
     │   ├── BPF_JIT
     │   ├── BPF_SYSCALL
     │   └── preload/kconfig: 0, 3
     │       ├── BPF_PRELOAD
     │       └── BPF_PRELOAD_UMD
     ├── kernel/time: 0, 2
     │   ├── LEGACY_TIMER_TICK
     │   └── NO_HZ_FULL
     ├── mm
     └── net


Kconfig language based configuration database was introduced in the Linux
kernel from version v2.6.0. ie. config-kernel program works with all kernel
versions >= v2.6.0.

    $ ./configk ../rhel-7/
      ...
      Config files: 842
      Config options: 8934

    $ ./configk ../rhel-8/
      ...
      Config files: 1260
      Config options: 13710

    $ ./configk ../centos-stream-9/
      ...
      Config files: 1423
      Config options: 16044

    $ ./configk ../linux-upstream/
      ...
      Config files: 1523
      Config options: 16784


_configk_ program can check and validate a given '.config' configuration
against any given kernel source tree. It supports following options:

    Usage: ./configk [OPTIONS] <source-dir>

    Options:
      -c --config <file>    check configs against the source tree
      -d --disable <option> disable config option
      -e --enable <option>  enable config option
      -h --help             show help
      -s --show <option>    show a config option entry
      -v --version          show version
      -V --verbose          show verbose output

It uses -libfl and -liby libraries from **libfl-devel** and **bison-devel**
packages.

---

### Examples:

    $ dnf install libfl-devel bison-devel
    $ make  <= to build 'configk' binary file.

The **-s** switch shows all available attributes of a config entry.

    $ ./configk -s NO_HZ_FULL ../centos-stream-9/
     Config : NO_HZ_FULL
     Type   : bool(3)
     Depends: SMP,HAVE_CONTEXT_TRACKING_USER,HAVE_VIRT_CPU_ACCOUNTING_GEN
     Select : NO_HZ_COMMON,RCU_NOCB_CPU,VIRT_CPU_ACCOUNTING_GEN,IRQ_WORK,CPU_ISOLATION
     Help   :

         Adaptively try to shutdown the tick whenever possible, even when
         the CPU is running tasks. Typically this requires running a single
         task on the CPU. Chances for running tickless are maximized when
         the task mostly runs in userspace and has few kernel activity.

         You need to fill up the nohz_full boot parameter with the
         desired range of dynticks CPUs to use it. This is implemented at
         the expense of some overhead in user <-> kernel transitions:
         syscalls, exceptions and interrupts.

         By default, without passing the nohz_full parameter, this behaves just
         like NO_HZ_IDLE.

         If you're a distro say Y.


The **-d & -e** command line options help to recursively disable and enable a
given config option.

    $ ./configk -e ACPI_PROCESSOR ../linux/
     Enable option:
      ACPI_PROCESSOR
        ACPI_PROCESSOR_IDLE
          CPU_IDLE
            CPU_IDLE_GOV_LADDER
            CPU_IDLE_GOV_MENU
        ACPI_CPU_FREQ_PSS
        THERMAL


The **-c** option allows to validate a given '.config' or any kernel
configuration template file against any given kernel source tree.

    $ ./configk -c /tmp/config-6.3.11-200.fc38.x86_64 ../centos-stream-9/
    ...
    configk: option 'INIT_ENV_ARG_LIMIT' has invalid int value: 'x32'
    configk: option 'BPF_JIT' has invalid bool value: 'x'
    configk: option 'SCHED_MM_CID' not found in the source tree
    configk: option 'LD_ORPHAN_WARN_LEVEL' not found in the source tree
    ...

    Kconfig: 14, 0
      scripts/Kconfig.include: 0, 0
      init/Kconfig: 9, 231
        [32mCC_VERSION_TEXT: "(GCC) 13.1.1 20230614 (Red Hat 13.1.1-4)"[0m
        [32mCC_IS_GCC: y[0m
        [32mGCC_VERSION: 130101[0m
        CC_IS_CLANG
        [32mCLANG_VERSION: 0[0m    <= Green indicates selected options
        [32mAS_IS_GNU: y[0m
        AS_IS_LLVM
        [32mAS_VERSION: 23900[0m
        [33mLD_IS_BFD: x[0m        <= Yellow indicates value error
        [32mLD_VERSION: 23900[0m
        LD_IS_LLD
        [32mLLD_VERSION: 0[0m
        [32mCC_CAN_LINK: y[0m
        [32mCC_CAN_LINK_STATIC: y[0m
        CC_HAS_ASM_GOTO     <= Whites are non-selected options
        [32mCC_HAS_ASM_GOTO_OUTPUT: y[0m
        [32mCC_HAS_ASM_GOTO_TIED_OUTPUT: y[0m
        [33mILLEGAL_POINTER_VALUE: 0kdead0000m0000000[0m
        TOOLS_SUPPORT_RELR
        [32mCC_HAS_ASM_INLINE: y[0m
        [32mCC_HAS_NO_PROFILE_FN_ATTR: y[0m
        [32mPAHOLE_VERSION: 125[0m
        CONSTRUCTORS
        [32mIRQ_WORK: y[0m
        [32mBUILDTIME_TABLE_SORT: y[0m
        [32mTHREAD_INFO_IN_TASK: y[0m
        BROKEN
        BROKEN_ON_SMP
        [33mINIT_ENV_ARG_LIMIT: ml32[0m
        COMPILE_TEST
        WERROR
