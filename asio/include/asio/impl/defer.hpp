//
// impl/defer.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_DEFER_HPP
#define ASIO_IMPL_DEFER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/associated_allocator.hpp"
#include "asio/associated_executor.hpp"
#include "asio/detail/work_dispatcher.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class initiate_defer
{
public:
  template <typename CompletionHandler>
  void operator()(ASIO_MOVE_ARG(CompletionHandler) handler) const
  {
    typedef typename decay<CompletionHandler>::type handler_t;

    typename handler_t::executor_type ex(handler.get_executor());

    typename associated_allocator<handler_t>::type alloc(
        (get_associated_allocator)(handler));

    ex.defer(ASIO_MOVE_CAST(CompletionHandler)(handler), alloc);
  }
};

template <typename Executor>
class initiate_defer_with_executor
{
public:
  typedef Executor executor_type;

  explicit initiate_defer_with_executor(const Executor& ex)
    : io_ex_(ex)
  {
  }

  executor_type get_executor() const ASIO_NOEXCEPT
  {
    return io_ex_;
  }

  template <typename CompletionHandler>
  void operator()(ASIO_MOVE_ARG(CompletionHandler) handler) const
  {
    typedef typename decay<CompletionHandler>::type handler_t;

    typedef typename associated_executor<
      handler_t, Executor>::type handler_ex_t;
    handler_ex_t handler_ex((get_associated_executor)(handler, io_ex_));

    typename associated_allocator<handler_t>::type alloc(
        (get_associated_allocator)(handler));

    if (this->is_same_executor(io_ex_, handler_ex))
    {
      io_ex_.defer(ASIO_MOVE_CAST(CompletionHandler)(handler), alloc);
    }
    else
    {
      io_ex_.defer(detail::work_dispatcher<handler_t, handler_ex_t>(
            ASIO_MOVE_CAST(CompletionHandler)(handler), handler_ex), alloc);
    }
  }

private:
  template <typename T, typename U>
  bool is_same_executor(const T&, const U&) const
  {
    return false;
  }

  template <typename T>
  bool is_same_executor(const T& a, const T& b) const
  {
    return a == b;
  }

  Executor io_ex_;
};

} // namespace detail

template <ASIO_COMPLETION_TOKEN_FOR(void()) CompletionToken>
ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, void()) defer(
    ASIO_MOVE_ARG(CompletionToken) token)
{
  return async_initiate<CompletionToken, void()>(
      detail::initiate_defer(), token);
}

template <typename Executor,
    ASIO_COMPLETION_TOKEN_FOR(void()) CompletionToken>
ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, void()) defer(
    const Executor& ex, ASIO_MOVE_ARG(CompletionToken) token,
    typename enable_if<is_executor<Executor>::value>::type*)
{
  return async_initiate<CompletionToken, void()>(
      detail::initiate_defer_with_executor<Executor>(ex), token);
}

template <typename ExecutionContext,
    ASIO_COMPLETION_TOKEN_FOR(void()) CompletionToken>
inline ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, void()) defer(
    ExecutionContext& ctx, ASIO_MOVE_ARG(CompletionToken) token,
    typename enable_if<is_convertible<
      ExecutionContext&, execution_context&>::value>::type*)
{
  return (defer)(ctx.get_executor(),
      ASIO_MOVE_CAST(CompletionToken)(token));
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_DEFER_HPP
