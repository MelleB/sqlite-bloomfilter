#!/bin/bash

echo ""

echo "Testing hash functions"
echo "======================"
sqlite3 < test_hash.sql
echo ""


echo "Testing bloomfilter functions"
echo "============================="
sqlite3 < test_bloom.sql
echo ""

