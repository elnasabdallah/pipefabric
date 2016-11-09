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
 
#ifndef AGGRGLOBALMAX_HPP_
#define AGGRGLOBALMAX_HPP_

#include "AggregateFunc.hpp"

#include <cassert>
#include <string>
#include <limits>


namespace pfabric {

/**
 * @brief An aggregation determining the maximum value in a stream.
 *
 * @tparam Tin
 *    the type of the input argument
 */
template<
	typename Tin
>
class AggrGlobalMax : public AggregateFunc<Tin, Tin> {
public:

	AggrGlobalMax() {
        init();
    }

    virtual void init() override {
    	mMax = std::numeric_limits<Tin>::min();
    }

    virtual void iterate(Tin const& data, bool outdated = false) override {
        assert(!outdated);
        mMax = std::max(mMax, data);
    }

    virtual Tin value() override {
        return mMax;
    }

private:
    Tin mMax;
};

/*
 * TODO Question: What is this?
 */
template<>
inline void AggrGlobalMax<std::string>::init() {
	mMax = 0x00f;
}


} /* end namespace pquery */


#endif /* AGGRGLOBALMAX_HPP_ */
