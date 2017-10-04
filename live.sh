#!/bin/sh

#
# run openEAR live emotion recognition console demo
#

export LD_LIBRARY_PATH="linux_bin/"
./linux_bin/SMILExtract_pa.linux_generic_x86_libc6 -C config/emobase_live4.conf

# to list audio devices available use this line:
# ./SMILExtract -C config/emobase_live4.conf -devlist 1
# to change the default audio device use this line:
# ./SMILExtract -C config/emobase_live4.conf -D <id>

