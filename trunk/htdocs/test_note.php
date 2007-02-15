<html>
<h2> Virtual test </h2>

<?php

$req = "/ndb/table1?channel=12";

if(virtual($req))
	echo "virtual() success.<br>\n";
else echo "virtual() failure.<br>\n";

echo "<p>Subrequest: $req <br>";
echo "NDB Results: " . apache_note("ndb_result_0") . "</br>\n";

echo "</html>\n";

?>






