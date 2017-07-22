/* -*- c++ -*- */

#define WIMAX_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "wimax_swig_doc.i"

%{
#include "wimax/my_send_cb.h"
%}


%include "wimax/my_send_cb.h"
GR_SWIG_BLOCK_MAGIC2(wimax, my_send_cb);
