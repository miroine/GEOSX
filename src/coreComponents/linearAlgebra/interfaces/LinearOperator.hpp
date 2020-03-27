/*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Copyright (c) 2019, Lawrence Livermore National Security, LLC.
 *
 * Produced at the Lawrence Livermore National Laboratory
 *
 * LLNL-CODE-746361
 *
 * All rights reserved. See COPYRIGHT for details.
 *
 * This file is part of the GEOSX Simulation Framework.
 *
 * GEOSX is a free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License (as published by the
 * Free Software Foundation) version 2.1 dated February 1999.
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

/**
 * @file LinearOperator.hpp
 */

#ifndef GEOSX_LINEARALGEBRA_INTERFACES_LINEAROPERATOR_HPP_
#define GEOSX_LINEARALGEBRA_INTERFACES_LINEAROPERATOR_HPP_

namespace geosx
{

/**
 * @brief Abstract base class for linear operators
 * @tparam VECTOR Type of vector this operator can be applied to
 */
template< typename VECTOR >
class LinearOperator
{
public:

  using Vector = VECTOR;

  LinearOperator() = default;

  virtual ~LinearOperator() = default;

  /**
   * @brief Apply operator to a vector
   * @param src Input vector (x).
   * @param dst Output vector (b).
   */
  virtual void apply( Vector const & src, Vector & dst ) const = 0;

  /**
   * @brief Compute residual <tt>r = Ax - b</tt>.
   *
   * @param x Input solution.
   * @param b Input right hand side.
   * @param r Output residual.
   */
  virtual void residual( Vector const & x, Vector const & b, Vector & r ) const
  {
    this->apply( x, r );
    r.axpby( 1.0, b, -1.0 );
  }

  /**
   * @brief Returns the number of global rows.
   */
  virtual globalIndex numGlobalRows() const = 0;

  /**
   * @brief Returns the number of global columns.
   */
  virtual globalIndex numGlobalCols() const = 0;
};

}

#endif //GEOSX_LINEARALGEBRA_INTERFACES_LINEAROPERATOR_HPP_
