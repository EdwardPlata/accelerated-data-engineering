#!/bin/bash

echo "Testing SimpleDB..."

cd /workspaces/accelerated-data-engineering/examples/database/build

# Test basic functionality
echo "CREATE TABLE test (id int, name string)" | ./simple_db
echo "INSERT INTO test VALUES (1, hello)" | ./simple_db  
echo "SELECT * FROM test" | ./simple_db

echo "Test completed!"
