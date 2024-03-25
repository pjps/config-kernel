## config-kernel

config-kernel program recursively reads Kconfig files from a given source
directory and builds a tree structure of CONFIG_ options. This tree can be
traversed to query option's attributes or set/reset them. It can also be
used to validate kernel's '.config' file by checking option's value and
dependencies.

    Kconfig: <#files>, <#options>
     │
     ├── init/Kconfig: 2, 3
     │   ├── CC_VERSION_TEXT
     │   ├── DEFAULT_HOSTNAME
     │   ├── kernel/bpf/Kconfig: 1, 3
     │   │   ├── BPF
     │   │   ├── BPF_JIT
     │   │   ├── BPF_SYSCALL
     │   │   └── preload/kconfig: 0, 2
     │   │       ├── BPF_PRELOAD
     │   │       └── BPF_PRELOAD_UMD
     │   ├── kernel/time: 0, 2
     │   │   ├── LEGACY_TIMER_TICK
     │   │   └── NO_HZ_FULL
     │   └── NAMESPACES
     ├── mm: 0, 4
     │   ├── KSM
     │   ├── MEMORY_HOTPLUG
     │   ├── SLAB
     │   └── SWAP
     └── net: 0, 3
        ├── ETHTOOL_NETLINK
        ├── NET
        └── NETFILTER

[Kconfig language](https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html) based configuration database was introduced in the Linux kernel from
version v2.6.0. ie. config-kernel program works with all kernel
versions >= v2.6.0. It should work for all projects which use Kconfig files
and its syntax to define configuration options. A typical CONFIG entry in a
Kconfig file begins with a 'config' keyword on a new line and its attributes
on the following lines indented by the tab (\t) character:

    config <OPTION_NAME>
    <\t>Attribute-1
    <\t>Attribute-2
      ...
    <\t>Attribute-n

We probably don't need and/or have to maintain complex one-file-per-config-option directory structures.

---

### Quick Start
    0) Make

       $ dnf install libfl-devel bison-devel
       $ make  <= to build 'configk' binary file.

    1) List options in the default hierarchical view:

       $ ./configk ../centos-stream-9/

    2) List options in --config file format view:

       $ ./config -C ../linux/

    3) List options with --Verbose mode enabled:

       $ ./config -V ../centos-stream-9/

    4) List options for a given --srcarch architecture:

       $ ./configk -a powerpc ../centos-stream-9/

    5) List a config option with --show:

       $ ./configk -s NO_HZ_FULL ../linux/

    6) Check/compare a .config file against the given source tree:

       $ ./configk -c /tmp/config-6.4.8-200.fc38.x86_64 ../centos-stream-9/ 2> /tmp/stderr.log

    7) Recursively disable a config option with --disable:

       $ ./configk -d NO_HZ_FULL ../linux/

    8) Recursively disable a config option in a given .config file and view output in --config file format:

       $ ./configk -d NO_HZ_FULL -Cc /tmp/config-6.4.8-200.fc38.x86_64 ../linux/ 2> /tmp/stderr.log

    9) Recursively enable a config option in a given .config file:

       $ ./configk -e NO_HZ_FULL -c /tmp/config-6.4.8-200.fc38.x86_64 ../linux/ 2> /tmp/stderr.log

    10) Recursively toggle a tristate config option between 'y' and 'm' values:

       $ ./configk -c /tmp/config-6.4.8-200.fc38.x86_64  -t TIME_KUNIT_TEST ../linux/ 2> /tmp/stderr.log

    11) --Edit and validate a given .config file with an $EDITOR

       $ EDITOR=vim ./configk -E /tmp/config-6.4.8-200.fc38.x86_64 ../linux/


**configk** program can check and validate a '.config' configuration file
against any given kernel source tree. It supports following options:

    Usage: ./configk [OPTIONS] <source-directory>

    Options:
      -a --srcarch <arch>        set $SRCARCH variable
      -c --check <file>          check configs against the source tree
      -C --config                show output as a config file
      -d --disable <option>      disable config option
      -e --enable <option>[=val] enable config option
      -E --edit <file>           edit config file with an $EDITOR
      -g --grep <[s:]string>     show config option with matching attribute
      -h --help                  show help
      -s --show <option>         show a config option entry
      -t --toggle <option>       toggle an option between y & m
      -v --version               show version
      -V --verbose               show verbose output

It uses -libfl and -liby libraries from **libfl-devel** or **libfl-static**
and **bison-devel** packages.

### Examples:

The **-s** switch shows a config option with its attributes.

    $ ./configk -s NO_HZ_FULL ../centos-stream-9/
    File   : kernel/time/Kconfig
    Config : NO_HZ_FULL
    Type   : bool(3)
    Default: n
    Depends: SMP
           HAVE_CONTEXT_TRACKING_USER
           HAVE_VIRT_CPU_ACCOUNTING_GEN
    Select : NO_HZ_COMMON
           RCU_NOCB_CPU
           VIRT_CPU_ACCOUNTING_GEN
           IRQ_WORK
           CPU_ISOLATION
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


The **--grep** option helps to filter output by a given stirng

    $ ./configk -g EXT4_FS ../centos-stream-9/
        fs/ext4/Kconfig: 0, 9
          EXT4_USE_FOR_EXT2
          EXT4_FS_POSIX_ACL
          EXT4_FS_SECURITY
          EXT4_DEBUG
          EXT4_KUNIT_TESTS
    Config files: 13
    Config options: 9
    Config memory: 6.26 MB

    $ ./configk --grep s:EXT4_FS ../linux/
        fs/ext4/Kconfig: 0, 9
          EXT3_FS
          EXT3_FS_POSIX_ACL
          EXT3_FS_SECURITY
    Config files: 13
    Config options: 9
    Config memory: 6.64 MB

    $ ./configk --grep CGROUPS ../linux/
        net/netfilter/Kconfig: 2, 165
          NETFILTER_XT_MATCH_CGROUP
        net/sched/Kconfig: 0, 87
          NET_CLS_CGROUP
      net/Kconfig: 63, 48
        CGROUP_NET_PRIO
        CGROUP_NET_CLASSID
      lib/Kconfig.debug: 13, 267
        DEBUG_CGROUP_REF
        samples/Kconfig: 1, 43
          SAMPLE_CGROUP
    Config files: 92
    Config options: 610
    Config memory: 6.63 MB


The **-c** option allows to validate a given '.config' or a kernel
configuration template file against a kernel source tree.

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
        CC_VERSION_TEXT: "(GCC) 13.1.1 20230614 (Red Hat 13.1.1-4)"
        CC_IS_GCC: y
        GCC_VERSION: 130101
        CC_IS_CLANG
        CLANG_VERSION: 0    <= Green entries are selected options
        AS_IS_GNU: y
        AS_IS_LLVM
        AS_VERSION: 23900
        LD_IS_BFD: x        <= Yellow indicates value error
        LD_VERSION: 23900
        LD_IS_LLD
        LLD_VERSION: 0
        CC_CAN_LINK: y
        CC_CAN_LINK_STATIC: y
        CC_HAS_ASM_GOTO     <= Whites are non-selected options
        CC_HAS_ASM_GOTO_OUTPUT: y
        CC_HAS_ASM_GOTO_TIED_OUTPUT: y
        ILLEGAL_POINTER_VALUE: 0kdead0000m0000000
        TOOLS_SUPPORT_RELR
        CC_HAS_ASM_INLINE: y
        CC_HAS_NO_PROFILE_FN_ATTR: y
        PAHOLE_VERSION: 125
        CONSTRUCTORS
        ...
