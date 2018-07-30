#ifndef PG_REINDEX_H
#define PG_REINDEX_H

#define PROG_NAME "pg_reindex"

#define SUCCESS 1
#define FAIL 0

#define VERSION "1.1.3"

// Default statement timeout:
#define STATEMENT_TIMEOUT "5"

// Default log file:
#define LOG_FILE "/tmp/pg_reindex.log"

// Allowable command-line arguments:
static const char* opt_string = "d:r:f:u:l:t:nsihv";

// Global arguments struct:
struct glob_args_t {
	char* db_name;		// -d param
	char* idx_name;		// -i param
	char* idx_filename;	// -f param
	char* size_thresh;	// -u param
	char* st_timeout;	// -t param
	char* log_filename;	// -l param
	int new_pref;		// -n
	int stat;		// -s
	int inval;		// -i
} glob_args;

// Wrap function for parsing cli args:
static void get_opts(int argc, char **argv);

// File ptr for logging:
FILE* log_fp;

// Stat functions:
static void print_bloat_stat(PGconn *conn);

static void print_invalid_idx(PGconn *conn);

static void print_not_used_idx(PGconn *conn, char *s, char *t);

static void show_new_pref_idx(PGconn *conn);

// Primary functions:
static void exit_nicely(PGconn *conn);

int check_idx_name(PGconn *conn, char *iname);

int check_idx_validity(PGconn *conn, char *iname);

int rebuild_idx(PGconn *conn, char *iname);

char* get_indexdef(PGconn *conn, char *iname);

char* get_idx_comment(PGconn *conn, char *iname);

char* make_new_iname(char *iname);

char* make_creat_cmd(char *new_iname, char *idef);

int creat_idx(PGconn *conn, char *cmd);

int add_comment(PGconn *conn, char *iname, char *comment);

int drop_idx(PGconn *conn, char *iname);

int rename_idx(PGconn *conn, char *tmp_iname, char *iname);

int rebuild_from_file(PGconn *conn, char *filename);

void set_statement_timeout(PGconn *conn, char *sec);

unsigned get_rel_size(PGconn *conn, char *relname);

void print_help(int rcode);

void print_now_time(void);
#endif
