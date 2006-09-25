#!/bin/sh

. ./base.test

curl -d 'sess_var_value=maps.php'  \
 "$BASE_URL/ndb1/session?sess_id=3&sess_var_name=last_page_visited"
