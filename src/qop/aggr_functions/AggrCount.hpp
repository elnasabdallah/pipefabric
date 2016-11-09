/*
 * Copyright (c) 2014-16 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */
 
#ifndef AGGRCOUNT_HPP_
#define AGGRCOUNT_HPP_

#include "AggregateFunc.hpp"

#include <type_traits>


namespace pfabric {

/**
 * @brief A counting aggregation function.
 *
 * @tparam Tin
 *    the type of the input argument
 * @tparam Tres
 *    the type of the result (must be convertible from unsigned int)
 */
template<
	typename Tin,
	typename Tres
>
class AggrCount :
	public AggregateFunc< Tin, Tres >
{
private:

	/// the type used for implementing the counter
	typedef unsigned int Count;

	static_assert( std::is_convertible< Count, Tres >::value,
		"result type must be convertible from a counter"
	);

public:

	AggrCount() {
		init();
	}

	virtual void init() override {
		mCount = 0;
	}

	virtual void iterate(Tin const& data, bool outdated = false) override {
 		mCount += (outdated ? -1 : 1);
	}

	virtual Tres value() override {
		return mCount;
	}

private:
	Count mCount;
};

} /* end namespace pquery */


#endif /* AGGRCOUNT_HPP_ */
