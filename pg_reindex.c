/*
 * pg_reindex.c - Utility for PostgreSQL index rebuilding
 * and showing related index statistic.
 * Version: see VERSION in the pg_reindex.h
 * Date: 30-07-2018
 * Author: Andrey Klychkov <aaklychkov@mail.ru>
 * See README.md on https://github.com/Andersson007
 */ 
#include <ctype.h>
#include <getopt.h>
#include <libpq-fe.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "headers/pg_reindex_sql.h"
#include "headers/pg_reindex.h"
#include "headers/logging.h"


int main(int argc, char **argv)
{
	log_fp = NULL;
        int ret = 0;
	char* conn_pref = NULL;
	char *conninfo = NULL;
	PGconn *conn = NULL;
	FILE *file = NULL;
        char *fname = NULL;
	char *str = NULL;
	char *ptr = NULL;
	char buf[68];

	// Default values of command-line arguments:
	glob_args.db_name = NULL;
	glob_args.idx_name = NULL;
	glob_args.idx_filename = NULL;
	glob_args.size_thresh = NULL;
	glob_args.st_timeout = STATEMENT_TIMEOUT;
	glob_args.log_filename = LOG_FILE;
	glob_args.stat = 0;
	glob_args.inval = 0;
	glob_args.new_pref = 0;

	// Get command-lime arguments:
	get_opts(argc, argv);


	log_fp = fopen(glob_args.log_filename, "a+");
	if (!log_fp) {
		fprintf(stderr,
			"Could not open the log file %s\n", LOG_FILE);
		exit(1);
	}

	printf("log is collecting to %s\n", glob_args.log_filename);

	conn_pref = "dbname=";
	conninfo = (char*)malloc((strlen(conn_pref) +
		strlen(glob_args.db_name) + 1) * sizeof(char));
	strcpy(conninfo, conn_pref);
	strcat(conninfo, glob_args.db_name);

	// Establish a database connection:
	conn = PQconnectdb(conninfo);

	// Check this connection:
	if (PQstatus(conn) == CONNECTION_OK)
		log_write(log_fp, INF, "Connection established\n");
        else {
		fprintf(stderr, "Connection to database failed: %s\n",
			PQerrorMessage(conn));
		log_write(log_fp, ERR, "Connection failed\n");
		free(conninfo);
		exit_nicely(conn);
	}

	free(conninfo);

	// Print indexes with the "new_" prefix:
	if (glob_args.new_pref) {
		log_write(log_fp, INF, "Show show new_ indexes\n");
		show_new_pref_idx(conn);
	}

	// Print top of bloated indexes:
	if (glob_args.stat) {
		log_write(log_fp, INF, "Show bloat stat\n");
		print_bloat_stat(conn);
	}

	// Print invalid indexes:
	if (glob_args.inval) {
		log_write(log_fp, INF, "Show invalid indexes\n");
		print_invalid_idx(conn);
	}

	// Print unused indexes with size more than passed "size_thresh":
	if (glob_args.size_thresh) {
		log_write(log_fp, INF, "Show unused indexes\n");
		print_not_used_idx(conn, "0", glob_args.size_thresh);
	}

	// Rebuild an index with a passed name:
	if (glob_args.idx_name) {
		print_now_time();
		printf("Rebuild index %s\n", glob_args.idx_name);

		ret = rebuild_idx(conn, glob_args.idx_name);

		print_now_time();
		if (ret) {
			printf("Rebuilding is done\n");
		} else {
			printf("Rebuilding is FAILED, "
			       "see the log for more info\n");
		}
	}

	// Rebuild index(es) with name(s) from a passed file:
	if (glob_args.idx_filename) {
		fname = glob_args.idx_filename;
		log_write(log_fp, INF,
			  "Rebuild indexes from the file %s\n", fname);

		file = fopen(fname, "r");

		if (!file) {
			fprintf(stderr,
				"Could not open the file %s\n", fname);
			log_write(log_fp, ERR,
			          "Could not open the file %s\n", fname);
			exit_nicely(conn);
		}

		while (1) {
			str = fgets(buf, sizeof(buf), file);

			if (str == NULL) {
				if (feof(file) != 0) {
					break;
				} else {
					fprintf(stderr,
                                                "Error of reading file, exit\n");
					log_write(log_fp, ERR,
                                                  "Error of reading file\n");
					exit_nicely(conn);
				}
			}

			if (isalpha(str[0])) {
				ptr = strchr(str, '\n');
				if (ptr != NULL) *ptr = '\0';

				// For the each indexname in the file, do:
				rebuild_idx(conn, str);
			}
		}
	}

	// Close a connection to the database and cleanup:
	PQfinish(conn);
	return 0;
}


// get_opts(): the wrap function for parsing
// command-line arguments
static void get_opts(int argc, char **argv)
{
	int opt = 0;

	opt = getopt(argc, argv, opt_string);
	while (opt != -1) {
		switch(opt) {
			case 'd':
				glob_args.db_name = optarg;
				break;
			case 'r':
				glob_args.idx_name = optarg;
				break;
			case 'f':
				glob_args.idx_filename = optarg;
				break;
			case 't':
				glob_args.st_timeout = optarg;
				break;
			case 'u':
				glob_args.size_thresh = optarg;
				break;
			case 'l':
				glob_args.log_filename = optarg;
				break;
			case 's':
				glob_args.stat = 1;
				break;
			case 'i':
				glob_args.inval = 1;
				break;
			case 'n':
				glob_args.new_pref = 1;
				break;
			case 'h':
				print_help(0);
				break;
			case 'v':
				printf("%s\n", VERSION);
				exit(0);
			default:
				break;
		}

		opt = getopt(argc, argv, opt_string);
	}

	if (argc < 4)
		print_help(1);

	// If mutual excluseve args have been passed,
	// print help and exit with rcode 1:
	if (!glob_args.db_name)
		print_help(1);

	if ((glob_args.stat || glob_args.size_thresh || glob_args.inval) &&
	    (glob_args.idx_name || glob_args.idx_filename))
		print_help(1);

	if (glob_args.idx_name && glob_args.idx_filename)
		print_help(1);
}


// exit_nicely(): close the connection to the database and exit
static void exit_nicely(PGconn *conn)
{
	PQfinish(conn);
	exit(1);
}


// print_bloat_stat(): print bloat statistic
// for top of 50 indexes by bloat size
static void print_bloat_stat(PGconn *conn)
{
	PGresult *res;
	PQprintOpt option = {0};

	res = PQexec(conn, IDX_BLOAT_STAT_SQL);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "QUERY failed: %s\n",
		        PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	if (!PQntuples(res))
		printf("No bloated indexes found\n");
	else {
		option.header = 1;
		option.align = 1;
		option.fieldSep = "|";

		PQprint(stdout, res,  &option);
	}

	PQclear(res);
	exit_nicely(conn);
}


// show_new_pref_idx(): show indexes with
// the "new_" prefix in index names
static void show_new_pref_idx(PGconn *conn)
{
	PGresult *res;
	PQprintOpt option = {0};

	res = PQexec(conn, SHOW_NEW_PREF_IDX_SQL);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "QUERY failed: %s\n",
                        PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	if (!PQntuples(res))
		printf("No \"new_\" indexes found\n");
	else {
		option.header = 1;
		option.align = 1;
		option.fieldSep = "|";

		PQprint(stdout, res,  &option);
	}

	PQclear(res);
	exit_nicely(conn);
}


// print_invalid_idx(): show invalid indexes
static void print_invalid_idx(PGconn *conn)
{
	PGresult *res;
	PQprintOpt option = {0};

	res = PQexec(conn, GET_INV_IDX_SQL);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "QUERY failed: %s\n",
                        PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	if (!PQntuples(res))
		printf("No invalid indexes found\n");
	else {
		option.header = 1;
		option.align = 1;
		option.fieldSep = "|";

		PQprint(stdout, res,  &option);
	}

	PQclear(res);
	exit_nicely(conn);
}


// print_not_used(): print unused indexes with
// size larger than threshold
// and the scan counter less than scan_count
static void print_not_used_idx(PGconn *conn,
                               char *scan_count, char *threshold)
{
	PGresult *res;
	PQprintOpt option = {0};
	const char *param_values[2];

	param_values[0] = scan_count;
	param_values[1] = threshold;

	res = PQexecParams(conn,
			   GET_UNUSED_IDX_SQL,
			   2,
			   NULL,
			   param_values,
			   NULL,
			   NULL,
			   0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "QUERY failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	if (!PQntuples(res))
		printf("Not used indexes not found.\n");
	else {
		option.header = 1;
		option.align = 1;
		option.fieldSep = "|";

		PQprint(stdout, res,  &option);
	}

	PQclear(res);
	exit_nicely(conn);
}


// get_rel_size(): get relation size
unsigned get_rel_size(PGconn *conn, char *relname)
{
	PGresult *res;
	unsigned size;
	const char *param_values[1];

	param_values[0] = relname;

	res = PQexecParams(conn,
			   GET_REL_SIZE_SQL,
			   1,
			   NULL,
			   param_values,
			   NULL,
			   NULL,
			   0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "QUERY failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	if (PQntuples(res)) 
		size = atoi(PQgetvalue(res, 0, 0));
	else
		size = 0;

	PQclear(res);
	return size;
}


/*
 * REBUILDING FUNCTIONS BELOW
 */

// check_idx_name(): check a passed index name
int check_idx_name(PGconn *conn, char *iname)
{
	PGresult *res;
	const char *param_values[1];

        // The index name is too long:
	if (strlen(iname) > 63) 
		return 2;

	param_values[0] = iname;

	res = PQexecParams(conn,
			   CHECK_REL_EXIST_SQL,
			   1,
			   NULL,
			   param_values,
			   NULL,
			   NULL,
			   0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		log_write(log_fp, ERR, "QUERY failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	if (!PQntuples(res)) {
		PQclear(res);
		return 0;  // Passed index not found
	} else {
		PQclear(res);
		return 1;  // Index exists
	}
}


// chack_idx_validity(): check index validity
int check_idx_validity(PGconn *conn, char *iname)
{
	PGresult *res;
	const char *param_values[1];

	param_values[0] = iname;

	res = PQexecParams(conn,
			   CHECK_IDX_VALID_SQL,
			   1,
			   NULL,
			   param_values,
			   NULL,
			   NULL,
			   0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		log_write(log_fp, ERR, "QUERY failed: %s\n",
			  PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	if (PQntuples(res)) {
		PQclear(res);
		return 0;  // The index is invalid
	} else {
		PQclear(res);
		return 1;  // The index is valid
	}
}


// get_indexdef(): get the definition of
// the index from pg_indexes
char *get_indexdef(PGconn *conn, char *iname)
{
	PGresult *res;
	const char *param_values[1];
	char *idef;

	param_values[0] = iname;

	res = PQexecParams(conn,
			   GET_IDXDEF_SQL,
			   1,
			   NULL,
			   param_values,
			   NULL,
			   NULL,
			   0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		log_write(log_fp, ERR, "QUERY failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	if (PQntuples(res)) {
		idef = (char *)malloc(strlen(PQgetvalue(res, 0, 0)) * sizeof(char) + 1);
		strcpy(idef, PQgetvalue(res, 0, 0));
		PQclear(res);
	} else 
		idef = NULL;  // Not indexdef found

	return idef;
}


// get_idx_comment(): if the comment of index exists, get it
char *get_idx_comment(PGconn *conn, char *iname)
{
	PGresult *res;
	const char *param_values[1];
	char *icomm;

	param_values[0] = iname;

	res = PQexecParams(conn,
			   GET_ICOMMENT_SQL,
			   1,
			   NULL,
			   param_values,
			   NULL,
			   NULL,
			   0);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		log_write(log_fp, ERR, "QUERY failed: %s\n",
			  PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	if (PQntuples(res) > 0) {
		// Comment exists:
		if (strcmp(PQgetvalue(res, 0, 0),"\0")) {
			icomm = (char*)malloc(strlen(PQgetvalue(res, 0, 0)) *
					      sizeof(char) + 1);
			strcpy(icomm, PQgetvalue(res, 0, 0));
        	} else {
			icomm = NULL;
        	}
	} else 
		icomm = NULL;

	PQclear(res);
	return icomm;
}


// make_new_iname(): make a temporary name for a new index
char *make_new_iname(char *iname)
{
	char *new_iname;

	new_iname = (char*)malloc(67 * sizeof(char));

	strcpy(new_iname, "new_");
	strcat(new_iname, iname);

	return new_iname;
}


// make_creat_cmd: make a creation command
char *make_creat_cmd(char *new_iname, char *idef)
{
        unsigned i = 0;
        unsigned space_count = 0;
        char *cmd = NULL;
        char *buf = NULL;

	cmd = (char*)malloc(1024 * sizeof(char));
	buf = (char*)malloc(1024 * sizeof(char));

	strcpy(cmd, "CREATE INDEX CONCURRENTLY ");

	space_count = 0;
	for (i = 0; idef[i] != '\0'; i++) {
		if (idef[i] == ' ')
			space_count++;

		if (space_count >= 3) {
            		strcpy(buf, &idef[i]);
            		break; 
        	}
    	}

	strcat(cmd, new_iname);
	strcat(cmd, buf);

	free(buf);

	return cmd;
}


// create_idx(): create index
int creat_idx(PGconn *conn, char *cmd)
{
	PGresult *res;

    	log_write(log_fp, INF, "%s\n", cmd);

    	res = PQexec(conn, cmd);
    	if (PQresultStatus(res) == PGRES_COMMAND_OK) {
        	PQclear(res);
        	return 1;
    	} else {
        	log_write(log_fp, ERR, "QUERY failed: %s\n",
			  PQerrorMessage(conn));
        	PQclear(res);
        	return 0;
    	}
}


// add_commeent(): add comment for index
int add_comment(PGconn *conn, char *iname, char *comment)
{
    	PGresult *res;
    	char *str = "COMMENT ON INDEX ";
    	char *add_comment_sql;

    	add_comment_sql = malloc((strlen(str) + 7 +
                                  strlen(comment) + strlen(iname)) *
				  sizeof(char));

    	strcpy(add_comment_sql, str);
    	strcat(add_comment_sql, iname);
    	strcat(add_comment_sql, " IS '");
    	strcat(add_comment_sql, comment);
    	strcat(add_comment_sql, "'");

    	res = PQexec(conn, add_comment_sql);
    	free(add_comment_sql);

    	if (PQresultStatus(res) == PGRES_COMMAND_OK) {
        	PQclear(res);
        	return 1;
    	} else {
        	log_write(log_fp, ERR, "QUERY failed: %s\n",
			  PQerrorMessage(conn));
        	PQclear(res);
        	return 0;
    	}
}


// drop_idx(): drop an index
int drop_idx(PGconn *conn, char *iname)
{
	PGresult *res;
	char *drop_cmd;

	// 24 is a length of "DROP INDEX CONCURRENTLY + 1 '\0'"
	drop_cmd = (char*)malloc((25 + strlen(iname)) * sizeof(char));

	strcpy(drop_cmd, "DROP INDEX CONCURRENTLY ");
	strcat(drop_cmd, iname);

	log_write(log_fp, INF, "%s\n", drop_cmd);

	res = PQexec(conn, drop_cmd);
	free(drop_cmd);

	if (PQresultStatus(res) == PGRES_COMMAND_OK) {
		PQclear(res);
		return 1;
	} else {
		log_write(log_fp, ERR, "QUERY failed: %s\n",
			  PQerrorMessage(conn));
		PQclear(res);
		return 0;
	}
}


// rename_idx(): rename an index
int rename_idx(PGconn *conn, char *tmp_iname, char *iname)
{
	PGresult *res;
	char *rename_cmd;

	// 23 is a length of "ALTER INDEX RENAME TO " + 1 '\0'
	rename_cmd = (char*)malloc((24 + strlen(tmp_iname) + strlen(iname))
		                       * sizeof(char));

	strcpy(rename_cmd, "ALTER INDEX ");
	strcat(rename_cmd, tmp_iname);
	strcat(rename_cmd, " RENAME TO ");
	strcat(rename_cmd, iname);

	log_write(log_fp, INF, "%s\n", rename_cmd);

	res = PQexec(conn, rename_cmd);
	free(rename_cmd);

	if (PQresultStatus(res) == PGRES_COMMAND_OK) {
		PQclear(res);
		return 1;
	} else {
		log_write(log_fp, ERR, "QUERY failed: %s\n",
			  PQerrorMessage(conn));
		PQclear(res);
		return 0;
	}
}


// rebuild index(): the main function for rebuilding
int rebuild_idx(PGconn *conn, char *iname)
{
	unsigned prev_size, 
		 next_size,
		 diff;
        int ret = SUCCESS;
	char *indexdef = NULL;
	char *idx_comment = NULL;
	char *new_iname = NULL;
	char *creat_cmd = NULL;

	log_write(log_fp, INF, "== Start to rebuild index ==: %s\n", iname);

	// Check the index is into the database:
	ret = check_idx_name(conn, iname);
	if (ret == FAIL) {
		log_write(log_fp, ERR,
			  "Index with specified index name not found. Exit\n");
		return FAIL;

	} else if (ret == 2) {
		log_write(log_fp, ERR, "Index name is too long. Exit\n");
		return FAIL;
	}

	// Check index validity:
	if (!check_idx_validity(conn, iname)) {
		log_write(log_fp, ERR, "Index is invalid. Exit\n");
		return FAIL;
	}

	// Get size of the current index for statistic:
	prev_size = get_rel_size(conn, iname);

	// Get the index definition:
	if ((indexdef = get_indexdef(conn, iname)) != NULL)
		log_write(log_fp, INF, "Indexdef: %s\n", indexdef);
	else {
		log_write(log_fp, ERR, "Indexdef not found. Exit\n");
		free(indexdef);
		return FAIL;
	}

	// Get the index comment if exist:
	if ((idx_comment = get_idx_comment(conn, iname)) != NULL)
		log_write(log_fp, INF,
			  "Comment of index: '%s'\n", idx_comment);
	else
		log_write(log_fp, INF,
			  "Comment of index not found. Continue\n");

	// Make a new index name:
	new_iname = make_new_iname(iname);
	log_write(log_fp, INF, "Temporary new index name: %s\n", new_iname);

	// Check the name for the new index:
	if (check_idx_name(conn, new_iname) == 1) {
		log_write(log_fp, ERR,
			  "Index with name %s exists. Exit\n", new_iname);
		free(indexdef);
		free(idx_comment);
		free(new_iname);
		exit_nicely(conn);
	}

	// Make a creation command for the new index:
	if ((creat_cmd = make_creat_cmd(new_iname, indexdef)) == NULL) {
		log_write(log_fp, ERR,
			  "Can not make creation command for new index. Exit\n");
		free(indexdef);
		free(idx_comment);
		free(new_iname);
		exit_nicely(conn);
	}

	// Create a new index:
	log_write(log_fp, INF, "Try to create new index\n");

	if (creat_idx(conn, creat_cmd) == 1)
		log_write(log_fp, INF, "Index has been created\n");
	else {
		log_write(log_fp, ERR, "Creation FAILED. Exit\n");
		free(indexdef);
		free(idx_comment);
		free(new_iname);
		exit_nicely(conn);
	}
	free(creat_cmd);

	// Check the new index validity:
	if (!check_idx_validity(conn, new_iname)) {
		log_write(log_fp, ERR,
			  "New index is invalid. Drop it manually. Exit\n");
		free(indexdef);
		free(idx_comment);
		free(new_iname);
		exit_nicely(conn);
	}

	// Add the comment if it was:
	if (idx_comment != NULL) {
		if (add_comment(conn, new_iname, idx_comment))
			log_write(log_fp, INF, "Comment has been added\n");
		else 
			log_write(log_fp, WRN, "Comment is not added\n");

		free(idx_comment);
	}

	// Drop the index:
	log_write(log_fp, INF, "Try to drop previous index\n");

	if (drop_idx(conn, iname) == 1) {
		log_write(log_fp, INF, "Index has been dropped\n");

        	// Rename the index:
		log_write(log_fp, INF,
			  "Try to rename new index like previous\n");


		// Set statement_timeout for RENAME:
		set_statement_timeout(conn, glob_args.st_timeout);

		if (rename_idx(conn, new_iname, iname)) {
			log_write(log_fp, INF,
				  "Index has been renamed\n");

			// Get size of the rebuilded index for statistic:
			next_size = get_rel_size(conn, iname);
			diff = prev_size - next_size;
			log_write(log_fp, INF,
				  "Prev idx size: %d, new idx size: %d, diff: %d\n",
				  prev_size, next_size, diff);
		} else {
			log_write(log_fp, ERR, "Can not rename index\n");
			ret = FAIL;
		}

	} else {
		ret = FAIL;
		log_write(log_fp, ERR, "Can not drop index\n");
	}

        set_statement_timeout(conn, "0");

	free(indexdef);
	free(new_iname);

	if (ret == SUCCESS)
        	log_write(log_fp, INF, "== Rebuilding is done ==\n");
        else 
		log_write(log_fp, ERR, "== Rebuilding failed ==\n");

        return ret;
}


// set_statement_timeout: set the statement timeout
// for the current session
void set_statement_timeout(PGconn *conn, char *sec)
{
	PGresult *res;
	char *str = "SET statement_timeout = '";
	char cmd[strlen(str) + strlen(sec) + 4];

	strcpy(cmd, str);
	strcat(cmd, sec);
	strcat(cmd, "s';");
	log_write(log_fp, INF, "%s\n", cmd);
	res = PQexec(conn, cmd);
	PQclear(res);
}


// print_now_time: print the current date and time
void print_now_time(void)
{
	time_t rawtime;
	struct tm *t_info;

	time(&rawtime);
	t_info = localtime (&rawtime);

	printf("%04d/%02d/%02d %02d:%02d:%02d ",t_info->tm_year+1900,
						t_info->tm_mon+1,
						t_info->tm_mday,
						t_info->tm_hour,
						t_info->tm_min,
						t_info->tm_sec);
}


// print_help: print a help message
void print_help(int rcode)
{
	if (rcode != 0) {
		printf("\nSyntax Error! Do: %s -d DBNAME "
			"[OPTIONS]\nUse -h for more info\n\n", PROG_NAME);
	} else {
		printf("%s - Rebuild postgresql indexes and show related statistic\n"
		       "\nUSE: %s -d DBNAME [OPTIONS]\n"
		       "Options:\n"
		       "  -s		Show top of bloated indexes\n"
		       "  -i		Show invalid indexes\n"
		       "  -n		Show indexes with the \"new_\" prefix\n"
		       "  -r IDXNAME	Rebuild the specified index\n"
		       "  -f FILENAME	Take indexname(s) from the file\n"
		       "		(one name per line) and rebuild them successively\n"
		       "  -u SIZE_THRESH\n"
		       "		Show not used indexes with size more than SIZE_THRESH in bytes\n"
		       "  -l LOGFILE	Set up the LOGFILE (/tmp/pg_reindex.log by default)\n"
		       "  -t SEC	Set up the statement_timeout in SEC (10 sec by default)\n"
		       "  -v		Print version and exit\n"
		       "  -h		Print this message and exit\n\n", PROG_NAME, PROG_NAME);
	}

	exit(rcode);
}
