/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "my_send_cb_impl.h"

namespace gr {
  namespace wimax {

    my_send_cb::sptr
    my_send_cb::make(bool gray_code)
    {
      return gnuradio::get_initial_sptr
        (new my_send_cb_impl(gray_code));
    }

    /*
     * The private constructor
     */
    my_send_cb_impl::my_send_cb_impl(bool gray_code)
      : gr::sync_block("my_send_cb",
               gr::io_signature::make(1, 1, sizeof(gr_complex)),

              gr::io_signature::make(1, 1, sizeof(char))),

        d_gray_code(gray_code))
    {}

    /*
     * Our virtual destructor.
     */
    my_send_cb_impl::~my_send_cb_impl()
    {
    }

    int
    my_send_cb_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      <+OTYPE+> *out = (<+OTYPE+> *) output_items[0];

      // Do <+signal processing+>

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace wimax */
} /* namespace gr */

