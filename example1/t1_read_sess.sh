#!/bin/sh

. ./base.test

# Select all of the name/value pairs for session 3

curl "$BASE_URL/ndb1/session/3"
