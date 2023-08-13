
#include <err.h>
#define __USE_GNU
#include <search.h>

typedef struct
{
    char *fname;
    uint16_t s_count;
    uint16_t o_count;
    uint16_t u_count;
} sEntry; /* source entry */


typedef enum
{
    CINT=0x1,
    CHEX=0x2,
    CBOOL=0x3,
    CSTRING=0x4,
    CTRISTATE=0x5,
    CVALNOSET=0x6
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
     OUT_VERBOSE = 0x1,
      OUT_CONFIG = 0x2,
         OUTMASK = 0x3,
    CHECK_CONFIG = 0x4,
  DISABLE_CONFIG = 0x8,
   ENABLE_CONFIG = 0x10,
     EDIT_CONFIG = 0x20,
     SHOW_CONFIG = 0x40,
   TOGGLE_CONFIG = 0x80
};

enum INDX
{
    IPROG = 0x0,
    ISOPT = 0x1,
    IDOPT = 0x2,
    IEOPT = 0x3,
    ITOPT = 0x4,
    IARCH = 0x5,
    IEDTR = 0x6,
    ITMPD = 0x7,
   GSTRSZ = 0x8
};

extern uint16_t opts;
#define HASHSZ 20000
extern struct hsearch_data chash;

extern cNode *tree_root(void);
extern cNode *tree_curr_root_up(void);
extern cNode *tree_cnode(void *, nType);
extern cNode *tree_add(cNode *);
extern cNode *tree_init(char *);
extern void tree_display(cNode *);
extern uint16_t tree_reset(cNode *);
extern void check_depends(const char *);
extern void tree_display_config(cNode *);
extern cNode *hsearch_kconfigs(const char *);
