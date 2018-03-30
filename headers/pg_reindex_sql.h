#ifndef PG_REINDEX_SQL_H
#define PG_REINDEX_SQL_H

#define CHECK_REL_EXIST_SQL "SELECT c.relname FROM pg_catalog.pg_class AS c WHERE c.relname = $1::text"

#define SHOW_NEW_PREF_IDX_SQL "SELECT indexname FROM pg_indexes WHERE indexname LIKE 'new_%'"

#define GET_UNUSED_IDX_SQL "SELECT c.relname AS index_name,\
 pg_size_pretty(pg_relation_size(c.oid)) AS size, s.idx_scan AS idx_scan,\
 idx.indrelid::regclass AS table_name\
 FROM pg_index as idx\
 JOIN   pg_class as c ON c.oid = idx.indexrelid\
 LEFT JOIN pg_stat_user_indexes AS s ON c.relname = s.indexrelname\
 WHERE  s.idx_scan <= $1\
 AND pg_relation_size(c.oid) > $2\
 AND indisprimary = 'f'\
 AND    indisunique = 'f'\
 AND    c.relname not like 'pg_toast_%%'\
 ORDER BY pg_relation_size(c.oid) DESC"

#define GET_REL_KIND_SQL "SELECT c.relkind FROM pg_class AS c WHERE c.relname = $1::text"

#define GET_REL_SIZE_SQL "SELECT pg_relation_size($1::text)"

#define CHECK_IDX_VALID_SQL "SELECT i.indisvalid FROM pg_catalog.pg_index AS i\
 WHERE i.indexrelid = $1::regclass AND i.indisvalid = 'f'"

#define GET_ICOMMENT_SQL "SELECT obj_description((SELECT oid FROM pg_class WHERE relname = $1::text))"

#define GET_IDXDEF_SQL "SELECT indexdef FROM pg_indexes WHERE indexname = $1::text"

#define IDX_BLOAT_STAT_SQL "SELECT\
 row_number() over(ORDER by bs*(relpages-est_pages_ff) DESC) AS n,\
 tblname, idxname, pg_size_pretty(bs*(relpages)::bigint) AS size,\
 pg_size_pretty(bs*(relpages-est_pages_ff)::bigint) AS bloat_size,\
 (100 * (relpages-est_pages_ff)::float / relpages)::numeric(5,2) AS bloat_ratio\
 FROM (SELECT coalesce(1 + ceil(reltuples/floor((bs-pageopqdata-pagehdr)/(4+nulldatahdrwidth)::float)), 0) AS est_pages,\
 coalesce(1 + ceil(reltuples/floor((bs-pageopqdata-pagehdr)*fillfactor/(100*(4+nulldatahdrwidth)::float))), 0)\
 AS est_pages_ff, bs, nspname, table_oid, tblname, idxname, relpages, fillfactor, is_na\
 FROM (SELECT maxalign, bs, nspname, tblname, idxname, reltuples, relpages, relam, table_oid, fillfactor,\
 (index_tuple_hdr_bm + maxalign -\
 CASE WHEN index_tuple_hdr_bm%maxalign = 0\
 THEN maxalign ELSE index_tuple_hdr_bm%maxalign END + nulldatawidth + maxalign -\
 CASE WHEN nulldatawidth = 0 THEN 0 WHEN nulldatawidth::integer%maxalign = 0\
 THEN maxalign ELSE nulldatawidth::integer%maxalign END)::numeric AS nulldatahdrwidth, pagehdr, pageopqdata, is_na\
 FROM (SELECT i.nspname, i.tblname, i.idxname, i.reltuples, i.relpages, i.relam, a.attrelid AS table_oid,\
 current_setting('block_size')::numeric AS bs, fillfactor,\
 CASE WHEN version() ~ 'mingw32' OR version() ~ '64-bit|x86_64|ppc64|ia64|amd64'\
 THEN 8 ELSE 4 END AS maxalign, 24 AS pagehdr, 16 AS pageopqdata,\
 CASE WHEN max(coalesce(s.null_frac,0)) = 0 THEN 2 ELSE 2 + (( 32 + 8 - 1 ) / 8)\
 END AS index_tuple_hdr_bm, sum((1-coalesce(s.null_frac, 0)) * coalesce(s.avg_width, 1024)) AS nulldatawidth,\
 max(CASE WHEN a.atttypid = 'pg_catalog.name'::regtype THEN 1 ELSE 0 END) > 0 AS is_na\
 FROM pg_attribute AS a JOIN\
 (SELECT nspname, tbl.relname AS tblname, idx.relname AS idxname, idx.reltuples, idx.relpages,\
 idx.relam, indrelid, indexrelid, indkey::smallint[] AS attnum,\
 coalesce(substring(array_to_string(idx.reloptions, ' ')\
 FROM 'fillfactor=([0-9]+)')::smallint, 90) AS fillfactor\
 FROM pg_index JOIN pg_class idx ON idx.oid=pg_index.indexrelid\
 JOIN pg_class tbl ON tbl.oid=pg_index.indrelid\
 JOIN pg_namespace ON pg_namespace.oid = idx.relnamespace\
 WHERE pg_index.indisvalid AND pg_index.indisunique = 'f'\
 AND pg_index.indisprimary = 'f' AND tbl.relkind = 'r' AND idx.relpages > 0) AS i\
 ON a.attrelid = i.indexrelid JOIN pg_stats AS s ON s.schemaname = i.nspname\
 AND ((s.tablename = i.tblname AND s.attname = pg_catalog.pg_get_indexdef(a.attrelid, a.attnum, TRUE))\
 OR (s.tablename = i.idxname AND s.attname = a.attname))\
 JOIN pg_type AS t ON a.atttypid = t.oid WHERE a.attnum > 0\
 GROUP BY 1, 2, 3, 4, 5, 6, 7, 8, 9) AS s1) AS s2\
 JOIN pg_am am ON s2.relam = am.oid WHERE am.amname = 'btree') AS sub\
 WHERE nspname = 'public' AND bs*(relpages-est_pages_ff) > 1048576 LIMIT 50"

#define GET_INV_IDX_SQL "SELECT c.relname AS index_name FROM pg_catalog.pg_class AS c\
 JOIN pg_catalog.pg_index AS i ON c.oid = i.indexrelid AND indisvalid = 'f'"

#endif
