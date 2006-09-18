<html>
<h2> Virtual test </h2>

<?php

apache_note("preset","set this note in advance.");

$req = "/ndb/as-note?channel=12";

if(virtual($req))
	echo "virtual() success.<br>\n";
else echo "virtual() failure.<br>\n";

echo "<p>Subrequest: $req <br>";
echo "NDB Results: " . apache_note("NDB_results") . "</br>\n";

$req = "/ndb/one-row-note/1";
echo "<P>Subrequest: $req <br>\n";
virtual($req);  // bug here!  the subrequest sends a bunch of garbage to the browser!
if(! apache_note("id")) 
	echo "cannot fetch id note. <br>\n";
echo "id: " . apache_note("id") . "<br>\n";
echo "name: " . apache_note("name") . "<br>\n";
echo "channel: " . apache_note("channel") . "<br>\n";
echo "preset: " . apache_note("preset") . "<br>\n";

echo "</html>\n";

?>






