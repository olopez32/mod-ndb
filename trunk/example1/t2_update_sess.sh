#!/bin/sh

. ./base.test

# Update the "last_page_visited" key for session 3

curl -i -d 'sess_var_value=maps.php'  \
 "$BASE_URL/session?id=3&var=last_page_visited"
