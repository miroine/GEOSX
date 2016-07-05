/*
 * DataObjectBase.cpp
 *
 *  Created on: Jun 17, 2016
 *      Author: rrsettgast
 */

#include "DataObjectBase.hpp"

namespace geosx {

DataObjectBase::DataObjectBase( std::string const & name ):
  m_name(name)
{}


DataObjectBase::~DataObjectBase() {
	// TODO Auto-generated destructor stub
}


DataObjectBase::DataObjectBase( DataObjectBase&& source ):
    m_name( std::move(source.m_name) )
{}

} /* namespace geosx */
