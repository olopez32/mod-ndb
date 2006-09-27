#!/bin/sh

. ./base.test

# Delete a row of data 

curl -X DELETE "$BASE_URL/ndb1/session?sess_id=4&sess_var_name=user_name"
