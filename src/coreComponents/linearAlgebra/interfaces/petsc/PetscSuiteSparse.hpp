/*
 * ------------------------------------------------------------------------------------------------------------
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Copyright (c) 2018-2020 Lawrence Livermore National Security LLC
 * Copyright (c) 2018-2020 The Board of Trustees of the Leland Stanford Junior University
 * Copyright (c) 2018-2020 Total, S.A
 * Copyright (c) 2019-     GEOSX Contributors
 * All rights reserved
 *
 * See top level LICENSE, COPYRIGHT, CONTRIBUTORS, NOTICE, and ACKNOWLEDGEMENTS files for details.
 * ------------------------------------------------------------------------------------------------------------
 */

/**
 * @file PetscSuiteSparse.hpp
 */

#ifndef GEOSX_LINEARALGEBRA_INTERFACES_PETSCSUITESPARSE_HPP_
#define GEOSX_LINEARALGEBRA_INTERFACES_PETSCSUITESPARSE_HPP_

#include "common/DataTypes.hpp"
#include "linearAlgebra/interfaces/petsc/PetscMatrix.hpp"
#include "linearAlgebra/utilities/LinearSolverParameters.hpp"

namespace geosx
{

/**
 * @brief Sets SuiteSparse options
 * @param[in] matrix the PetscMatrix object
 * @param[in] params the linear solver parameters
 */
void SuiteSparseSetFromOptions( PetscMatrix const & matrix,
                                LinearSolverParameters const & params );

}

#endif /*GEOSX_LINEARALGEBRA_INTERFACES_PETSCSUITESPARSE_HPP_*/
