#!/bin/sh

. ./base.test

# Update the "last_page_visited" key for session 3

curl -i -d 'sess_var_value=maps.php'  \
 "$BASE_URL/ndb1/session?sess_id=3&sess_var_name=last_page_visited"
