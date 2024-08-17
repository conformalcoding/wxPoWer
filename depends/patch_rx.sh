#!/bin/bash

patch RandomX-1.2.1/src/configuration.h << EOF
@@ -32,16 +32,16 @@
 #define RANDOMX_ARGON_MEMORY       262144
 
 //Number of Argon2d iterations for Cache initialization.
-#define RANDOMX_ARGON_ITERATIONS   3
+#define RANDOMX_ARGON_ITERATIONS   2
 
 //Number of parallel lanes for Cache initialization.
 #define RANDOMX_ARGON_LANES        1
 
 //Argon2d salt
-#define RANDOMX_ARGON_SALT         "RandomX\x03"
+#define RANDOMX_ARGON_SALT         "wxPoWer\x03"
 
 //Number of random Cache accesses per Dataset item. Minimum is 2.
-#define RANDOMX_CACHE_ACCESSES     8
+#define RANDOMX_CACHE_ACCESSES     10
 
 //Target latency for SuperscalarHash (in cycles of the reference CPU).
 #define RANDOMX_SUPERSCALAR_LATENCY   170
@@ -50,10 +50,10 @@
 #define RANDOMX_DATASET_BASE_SIZE  2147483648
 
 //Dataset extra size. Must be divisible by 64.
-#define RANDOMX_DATASET_EXTRA_SIZE 33554368
+#define RANDOMX_DATASET_EXTRA_SIZE 33554304
 
 //Number of instructions in a RandomX program. Must be divisible by 8.
-#define RANDOMX_PROGRAM_SIZE       256
+#define RANDOMX_PROGRAM_SIZE       192
 
 //Number of iterations during VM execution.
 #define RANDOMX_PROGRAM_ITERATIONS 2048
EOF

exit $?
