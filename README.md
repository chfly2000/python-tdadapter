# Description

tdcliviipy is a Python extension library which allows to connect to Teradata RDBMS via CLIv2 (Teradata native access interface).

# Requirements

    CLIv2 (32bit / 64bit)
    C compiler

# Installation

    setup.py install build --compile=mingw32

# Usage

```python
from clivii import connect, CliFail

try:
    with connect("hostname/username,password") as con:
        print "Logon."

        stmt = "update db.table1 set col1 = 'value1' where col2 = 'value2';"

        print "Execute update:\n%s" % stmt
        count = con.update(stmt)
        print "Update record count: %d." % count

        count = 0
        stmt = "select * from dbc.tables where databasename='db1' and tablename = 'table1';"

        print "Execute query:\n%s" % stmt
        with con.query(stmt) as qry:
            print "(",
            for col in qry.cols:
                print unicode(col), ",",
            print ")"
            for row in qry:
                count += 1
                print "(",
                for fld in row:
                    print unicode(fld), ",",
                print ")"
        print "Query record count: %d." % count

        print "Logoff."
except CliFail as ex:
    print "%s" % ex
```
