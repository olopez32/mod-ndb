<?php

#   This code illustrates how you can perform a Sort-Merge join in
#   web application code using mod_ndb and the ApacheNote result format.
#   The example here is in PHP, but the same idea can work in any language 
#   that is embedded in Apache and provides functions to perform a 
#   subrequest and to set and retreive Apache notes.


# Scan table 1 in an Apache sub-request.  
# Use an OrderedIndex scan to get a set of rows from a table.
# In httpd.conf, include an [ASC] sort flag in the OrderedIndex specifier.
#
virtual("/ndb/table1?channel=12");

# Scan table 2
virtual("/ndb/table2?name=joe");

# Now commit the transaction:
virtual("/ndb-commit-all");

# Read the results of the two queries from the Apache notes table
# and deserialize it into an array.
$table1 = json_decode(apache_note("ndb_result_0",true));
$table2 = json_decode(apache_note("ndb_result_1",true));


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