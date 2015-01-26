# tdcliviipy
Simple Python module for access teradata database by CLIv2.

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
        try:
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
        finally:
            if "qry" in locals():
                del qry
        print "Query record count: %d." % count

        print "Logoff.\n"
except CliFail as ex:
    print "%s" % ex
finally:
    if "con" in locals():
        del con
```

