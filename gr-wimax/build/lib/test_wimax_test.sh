#!/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir=/home/atexide/gr-wimax/gr-wimax/lib
export GR_CONF_CONTROLPORT_ON=False
export PATH=/home/atexide/gr-wimax/gr-wimax/build/lib:$PATH
export LD_LIBRARY_PATH=/home/atexide/gr-wimax/gr-wimax/build/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$PYTHONPATH
test-wimax 
