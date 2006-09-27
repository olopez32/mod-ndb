#!/bin/sh

#  Display the session table

ndb_select_all -d mod_ndb_test session
