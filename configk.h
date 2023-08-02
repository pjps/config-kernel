

typedef struct
{
    char *fname;
    uint16_t s_count;
    uint16_t o_count;
} sEntry; /* source entry */


typedef enum
{
    CINT=0x1,
    CHEX=0x2,
    CBOOL=0x3,
    CSTRING=0x4,
    CTRISTATE=0x5
} cType; /* config value type */

typedef struct
{
    char *opt_name;
    cType opt_type;
    char *opt_value;
    char *opt_prompt;
    char *opt_depends;
    char *opt_select;
    char *opt_help;
    int8_t opt_status;
} cEntry; /* config entry */


typedef enum
{
    SENTRY=0x1,
    CENTRY=0x2
} nType; /* Node types */


/* configuration tree node */
typedef struct cNode cNode;
struct cNode
{
    void *data;
    nType type;

    cNode *up;
    cNode *down;
    cNode *next;
};


enum OPTS
{
      BE_VERBOSE = 0x1,
    CHECK_CONFIG = 0x2,
  DISABLE_CONFIG = 0x4,
   ENABLE_CONFIG = 0x8,
     EDIT_CONFIG = 0x10,
     SHOW_CONFIG = 0x20,
   TOGGLE_CONFIG = 0x40
};

extern uint16_t opts;
#define HASHSZ 20000

extern cNode *tree_root(void);
extern cNode *tree_curr_root_up(void);
extern cNode *tree_cnode(void *, nType);
extern cNode *tree_add(cNode *);
extern cNode *tree_init(char *);
extern void tree_display(cNode *);
extern uint16_t tree_reset(cNode *);
extern void check_depends(const char *);
