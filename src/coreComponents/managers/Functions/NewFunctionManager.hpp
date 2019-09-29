/*
 * ------------------------------------------------------------------------------------------------------------
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Copyright (c) 2018-2019 Lawrence Livermore National Security LLC
 * Copyright (c) 2018-2019 The Board of Trustees of the Leland Stanford Junior University
 * Copyright (c) 2018-2019 Total, S.A
 * Copyright (c) 2019-     GEOSX Contributors
 * All right reserved
 *
 * See top level LICENSE, COPYRIGHT, CONTRIBUTORS, NOTICE, and ACKNOWLEDGEMENTS files for details.
 * ------------------------------------------------------------------------------------------------------------
 */

/**
 * @file NewFunctionManager.hpp
 */

#ifndef NEWFUNCTIONMANAGER_HPP_
#define NEWFUNCTIONMANAGER_HPP_

#include "dataRepository/Group.hpp"
#include "FunctionBase.hpp"

namespace geosx
{



class NewFunctionManager : public dataRepository::Group
{
public:
  NewFunctionManager( const std::string& name,
                      dataRepository::Group * const parent );
  virtual ~NewFunctionManager() override;

  static NewFunctionManager * Instance()
  {
    static NewFunctionManager * theFunctionManager = nullptr;
    if (theFunctionManager == nullptr)
    {
      theFunctionManager = new NewFunctionManager("Functions", nullptr); 
    }
    return theFunctionManager;
  }

  static void finalize()
  {
    delete Instance();
  }

  static string CatalogName() { return "NewFunctionManager"; }
  virtual Group * CreateChild( string const & functionCatalogKey, string const & functionName ) override;

  /// This function is used to expand any catalogs in the data structure
  virtual void ExpandObjectCatalogs() override;
  
};


} /* namespace geosx */

#endif /* NEWFUNCTIONMANAGER_HPP_ */
