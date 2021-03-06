// -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil  -*-
// $Id$
//
// Copyright 2010-2014 Christoph Mayer
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _PROCESSOR_REGISTRY_HPP_cm130110_
#define _PROCESSOR_REGISTRY_HPP_cm130110_

#include <map>
#include <string>

#include <boost/function.hpp>
#include <boost/functional/factory.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>

#include "processor.hpp"

namespace processor {
  /// registry for processors (deriving from @ref processor::base)
  class registry {
  public:
    typedef boost::function<base::sptr(const boost::property_tree::ptree&)> a_factory;
    typedef std::map<std::string, a_factory> map_type;

    registry() {}

    /// construct a processor with configuration \c config accoring to \c key (factory method)
    static base::sptr make(std::string key, const boost::property_tree::ptree& config) {
      map_type::iterator i(map().find(key));
      if (i == map().end())
        throw std::runtime_error(str(boost::format("processor::registry::make '%s' is not supported") % key));
      return i->second(config);
    }

    /// add a processor type to the registry
    template<typename T>
    static void add(std::string key) {
      map()[key] = boost::factory<typename T::sptr>();
    }

    static map_type& map() { return map_; }
    static map_type map_;
  private:
  } ;

} // namespace processor

#endif // _PROCESSOR_REGISTRY_HPP_cm130110_
