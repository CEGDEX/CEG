//
// detail/impl/posix_thread.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_POSIX_THREAD_IPP
#define ASIO_DETAIL_IMPL_POSIX_THREAD_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_PTHREADS)

#include "asio/detail/posix_thread.hpp"
#include "asio/detail/throw_error.hpp"
#include "asio/error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

posix_thread::~posix_thread()
{
  if (!joined_)
    ::pthread_detach(thread_);
}

void posix_thread::join()
{
  if (!joined_)
  {
    ::pthread_join(thread_, 0);
    joined_ = true;
  }
}

void posix_thread::start_thread(func_base* arg)
{
  pthread_attr_t object_attr;
  pthread_attr_init(&object_attr);
  
  size_t stacksize = 0;
  int ret = 0;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  ret = pthread_attr_getstacksize(&attr, &stacksize);
  if(ret != 0) {
	printf("get stacksize error!:%d\n", (int)stacksize);
	pthread_attr_destroy(&object_attr);
	return;
  }
  
  //printf("default asio pthread statck size:%d\n", (int)stacksize);
  if(stacksize <= 2 * 1024 * 1024)
  {
	  stacksize = 2 * 1024 * 1024;
	  //printf("set pthread statck size:%d\n", (int)stacksize);
	  
	  ret = pthread_attr_setstacksize(&object_attr, stacksize);
	  if(ret != 0) {
		printf("set stacksize error!:%d\n", (int)stacksize);
		pthread_attr_destroy(&object_attr);
		return;
	  }
  }
  
  int error = ::pthread_create(&thread_, &object_attr,
        asio_detail_posix_thread_function, arg);
  if (error != 0)
  {
    delete arg;
    asio::error_code ec(error,
        asio::error::get_system_category());
    asio::detail::throw_error(ec, "thread");
  }
  
  pthread_attr_destroy(&object_attr);
}

void* asio_detail_posix_thread_function(void* arg)
{
  posix_thread::auto_func_base_ptr func = {
      static_cast<posix_thread::func_base*>(arg) };
  func.ptr->run();
  return 0;
}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_PTHREADS)

#endif // ASIO_DETAIL_IMPL_POSIX_THREAD_IPP
