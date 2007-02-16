<?php

#   This code illustrates how you can perform a Sort-Merge join in
#   web application code using mod_ndb and the ApacheNote result format.
#   The example here is in PHP, but the same idea can work in any language 
#   that is embedded in Apache and provides functions to perform a 
#   subrequest and to set and retreive Apache notes.

# Before each query except the final one, set ndb_keep_tx_open.  This makes
# all queries execute inside a single consistent transaction.
apache_note("ndb_keep_tx_open",1);

# Scan table 1 in an Apache sub-request, and get the result set back in an
# Apache note.  Use an OrderedIndex scan to get a set of rows from a table.
# In httpd.conf, specify Format ApacheNote, and put an [ASC] sort flag at 
# the end of the OrderedIndex specifier.
#
virtual("/ndb/as-note/table1?channel=12");
$results = apache_note("NDB_results");
$table1 = json_decode($results,true);

# Scan table 2: (Same as table1).
#
virtual("/ndb/as-note/table2?name=joe");
$results = apache_note("NDB_results");
$table2 = json_decode($results,true);

# Now we have two sorted arrays and can merge them into a result set.
#
$t1_row = reset($table1);   # PHP's reset() returns the first row.
$t2_row = reset($table2);

while($t1_row && $t2_row) {
  $match = strcmp($t1_row['join_key'], $t2_row['join_key']);
  
  if($match < 0)
    $t1_row = next($table1);
  else if($match > 0)
    $t2_row = next($table2);
  else {
    $result_set[] = array_merge($t1_row,$t2_row);
    $t1_row = next($table1);
    $t2_row = next($table2);
  }
}
print_r($result_set);

?>