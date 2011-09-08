
Bloom filter extension for SQLite
=================================

Description
-----------
This is a simple [bloom filter][0] extension for [SQLite][1]. 
Internally it uses 'buckets' of 64-bit unsigned integers to 
store the bloomfilter. It uses the [Murmur3 hashing function][2]
to create a hash.


Installation
------------
Optional: Get the files from github  
```git clone ...```

Initialize the murmur3 submodule. This is a port of [murmur3][2] to C.
Please see [Peter Scott's repo][2] for details  
```git submodule init && git submodule update```

Compile the module  
```make module```

To play around with murmur3 hashes  
```make murmur_check```


Usage
-----
First, the extension needs to be loaded using  
```SELECT LOAD_EXTENSION('./libbloom.so');```

After that the following functions are provided:

* __murmur3([string])__  
  Calculates the murmur3 hash of a given string.
* __bloomfilter([field])__  
  Create a bloomfilter for a given field.  
  E.g. ```SELECT bloomfilter(name) FROM persons WHERE age < 100;```  
  This function returns a bloom filter as a long hexadecimal string.
* __in_bloom([value], [bloomfilter])__    
  Test whether the value is in the bloomfilter, returns 1 (true) or 
  0 (false).


Limitations and bugs
--------------------
There are probably more bugs and limitations than I can think of, if you 
come accross any, please let me know.

Limitations:

* Size of bloomfilter is determined at in the ```SQLITE_BLOOM_DEFAULT_SIZE```
  definition.


License
-------
This code is released under the BSD3 license. For a copy of the license
please see the LICENSE file.


[0]: http://en.wikipedia.org/wiki/Bloom_filter
[1]: http://www.sqlite.org
[2]: http://code.google.com/p/smhasher/
[3]: https://github.com/PeterScott/murmur3

