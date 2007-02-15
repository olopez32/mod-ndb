#!/bin/sh

. ./base.test

# Delete a row of data 

curl -i -X DELETE "$BASE_URL/session?sess_id=4&sess_var_name=user_name"
