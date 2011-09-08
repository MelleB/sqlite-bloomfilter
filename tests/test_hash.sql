
-- Load the extension
SELECT 'Loaded "libbloom" extension...' FROM (SELECT LOAD_EXTENSION('../libbloom.so'));

-- Create a db to test simple hashing
DROP TABLE IF EXISTS hash_test;
CREATE TABLE hash_test(id INTEGER PRIMARY KEY, key TEXT, hash BLOB);

-- Insert test values
INSERT INTO hash_test VALUES (1, 'a','28259CA4FDF626B025EBCA9125F82B15');
INSERT INTO hash_test VALUES (2, 'b','B6EC16D6CF02DB0F2C0FEE29FA33F9EB');
INSERT INTO hash_test VALUES (3, '0','3F11CDDF7A9764519ADC455C4E0037B3');
INSERT INTO hash_test VALUES (4, '1','78DDA9826E00F442710EEA5D640E2032');
INSERT INTO hash_test VALUES (5, 'test','BADA7A44BBD733CB721E7FC19D071277');
INSERT INTO hash_test VALUES (6, 'sqlite','CC80A5A37FEB6732EF2096CC94AA9A7F');
INSERT INTO hash_test VALUES (7, 'SQLite In 5 Minutes Or Less','8D280C535CC0B22E36CA2799BF0FFA85');
INSERT INTO hash_test VALUES (8, 'Datatypes In SQLite Version 3','431892DF6262665E73732FBED784EF86');

-- Running the actual tests
SELECT 'Test failed: ' || id || '. ' || key || ' not equal to ' || hash 
FROM hash_test WHERE murmur3(key) <> hash;

-- Summary of tests
SELECT 'Number of tests failed: ' || COUNT(*) FROM (
  SELECT 'Test failed: ' || id || '. ' || key || ' not equal to ' || hash 
  FROM hash_test WHERE murmur3(key) <> hash
);

