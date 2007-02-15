#!/bin/sh

. ./base.test

# Select all of the name/value pairs for session 3

curl -i "$BASE_URL/session/3"
