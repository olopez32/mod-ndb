<?php

virtual("/ndb/test/sesx/3");
virtual("/ndb/test/sesx/1");
virtual("/ndb-commit-all");

echo "ndb_result_0: " . apache_note("ndb_result_0") . "</br>\n";
echo "ndb_result_1: " . apache_note("ndb_result_1") . "</br>\n\n";

$x = json_decode(apache_note("ndb_result_1",true));

var_dump($x);
?>

