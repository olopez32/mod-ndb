<html>
<h2> Virtual test </h2>

<?php

virtual("/ndb/ex/1/session/3");
virtual("/ndb/ex/1/session/1");
# apache_note("ndb_send_result",1);
virtual("/ndb-commit-all");


echo "ndb_result_0: " . apache_note("ndb_result_0") . "</br>\n";
echo "ndb_result_1: " . apache_note("ndb_result_1") . "</br>\n";

$x = json_decode(apache_note("ndb_result_1",true));

var_dump($x);

# echo "</html>\n";

?>






