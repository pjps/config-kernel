
typedef enum {
    CINT=0x1, CHEX=0x2, CBOOL=0x3,
    CSTRING=0x4, CTRISTATE=0x5
} cType;

typedef struct {
    char *opt_name;
    cType opt_type;
    char *opt_value;
    char *opt_prompt;
    char *opt_depends;
    char *opt_select;
    char *opt_help;
} cEntry;

enum OPTS {
      BE_VERBOSE = 0x1,
    CHECK_CONFIG = 0x2,
    LIST_DEPENDS = 0x4,
    LIST_SELECTS = 0x8,
};

extern uint8_t opts;
extern uint16_t oindex;
#define OPTLSTSZ 20000
extern cEntry oList[OPTLSTSZ];
