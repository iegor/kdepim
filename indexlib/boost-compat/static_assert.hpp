#ifndef LPC_STATIC_ASSERT_HPP1119293317_INCLUDE_GUARD_
#define LPC_STATIC_ASSERT_HPP1119293317_INCLUDE_GUARD_

#ifdef HAVE_BOOST
#include <boost/static_assert.hpp>
#elif !defined( BOOST_STATIC_ASSERT )
#define BOOST_STATIC_ASSERT( x )
#endif


#endif /* LPC_STATIC_ASSERT_HPP1119293317_INCLUDE_GUARD_ */
