#!/bin/sh

. ./base.test

# Delete a row of data 

curl -i -X DELETE "$BASE_URL/session?id=4&var=user_name"
