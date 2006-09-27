#!/bin/sh

. ./base.test

#  Insert a new row of session data

curl -d 'sess_id=4' -d 'sess_var_name=user_name' -d 'sess_var_value=roger' \
 "$BASE_URL/ndb1/new_session"
