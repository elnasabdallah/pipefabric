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

#ifndef RelatedStateValueMax_hpp_
#define RelatedStateValueMax_hpp_

#include "RelatedStateValue.hpp"
namespace pfabric {
template <class Tin, class StorageType, class ResultType,  int Index>
class RelatedStateValueMax: public RelatedStateValue<Tin, StorageType, ResultType,Index> {
private:
	/**
	 * store sum value for this state for particular tuple attribute
	 */
	StorageType maxValue;
public:
	/**
	 * Gets the current value
	 * @return the current value
	 */
	ResultType getValue() {
		return maxValue ;
	}

	/**
	 * Updates the value
	 * @param e the newly selected event
	 */
	void updateValue(const typename RelatedStateValue<Tin, StorageType, ResultType,Index>::TinPtr& e) {
		ResultType temp = std::get<Index>(e);
			if (temp > maxValue) {
				this->maxValue = temp;
			}
	}
	/**
	 * initializes the value by an event
	 * @param e
	 */
	void initValue(const typename RelatedStateValue<Tin, StorageType, ResultType,Index>::TinPtr& e) {
		updateValue(e);
	}
	/**
	 * constructor
	 */
	RelatedStateValueMax() {
		maxValue = std::numeric_limits<double>::min();
	}
	/**
	 * destructor
	 */
	virtual ~RelatedStateValueMax() {

	}
};
}
#endif /* RelatedStateValueMax_hpp_ */
