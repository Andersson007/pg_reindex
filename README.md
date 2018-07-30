# pg_reindex
pg_reindex - Concurrent rebuilding of PostgreSQL indexes and showing related index statistic

v. 1.1.3

Date: 30-07-2018

Author: Andrey Klychkov <aaklychkov@mail.ru>

### Requirements:
```gcc, libpq```

### Compilation:
```
cd <this_prog_src>
make
```
Note: don't forget to change the Makefile in the part of the path to libpq files in your system.
In my case (CentOS 7, the libpq is installed by yum install postgresql10-devel): 
```
CFLAGS=-I /usr/pgsql-10/include/ ...
LDLIBS=-L /usr/pgsql-10/lib -lpq
```

### Important Information:
During execution ALTER INDEX commands the table is locked and all queries are not executed until the commands are fulfilled. To avoid the occurrence of queues the statement_timeout set in the const STATEMENT_TIMEOUT into the headers/pg_reindex.h (initially set to 5 seconds). After the specified time the command will be interrupted (that you'll see in the log) and it needs to be done manually into the database, see "Understanding of the concurrent index rebuilding" below. You may change the STATEMENT_TIMEOUT value by using the -t <NUM_SEC> command-line argument. 

### Descriprion:
pg_reindex - rebuild postgresql indexes (concurrently) or show:
```
1) top of bloated indexes (maybe you want to know which indexes should be rebuilt first)
2) invalid indexes (check them after mass rebuilding for your confidence)
3) unused indexes (perhaps there are unused bloated indexes that should be removed
   and needn't be rebuilt respectively)
4) indexes with the "new_" prefix
```
### Understanding of the concurrent index rebuilding:
For concurrent rebuilding of postgresql index
without a table locking needs to do the steps below:
```
1) check validity of the current index
2) get an index definition from pg_indexes;
3) get an index comment if exists;
4) realize a temporary name for the new index;
5) make a creation command using the index definition
   and the temporary index name and add the expression 'CONCURRENTLY' 
6) create a new index by the creation command
7) and add comment if it was on the old index
8) check new index validity
9) if it's valid, drop the old index
10) rename the new index like the old index
```
### Logging:

Example of event log file /tmp/pg_reindex.log entries:
```
2017/12/13 12:53:55 [INFO] == Start to rebuild index ==: my_index_name
2017/12/13 12:53:55 [INFO] Indexdef: CREATE INDEX my_index_name ON my_table USING btree (some_attr)
2017/12/13 12:53:55 [INFO] Comment of index: 'test'
2017/12/13 12:53:55 [INFO] Temporary new index name: new_my_index_name
2017/12/13 12:53:55 [INFO] Try to create new index
2017/12/13 12:53:55 [INFO] CREATE INDEX CONCURRENTLY new_my_index_name ON my_index_name USING btree (some_attr)
2017/12/13 12:53:55 [INFO] Index has been created
2017/12/13 12:53:55 [INFO] Comment has been added
2017/12/13 12:53:55 [INFO] SET statement_timeout = '10s';
2017/12/13 12:53:55 [INFO] Try to drop previous index
2017/12/13 12:53:55 [INFO] DROP INDEX my_index_name
2017/12/13 12:53:55 [INFO] Index has been dropped
2017/12/13 12:53:55 [INFO] Try to rename new index like previous
2017/12/13 12:53:55 [INFO] ALTER INDEX new_my_index_name RENAME TO my_index_name
2017/12/13 12:53:55 [INFO] Index has been renamed
2017/12/13 12:53:55 [INFO] Prev idx size: 16384, new idx size: 16383, diff: 1
2017/12/13 12:53:55 [INFO] SET statement_timeout = '0s';
2017/12/13 12:53:55 [INFO] == Rebuilding is done ==

```

### Synopsis:
```pg_reindex -d DBNAME [OPTIONS]```

**Options:**
```
  -s		Show top of bloated indexes
  -i		Show invalid indexes
  -n		Show indexes with the "new_" prefix
  -r IDXNAME	Rebuild the specified index
  -f FILENAME	Take indexname(s) from the file
		(one name per line) and rebuild them successively
  -u SIZE_THRESH
		Show not used indexes with size more than SIZE_THRESH in bytes
  -l LOGFILE	Set up the LOGFILE (/tmp/pg_reindex.log by default)
  -t SEC	Set up the statement_timeout in SEC (10 sec by default)
  -v		Print version and exit
  -h		Print this message and exit
```


**Examples:**

Show top of bloated indexes:
```
./pg_reindex -d mydbname -s
```
Show unused indexes that have size equal or larger than 1KB:
```
./pg_reindex -d mydbname -u 1024
```

Show invalid indexes:
```
./pg_reindex -d mydbname -i
```

Rebuild my_bloated_index, write information to the log.txt:
```
./pg_reindex -d mydbname -r my_bloated_index -l log.txt
```

Rebuild indexes from the file, statement_timeout is 30 seconds:
```
./pg_reindex -d mydbname -f file_with_indexnames -t 30
```
