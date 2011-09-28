/*
    sdb.c -- SQLite Database (SDB)

    WARNING: This is prototype code

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#include    "appweb.h"
#include    "edi.h"

#if BLD_FEATURE_ESP && BLD_FEATURE_SQLITE && 0
#include    "sqlite3.h"

/************************************* Local **********************************/
/*
    Map allocation and mutex routines to use ejscript version.
 */
#define MAP_ALLOC   1
#define MAP_MUTEXES 0

#define THREAD_STYLE SQLITE_CONFIG_MULTITHREAD

typedef struct Sdb {
    Edi             edi;            /**< */
    sqlite3         *db;
    char            *path;          /**< Currently open database */
} Sdb;

static int sqliteInitialized;
static void initSqlite();
static EdiField makeRecField(cchar *value, cchar *name, int type)

    //  MOB - HERE
static var DataTypeToSqlType: Object = {
    "binary":       "blob",
    "boolean":      "tinyint",
    "date":         "date",
    "datetime":     "datetime",
    "decimal":      "decimal",
    "float":        "float",
    "integer":      "int",
    "number":       "decimal",
    "string":       "varchar",
    "text":         "text",
    "time":         "time",
    "timestamp":    "datetime",
};

/************************************ Forwards ********************************/

static int sdbAddColumn(Sdb *sdb, cchar *tableName, cchar *columnName, int type, int flags);
static int sdbAddIndex(Sdb *sdb, cchar *tableName, cchar *columnName, cchar *indexName);
static int sdbAddTable(Sdb *sdb, cchar *tableName);
static int sdbChangeColumn(Sdb *sdb, cchar *tableName, cchar *columnName, int type, int flags);
static void sdbClose(Sdb *sdb);
static int sdbDelete(Sdb *sdb, cchar *path);
static int sdbDeleteRec(Sdb *sdb, cchar *tableName, cchar *key);
static MprList *sdbGetColumns(Sdb *sdb, cchar *tableName);
static EdiField sdbGetField(Sdb *sdb, cchar *tableName, cchar *key, cchar *fieldName);
static EdiGrid *sdbGetGrid(Sdb *sdb, cchar *tableName);
static EdiRec *sdbGetRec(Sdb *sdb, cchar *tableName, cchar *key);
static int sdbGetSchema(Sdb *sdb, EdiRec *rec, int fid, cchar **name, int *type, int *flags);
static MprList *sdbGetTables(Sdb *sdb);
static int sdbLoad(Sdb *sdb, cchar *path);
static int sdbLoadString(Sdb *sdb, cchar *string);
static int sdbLookupField(Sdb *sdb, EdiRec *rec, cchar *fieldName);
static Sdb *sdbOpen(cchar *path, int flags);
static EdiGrid *sdbQuery(Sdb *sdb, cchar *cmd);
static int sdbRemoveColumn(Sdb *sdb, cchar *tableName, cchar *columnName);
static int sdbRemoveIndex(Sdb *sdb, cchar *tableName, cchar *indexName);
static int sdbRemoveTable(Sdb *sdb, cchar *tableName);
static int sdbRenameTable(Sdb *sdb, cchar *tableName, cchar *newTableName);
static int sdbRenameColumn(Sdb *sdb, cchar *tableName, cchar *columnName, cchar *newColumnName);
static int sdbSetField(Sdb *sdb, cchar *tableName, cchar *key, cchar *fieldName, cchar *value, int flags);
static int sdbSetRec(Sdb *sdb, cchar *tableName, cchar *key, MprHashTable *params);
static int sdbSave(Sdb *sdb);

EdiProvider SdbProvider = {
    "sdb",
    sdbAddColumn,
    sdbAddIndex,
    sdbAddTable,
    sdbChangeColumn,
    sdbClose,
    sdbDelete,
    sdbDeleteRec,
    sdbGetColumns,
    sdbGetField,
    sdbGetGrid,
    sdbGetRec,
    sdbGetSchema,
    sdbGetTables,
    sdbLoad,
    sdbLookupField,
    sdbOpen,
    sdbQuery,
    sdbRemoveColumn,
    sdbRemoveIndex,
    sdbRemoveTable,
    sdbRenameTable,
    sdbRenameColumn,
    sdbSetField,
    sdbSetRec,
    sdbSave,
};

/************************************* Code ***********************************/

void sdbInit()
{
    ediAddProvider(&SdbProvider);
}


static Sdb *sdbCreate(cchar *path, int flags)
{
    Sdb      *sdb;

    initSqlite();
    if ((sdb = mprAllocObj(Sdb, manageSdb)) == 0) {
        return 0;
    }
    sdb->edi.flags = flags;
    sdb->edi.provider = &SdbProvider;
    sdb->path = sclone(path);
    return sdb;
}


static void manageSdb(Sdb *sdb, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(sdb->path);
    } else if (flags & MPR_MANAGE_FREE) {
        sdbClose(sdb);
    }
}


static void sdbClose(Sdb *sdb)
{
    if (sdb->db) {
        sqlite3_close(sdb->db);
        db->sdb = 0;
    }
    sdb->path = 0;
}


static int sdbDelete(Sdb *sdb, cchar *path)
{
    return mprDeletePath(path);
}


static Sdb *sdbOpen(cchar *path, int flags)
{
    Sdb     *sdb;

    if ((sdb = sdbCreate(path, flags)) == 0) {
        return 0;
    }
    if (mprPathExists(path, R_OK) || (flags & EDI_CREATE)) {
        if (sqlite3_open(path, &sdb->db) != SQLITE_OK) {
            mprError("Can't open database %s", path);
            return 0;
        }
        //  MOB - should be configurable somewhere
        sqlite3_soft_heap_limit(20 * 1024 * 1024);
        sqlite3_busy_timeout(sdb, EJS_SQLITE_TIMEOUT);
        if (sdbLoad(sdb, path) < 0) {
            return 0;
        }
    } else {
        return 0;
    }
    return sdb;
}



static int sdbAddColumn(Sdb *sdb, cchar *tableName, cchar *columnName, int type, int flags)
{
    //  MOB OPT - have option to run command without gathering a grid result
    query(sdb, "ALTER TABLE %s ADD %s %s",  tableName, columnName, mapped);
    return 0;
}


//  IndexName is ignored
static int sdbAddIndex(Sdb *sdb, cchar *tableName, cchar *columnName, cchar *indexName)
{
    SdbTable    *table;
    SdbCol      *col;

    if ((table = lookupTable(sdb, tableName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupColumn(table, columnName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    table->index = mprCreateHash(0, 0);
    table->indexCol = col;
    col->flags |= EDI_INDEX;
    autoSave(sdb, table);
    return 0;
}


static int sdbAddTable(Sdb *sdb, cchar *tableName)
{
    SdbTable     *table;

    if ((table = mprAllocObj(SdbTable, manageTable)) == 0) {
        return 0;
    }
    table->name = sclone(tableName);
    table->nextId = 1;
    if (sdb->tables == 0) {
        sdb->tables = mprCreateList(0, 0);
    }
    mprAddItem(sdb->tables, table);
    autoSave(sdb, lookupTable(sdb, tableName));
    return 0;
}


static int sdbChangeColumn(Sdb *sdb, cchar *tableName, cchar *columnName, int type, int flags)
{
    SdbTable    *table;
    SdbCol      *col;

    if ((table = lookupTable(sdb, tableName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupColumn(table, columnName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    col->name = sclone(columnName);
    col->type = type;
    autoSave(sdb, table);
    return 0;
}


static int sdbDeleteRec(Sdb *sdb, cchar *tableName, cchar *key)
{
    SdbTable    *table;
    int         r;

    if ((table = lookupTable(sdb, tableName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    if ((r = lookupRow(table, key)) < 0) {
        return MPR_ERR_CANT_FIND;
    }
    return mprRemoveItemAtPos(table->rows, r);
}


static MprList *sdbGetColumns(Sdb *sdb, cchar *tableName)
{
    SdbTable    *table;
    SdbSchema   *schema;
    MprList     *list;
    int         i;

    if ((table = lookupTable(sdb, tableName)) == 0) {
        return 0;
    }
    schema = table->schema;
    list = mprCreateList(schema->ncols, 0);
    for (i = 0; i < schema->ncols; i++) {
        mprAddItem(list, &schema->cols[i]);
    }
    return list;
}


static EdiField makeRecField(SdbRow *row, SdbCol *col)
{
    EdiField    f;

    f.valid = 1;
    f.value = row->fields[col->cid];
    f.type = col->type;
    f.name = col->name;
    f.flags = 0;
    return f;
}


static EdiField sdbGetField(Sdb *sdb, cchar *tableName, cchar *key, cchar *fieldName)
{
    SdbTable    *table;
    SdbCol      *col;
    SdbRow      *row;
    EdiField    err;
    int         r;

    err.valid = 0;
    if ((table = lookupTable(sdb, tableName)) == 0) {
        return err;
    }
    if ((col = lookupColumn(table, fieldName)) == 0) {
        return err;
    }
    if ((r = lookupRow(table, key)) < 0) {
        return err;
    }
    row = mprGetItem(table->rows, r);
    return makeRecField(row, col);
}


static EdiGrid *sdbGetGrid(Sdb *sdb, cchar *tableName)
{
    EdiGrid     *grid;
    SdbTable    *table;
    SdbRow      *row;
    int         nrows, next;

    if ((table = lookupTable(sdb, tableName)) == 0) {
        return 0;
    }
    nrows = mprGetListLength(table->rows);
    if ((grid = ediCreateGrid(nrows)) == 0) {
        return 0;
    }
    for (ITERATE_ITEMS(table->rows, row, next)) {
        grid->records[next - 1] = createRec(row);
    }
    return grid;
}


static EdiRec *sdbGetRec(Sdb *sdb, cchar *tableName, cchar *key)
{
    SdbTable    *table;
    SdbRow      *row;
    EdiRec      *rec;
    int         r, nrows;

    if ((table = lookupTable(sdb, tableName)) == 0) {
        return 0;
    }
    nrows = mprGetListLength(table->rows);
    if ((r = lookupRow(table, key)) < 0) {
        return 0;
    }
    row = mprGetItem(table->rows, r);
    if ((rec = createRec(row)) == 0) {
        return 0;
    }
    return rec;
}


static int sdbGetSchema(Sdb *sdb, EdiRec *rec, int fid, cchar **name, int *type, int *flags)
{
    SdbTable    *table;
    SdbCol      *col;

    if ((table = lookupTable(sdb, rec->tableName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    if ((col = getCol(table, fid)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    if (name) {
        *name = col->name;
    }
    if (type) {
        *type = col->type;
    }
    if (flags) {
        *flags = col->flags;
    }
    return 0;
}


static MprList *sdbGetTables(Sdb *sdb)
{
    MprList     *list;
    SdbTable     *table;
    int         tid, ntables;

    list = mprCreateList(-1, 0);
    ntables = mprGetListLength(sdb->tables);
    for (tid = 0; tid < ntables; tid++) {
        table = mprGetItem(sdb->tables, tid);
        mprAddItem(list, table->name);
    }
    return list;
}


static int sdbLoad(Sdb *sdb, cchar *path)
{
    cchar       *data;
    ssize       len;

    if ((data = mprReadPath(path, &len)) == 0) {
        return MPR_ERR_CANT_READ;
    }
    return sdbLoadString(sdb, data);
}


static int sdbLookupField(Sdb *sdb, EdiRec *rec, cchar *fieldName)
{
    SdbTable    *table;
    SdbCol      *col;

    if ((table = lookupTable(sdb, rec->tableName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupColumn(table, fieldName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    return col->cid;
}


static EdiGrid *sdbQuery(Sdb *sdb, cchar *cmd)
{
    sqlite3         *db;
    sqlite3_stmt    *stmt;
    EdiRec          *rec;
    MprList         *result;
    char            *tableName;
    cchar           *tail, *colName, *value, *defaultTableName;
    int             i, ncol, rc, retries, rowNum, len;

    mprAssert(db);

    retries = 0;
    if ((db = sdb->db) == 0) {
        mprError("Database '%s' is closed", sdb->path);
        return 0;
    }
    if ((result = mprCreateList(0, 0)) == 0) {
        return 0;
    }
    rc = SQLITE_OK;
    while (cmd && *cmd && (rc == SQLITE_OK || (rc == SQLITE_SCHEMA && ++retries < 2))) {
        stmt = 0;
        rc = sqlite3_prepare_v2(db, cmd, -1, &stmt, &tail);
        if (rc != SQLITE_OK) {
            continue;
        }
        if (stmt == 0) {
            /* Comment or white space */
            cmd = tail;
            continue;
        }
        defaultTableName = 0;
        ncol = sqlite3_column_count(stmt);
        for (r = 0; ; r++) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                tableName = (char*) sqlite3_column_table_name(stmt, i);
                if (defaultTableName == 0) {
                    defaultTableName = tableName;
                }
                if ((rec = createRec(edi, tableName, id, ncol)) == 0) {
                    sqlite3_finalize(stmt);
                    return 0;
                }
                mprAddItem(result, rec);
                for (i = 0; i < ncol; i++) {
                    colName = sqlite3_column_name(stmt, i);
                    value = (cchar*) sqlite3_column_text(stmt, i);
                    if (tableName && strcmp(tableName, defaultTableName) != 0) {
                        /*
                            Append the table name for columns from foreign tables. Convert to camel case (tableColumn)
                            Prefix with "_". ie. "_TableColumn"
                            MOB - remove singularization.
                         */
                        len = strlen(tableName) + 1;
                        tableName = sjoin("_", tableName, colName, NULL);
#if UNUSED
                        if (len > 3 && tableName[len - 1] == 's' && tableName[len - 2] == 'e' && tableName[len - 3] == 'i') {
                            tableName[len - 3] = 'y';
                            strcpy(&tableName[len - 2], colName);
                            len -= 2;
                        } else if (len > 2 && tableName[len - 1] == 's' && tableName[len - 2] == 'e') {
                            strcpy(&tableName[len - 2], colName);
                            len -= 2;
                        } else if (tableName[len - 1] == 's') {
                            strcpy(&tableName[len - 1], colName);
                            len--;
                        }
#endif
                        tableName[len] = toupper((int) tableName[len]);
                    }
                    //  MOB - need type?
                    rec->field[i] = makeRecField(value, colName, 0);
                }
            } else {
                rc = sqlite3_finalize(stmt);
                stmt = 0;
                if (rc != SQLITE_SCHEMA) {
                    retries = 0;
                    for (cmd = tail; isspace((int) *cmd); cmd++) {
                        ;
                    }
                }
                break;
            }
        }
    }
    if (stmt) {
        rc = sqlite3_finalize(stmt);
    }
    if (rc != SQLITE_OK) {
        if (rc == sqlite3_errcode(db)) {
            ejsThrowIOError(ejs, "SQL error: %s", sqlite3_errmsg(db));
        } else {
            ejsThrowIOError(ejs, "Unspecified SQL error");
        }
        return 0;
    }
    for (r = 0; r < nrows; r++) {
        grid->records[r] = mprGetItem(result, r);
    }
    return grid;
}


static int sdbRemoveColumn(Sdb *sdb, cchar *tableName, cchar *columnName)
{
    SdbTable    *table;
    SdbSchema   *schema;
    SdbCol      *col;
    int         c;

    if ((table = lookupTable(sdb, tableName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupColumn(table, columnName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    if (table->indexCol == col) {
        table->index = 0;
        table->indexCol = 0;
    }
    schema = table->schema;
    for (c = col->cid; c < schema->ncols; c++) {
        schema->cols[c] = schema->cols[c + 1];
    }
    schema->ncols--;
    schema->cols[schema->ncols].name = 0;
    mprAssert(schema->ncols >= 0);
    autoSave(sdb, table);
    return 0;
}


//  MOB - indexName is ignored
static int sdbRemoveIndex(Sdb *sdb, cchar *tableName, cchar *indexName)
{
    SdbTable    *table;

    if ((table = lookupTable(sdb, tableName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    table->index = 0;
    if (table->indexCol) {
        table->indexCol->flags &= ~EDI_INDEX;
        table->indexCol = 0;
        autoSave(sdb, table);
    }
    return 0;
}


static int sdbRemoveTable(Sdb *sdb, cchar *tableName)
{
    SdbTable    *table;
    int         next;

    for (ITERATE_ITEMS(sdb->tables, table, next)) {
        if (smatch(table->name, tableName)) {
            mprRemoveItem(sdb->tables, table);
            autoSave(sdb, table);
            break;
        }
    }
    return MPR_ERR_CANT_FIND;;
}


static int sdbRenameTable(Sdb *sdb, cchar *tableName, cchar *newTableName)
{
    SdbTable    *table;

    if ((table = lookupTable(sdb, tableName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    table->name = sclone(newTableName);
    autoSave(sdb, table);
    return 0;
}


static int sdbRenameColumn(Sdb *sdb, cchar *tableName, cchar *columnName, cchar *newColumnName)
{
    SdbTable    *table;
    SdbCol      *col;

    if ((table = lookupTable(sdb, tableName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupColumn(table, columnName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    col->name = sclone(newColumnName);
    autoSave(sdb, table);
    return 0;
}


static int sdbSetField(Sdb *sdb, cchar *tableName, cchar *key, cchar *fieldName, cchar *value, int flags)
{
    SdbTable    *table;
    SdbRow      *row;
    SdbCol      *col;
    MprHash     *hp;
    int         r;

    if ((table = lookupTable(sdb, tableName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    if ((r = lookupRow(table, key)) < 0) {
        row = createRow(table);
    }
    if ((row = getRow(table, r)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupColumn(table, fieldName)) != 0) {
        setField(row, col->cid, ediParseValue(hp->data, col->cid));
    }
    return 0;
}


static int sdbSetRec(Sdb *sdb, cchar *tableName, cchar *key, MprHashTable *params)
{
    SdbTable    *table;
    SdbRow      *row;
    SdbCol      *col;
    MprHash     *hp;
    int         r;

    if ((table = lookupTable(sdb, tableName)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    if ((r = lookupRow(table, key)) < 0) {
        row = createRow(table);
    }
    if ((row = getRow(table, r)) == 0) {
        return MPR_ERR_CANT_FIND;
    }
    for (ITERATE_KEYS(params, hp)) {
        if ((col = lookupColumn(table, hp->key)) != 0) {
            setField(row, col->cid, ediParseValue(hp->data, col->cid));
        }
    }
    return 0;
}


/*********************************** Support *******************************/
/*
    Optimized record creation
 */
EdiRec *createRec(int nfields)
{
    EdiRec  *rec;
    MdbCol  *col;
    int     c;

    if ((rec = mprAllocMem(sizeof(EdiRec) + sizeof(EdiField) * nfields, MPR_ALLOC_MANAGER | MPR_ALLOC_ZERO)) == 0) {
        return 0;
    }
    mprSetAllocName(rec, "record");
    mprSetManager(rec, ediManageEdiRec);

    rec->tableName = row->table->name;
    //  MOB - assert and check there is a valid ID
    rec->id = row->fields[0].str;
    rec->nfields = row->nfields;
    for (c = 0; c < row->nfields; c++) {
        col = getCol(row->table, c);
        rec->fields[c] = makeRecField(row, col);
    }
    memmove(rec->fields, row->fields, sizeof(EdiField) * row->nfields);
    return rec;
}


static EdiField makeRecField(cchar *value, cchar *name, int type)
{
    EdiField    field;

    field.valid = 1;
    field.type = 0;
    field.flags = 0;
    field.value.str = sclone(value);
    field.name = sclone(name);
}

/*********************************** Alloc *********************************/
#if MAP_ALLOC
/*
    Map memory allocations to use MPR
 */
static void *allocBlock(int size)
{
    void    *ptr;

    if ((ptr = mprAlloc(size)) != 0) {
        mprHold(ptr);
    }
    return ptr;
}


static void freeBlock(void *ptr)
{
    mprRelease(ptr);
}


static void *reallocBlock(void *ptr, int size)
{
    mprRelease(ptr);
    if ((ptr =  mprRealloc(ptr, size)) != 0) {
        mprHold(ptr);
    }
    return ptr;
}


static int blockSize(void *ptr)
{
    return mprGetBlockSize(ptr);
}


static int roundBlockSize(int size)
{
    return MPR_ALLOC_ALIGN(size);
}


static int initAllocator(void *data)
{
    return 0;
}


static void termAllocator(void *data)
{
}


struct sqlite3_mem_methods mem = {
    allocBlock, freeBlock, reallocBlock, blockSize, roundBlockSize, initAllocator, termAllocator, NULL 
};

#endif /* MAP_ALLOC */

/*********************************** Mutex ********************************/
#if MAP_MUTEXES
/*
    Map mutexes to use MPR
 */

int mutc = 0;

static int initMutex(void) { 
    return 0; 
}


static int termMutex(void) { 
    return 0; 
}


//  MOB - incomplete must handle kind
static sqlite3_mutex *allocMutex(int kind)
{
    MprMutex    *lock;

    if ((lock = mprCreateLock()) != 0) {
        mprHold(lock);
        mutc++;
    }
    return (sqlite3_mutex*) lock;
}


static void freeMutex(sqlite3_mutex *mutex)
{
    mutc--;
    mprRelease((MprMutex*) mutex);
}


static void enterMutex(sqlite3_mutex *mutex)
{
    mprLock((MprMutex*) mutex);
}


static int tryMutex(sqlite3_mutex *mutex)
{
    return mprTryLock((MprMutex*) mutex);
}


static void leaveMutex(sqlite3_mutex *mutex)
{
    mprUnlock((MprMutex*) mutex);
}


static int mutexIsHeld(sqlite3_mutex *mutex) { 
    mprAssert(0); 
    return 0; 
}


static int mutexIsNotHeld(sqlite3_mutex *mutex) { 
    mprAssert(0); 
    return 0; 
}


struct sqlite3_mutex_methods mut = {
    initMutex, termMutex, allocMutex, freeMutex, enterMutex, tryMutex, leaveMutex, mutexIsHeld, mutexIsNotHeld,
};

#endif /* MAP_MUTEXES */

/*********************************** Factory *******************************/

static void initSqlite()
{
    //  MOB - locking
    if (!sqliteInitialized) {
#if MAP_ALLOC
        sqlite3_config(SQLITE_CONFIG_MALLOC, &mem);
#endif
#if MAP_MUTEXES
        sqlite3_config(SQLITE_CONFIG_MUTEX, &mut);
#endif
        sqlite3_config(THREAD_STYLE);
        if (sqlite3_initialize() != SQLITE_OK) {
            mprError("Can't initialize SQLite");
            return;
        }
        sqliteInitialized = 1;
    }
}

#endif /* BLD_FEATURE_ESP && BLD_FEATURE_SQLITE */

/*
    @copy   default
    
    Copyright (c) Embedthis Software LLC, 2003-2011. All Rights Reserved.
    Copyright (c) Michael O'Brien, 1993-2011. All Rights Reserved.
    
    This software is distributed under commercial and open source licenses.
    You may use the GPL open source license described below or you may acquire 
    a commercial license from Embedthis Software. You agree to be fully bound 
    by the terms of either license. Consult the LICENSE.TXT distributed with 
    this software for full details.
    
    This software is open source; you can redistribute it and/or modify it 
    under the terms of the GNU General Public License as published by the 
    Free Software Foundation; either version 2 of the License, or (at your 
    option) any later version. See the GNU General Public License for more 
    details at: http://embedthis.com/downloads/gplLicense.html
    
    This program is distributed WITHOUT ANY WARRANTY; without even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
    
    This GPL license does NOT permit incorporating this software into 
    proprietary programs. If you are unable to comply with the GPL, you must
    acquire a commercial license to use this software. Commercial licenses 
    for this software and support services are available from Embedthis 
    Software at http://embedthis.com 
    
    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
