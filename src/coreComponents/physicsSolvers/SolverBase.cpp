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

#include "SolverBase.hpp"
#include "PhysicsSolverManager.hpp"

#include "common/TimingMacros.hpp"
#include "linearAlgebra/utilities/LinearSolverParameters.hpp"
#include "managers/DomainPartition.hpp"

namespace geosx
{

using namespace dataRepository;

SolverBase::SolverBase( std::string const & name,
                        Group * const parent )
  :
  ExecutableGroup( name, parent ),
  m_gravityVector( R1Tensor( 0.0 ) ),
  m_systemSolverParameters( groupKeyStruct::systemSolverParametersString, this ),
  m_cflFactor(),
  m_maxStableDt{ 1e99 },
  m_nextDt(1e99),
  m_dofManager( name )
{
  setInputFlags( InputFlags::OPTIONAL_NONUNIQUE );

  // This enables logLevel filtering
  enableLogLevelInput();

  this->registerWrapper( viewKeyStruct::gravityVectorString, &m_gravityVector, false );

  // This sets a flag to indicate that this object increments time
  this->SetTimestepBehavior( 1 );

  registerWrapper( viewKeyStruct::cflFactorString, &m_cflFactor, false )->
    setApplyDefaultValue( 0.5 )->
    setInputFlag( InputFlags::OPTIONAL )->
    setDescription( "Factor to apply to the `CFL condition <http://en.wikipedia.org/wiki/Courant-Friedrichs-Lewy_condition>`_"
                    " when calculating the maximum allowable time step. Values should be in the interval (0,1] " );

  registerWrapper( viewKeyStruct::maxStableDtString, &m_maxStableDt, false )->
    setApplyDefaultValue( 0.5 )->
    setInputFlag( InputFlags::FALSE )->
    setDescription( "Value of the Maximum Stable Timestep for this solver." );

  this->registerWrapper( viewKeyStruct::discretizationString, &m_discretizationName, false )->
    setApplyDefaultValue( "none" )->
    setInputFlag( InputFlags::OPTIONAL )->
    setDescription( "Name of discretization object (defined in the :ref:`NumericalMethodsManager`) to use for this "
                    "solver. For instance, if this is a Finite Element Solver, the name of a :ref:`FiniteElement` "
                    "should be specified. If this is a Finite Volume Method, the name of a :ref:`FiniteVolume` "
                    "discretization should be specified." );

  registerWrapper( viewKeyStruct::targetRegionsString, &m_targetRegions, false )->
    setInputFlag( InputFlags::REQUIRED )->
    setDescription( "Allowable regions that the solver may be applied to. Note that this does not indicate that "
                    "the solver will be applied to these regions, only that allocation will occur such that the "
                    "solver may be applied to these regions. The decision about what regions this solver will be"
                    "applied to rests in the EventManager." );

  registerWrapper( viewKeyStruct::initialDtString, &m_nextDt, false )->
    setApplyDefaultValue( 1e99 )->
    setInputFlag( InputFlags::OPTIONAL )->
    setDescription( "Initial time-step value required by the solver to the event manager." );
}

SolverBase::~SolverBase()
{
//  delete m_linearSolverWrapper;
}

SolverBase::CatalogInterface::CatalogType & SolverBase::GetCatalog()
{
  static SolverBase::CatalogInterface::CatalogType catalog;
  return catalog;
}

Group * SolverBase::CreateChild( string const & childKey, string const & childName )
{
  Group * rval = nullptr;
  if( childKey == SystemSolverParameters::CatalogName() )
  {
    rval = RegisterGroup( childName, &m_systemSolverParameters, 0 );
  }
  else
  {
    GEOS_ERROR( childKey << " is an invalid key to SolverBase child group." );
  }
  return rval;
}

void SolverBase::ExpandObjectCatalogs()
{
  CreateChild( SystemSolverParameters::CatalogName(), SystemSolverParameters::CatalogName() );
}

void SolverBase::PostProcessInput()
{
  if( this->globalGravityVector() != nullptr )
  {
    m_gravityVector = *globalGravityVector();
  }

  SetLinearSolverParameters();
}

void SolverBase::SetLinearSolverParameters()
{
  m_linearSolverParameters.logLevel = m_systemSolverParameters.getLogLevel();

  if ( m_systemSolverParameters.scalingOption() )
  {
    m_linearSolverParameters.scaling.useRowScaling = true;
  }

  if( m_systemSolverParameters.useDirectSolver() )
  {
    m_linearSolverParameters.solverType = "direct";
  }
  else
  {
    m_linearSolverParameters.krylov.maxIterations = m_systemSolverParameters.numKrylovIter();
    m_linearSolverParameters.krylov.tolerance = m_systemSolverParameters.krylovTol();

    if ( m_systemSolverParameters.kspace() > 0 )
    {
      m_linearSolverParameters.krylov.maxRestart = m_systemSolverParameters.kspace();
    }

    if ( m_systemSolverParameters.useBicgstab() )
    {
      m_linearSolverParameters.solverType = "bicgstab";
    }
    else
    {
      m_linearSolverParameters.solverType = "gmres";
    }

    if ( m_systemSolverParameters.useMLPrecond() )
    {
      m_linearSolverParameters.preconditionerType = "amg";

      // TODO hardcoded to match old behavior
      m_linearSolverParameters.amg.cycleType = "W";
      m_linearSolverParameters.amg.smootherType = "ilu";
    }
    else
    {
      m_linearSolverParameters.preconditionerType = "ilut";
      m_linearSolverParameters.ilu.fill = static_cast<int>( m_systemSolverParameters.ilut_fill() );
      m_linearSolverParameters.ilu.threshold = m_systemSolverParameters.ilut_drop();

      // TODO hardcoded to match old behavior
      m_linearSolverParameters.dd.overlap = 1;
    }
  }
}

real64 SolverBase::SolverStep( real64 const & GEOSX_UNUSED_ARG( time_n ),
                               real64 const & GEOSX_UNUSED_ARG( dt ),
                               const integer GEOSX_UNUSED_ARG( cycleNumber ),
                               DomainPartition * const GEOSX_UNUSED_ARG( domain ) )
{
  return 0;
}


void SolverBase::Execute( real64 const time_n,
                          real64 const dt,
                          integer const cycleNumber,
                          integer const GEOSX_UNUSED_ARG( eventCounter ),
                          real64 const GEOSX_UNUSED_ARG( eventProgress ),
                          Group * const domain )
{
  GEOSX_MARK_FUNCTION;
  real64 dtRemaining = dt;
  real64 nextDt = dt;

  SystemSolverParameters * const solverParams = getSystemSolverParameters();
  integer const maxSubSteps = solverParams->maxSubSteps();
  integer subStep = 0;

  for( ; subStep < maxSubSteps && dtRemaining > 0.0; ++subStep )
  {
    real64 const dtAccepted = SolverStep( time_n + (dt - dtRemaining),
                                          nextDt,
                                          cycleNumber,
                                          domain->group_cast<DomainPartition *>() );
    /*
     * Let us check convergence history of previous solve:
     * - number of nonlinear iter.
     * - if the time-step was chopped. Then we can add some heuristics to choose next dt.
     * */
    dtRemaining -= dtAccepted;

    if( dtRemaining > 0.0 )
    {
    	SetNextDt(solverParams, dtAccepted, nextDt);
    	nextDt = std::min(nextDt, dtRemaining);
    }

    if( getLogLevel() >= 1 && dtRemaining > 0.0 )
    {
      GEOS_LOG_LEVEL_RANK_0( 1, getName() << ": sub-step = " << subStep
                                       << ", accepted dt = " << dtAccepted
                                       << ", remaining dt = " << dtRemaining );
    }
  }

  GEOS_ERROR_IF( dtRemaining > 0.0, "Maximum allowed number of sub-steps reached. Consider increasing maxSubSteps." );

  // Decide what to do with the next Dt for the event running the solver.
  SetNextDt(solverParams, nextDt, m_nextDt);
}

void SolverBase::SetNextDt( SystemSolverParameters * const solverParams,
                            real64 const & currentDt,
                            real64 & nextDt )
{
  integer & newtonIter = solverParams->numNewtonIterations();
  int iterCutLimit = std::ceil(solverParams->dtCutIterLimit());
  int iterIncLimit = std::ceil(solverParams->dtIncIterLimit());

  if (newtonIter <  iterIncLimit )
  {
    // Easy convergence, let's double the time-step.
    nextDt = 2*currentDt;
    GEOS_LOG_LEVEL_RANK_0( 2,  getName() << ": Newton solver converged in less than " << iterIncLimit << " iterations, time-step required will be doubled.");
  }else if (newtonIter >  iterCutLimit)
  {
    // Tough convergence let us make the time-step smaller!
    nextDt = currentDt/2;
    GEOS_LOG_LEVEL_RANK_0(2, getName() << ": Newton solver converged in more than " << iterCutLimit << " iterations, time-step required will be halved.");
  }else
  {
    nextDt = currentDt;
  }
}

real64 SolverBase::LinearImplicitStep( real64 const & time_n,
                                       real64 const & dt,
                                       integer const GEOSX_UNUSED_ARG( cycleNumber ),
                                       DomainPartition * const domain,
                                       DofManager & dofManager,
                                       ParallelMatrix & matrix,
                                       ParallelVector & rhs,
                                       ParallelVector & solution )
{
  // call setup for physics solver. Pre step allocations etc.
  ImplicitStepSetup( time_n, dt, domain, dofManager, matrix, rhs, solution );

  // call assemble to fill the matrix and the rhs
  AssembleSystem( time_n, dt, domain, dofManager, matrix, rhs );

  // apply boundary conditions to system
  ApplyBoundaryConditions( time_n, dt, domain, dofManager, matrix, rhs );

  // call the default linear solver on the system
  SolveSystem( dofManager, matrix, rhs, solution );

  // apply the system solution to the fields/variables
  ApplySystemSolution( dofManager, solution, 1.0, domain );

  // final step for completion of timestep. typically secondary variable updates and cleanup.
  ImplicitStepComplete( time_n, dt, domain );

  // return the achieved timestep
  return dt;
}

bool SolverBase::LineSearch( real64 const & time_n,
                             real64 const & dt,
                             integer const GEOSX_UNUSED_ARG( cycleNumber ),
                             DomainPartition * const domain,
                             DofManager const & dofManager,
                             ParallelMatrix & matrix,
                             ParallelVector & rhs,
                             ParallelVector const & solution,
                             real64 const scaleFactor,
                             real64 & lastResidual )
{
  //GEOS_LOG_LEVEL_RANK_0(1, "    Beginning line search with last residual = " << std::scientific << lastResidual );
  SystemSolverParameters * const solverParams = getSystemSolverParameters();

  integer const maxNumberLineSearchCuts = solverParams->maxLineSearchCuts();
  real64 const lineSearchCutFactor = solverParams->lineSearchCutFactor();

  // flag to determine if we should solve the system and apply the solution. If the line
  // search fails we just bail.
  bool lineSearchSuccess = false;

  real64 residualNorm = lastResidual;

  // scale factor is value applied to the previous solution. In this case we want to
  // subtract a portion of the previous solution.
  real64 localScaleFactor = -scaleFactor;
  real64 cumulativeScale = scaleFactor;

  // main loop for the line search.
  for( integer lineSearchIteration = 0; lineSearchIteration < maxNumberLineSearchCuts; ++lineSearchIteration )
  {
    m_nlSolverOutputLog.clear();

    // cut the scale factor by half. This means that the scale factors will
    // have values of -0.5, -0.25, -0.125, ...
    localScaleFactor *= lineSearchCutFactor;
    cumulativeScale += localScaleFactor;

    if( !CheckSystemSolution( domain, dofManager, solution, localScaleFactor ) )
    {
      GEOS_LOG_LEVEL_RANK_0( 1, "---- Line search " << lineSearchIteration << ", solution check failed" );
      continue;
    }

    ApplySystemSolution( dofManager, solution, localScaleFactor, domain );

    // re-assemble system
    AssembleSystem( time_n, dt, domain, dofManager, matrix, rhs );

    // apply boundary conditions to system
    ApplyBoundaryConditions( time_n, dt, domain, dofManager, matrix, rhs );

    // get residual norm
    residualNorm = CalculateResidualNorm( domain, dofManager, rhs );

    {
      char output[100];
      sprintf(output,"---- Line search @ %.3f, R_abs = %.3e",cumulativeScale,residualNorm);
      GEOS_LOG_LEVEL_RANK_0(1,output);
    }

    // if the residual norm is less than the last residual, we can proceed to the
    // solution step
    if( residualNorm < lastResidual )
    {
      lineSearchSuccess = true;
      break;
    }
  }

  lastResidual = residualNorm;
  return lineSearchSuccess;
}


real64 SolverBase::NonlinearImplicitStep( real64 const & time_n,
                                          real64 const & dt,
                                          integer const cycleNumber,
                                          DomainPartition * const domain,
                                          DofManager const & dofManager,
                                          ParallelMatrix & matrix,
                                          ParallelVector & rhs,
                                          ParallelVector & solution )
{
  GEOSX_MARK_FUNCTION;
  // dt may be cut during the course of this step, so we are keeping a local
  // value to track the achieved dt for this step.
  real64 stepDt = dt;

  SystemSolverParameters * const solverParams = getSystemSolverParameters();

  integer const maxNewtonIter = solverParams->maxIterNewton();
  real64 const newtonTol = solverParams->newtonTol();

  integer const maxNumberDtCuts = solverParams->maxTimeStepCuts();
  real64 const dtCutFactor = solverParams->timeStepCutFactor();

  bool const allowNonConverged = solverParams->allowNonConverged() > 0;

  integer & dtAttempt = solverParams->numdtAttempts();

  // a flag to denote whether we have converged
  integer isConverged = 0;

  // outer loop attempts to apply full timestep, and managed the cutting of the timestep if
  // required.
  for(dtAttempt = 0; dtAttempt < maxNumberDtCuts; ++dtAttempt )
  {
    // reset the solver state, since we are restarting the time step
    if( dtAttempt > 0 )
    {
      ResetStateToBeginningOfStep( domain );
    }

    // keep residual from previous iteration in case we need to do a line search
    real64 lastResidual = 1e99;
    integer & newtonIter = solverParams->numNewtonIterations();
    real64 scaleFactor = 1.0;

    // main Newton loop
    for( newtonIter = 0; newtonIter < maxNewtonIter; ++newtonIter )
    {
      // call assemble to fill the matrix and the rhs
      AssembleSystem( time_n, stepDt, domain, dofManager, matrix, rhs );

      // apply boundary conditions to system
      ApplyBoundaryConditions( time_n, stepDt, domain, dofManager, matrix, rhs );

      // TODO: maybe add scale function here?
      // Scale()

      // get residual norm
      real64 residualNorm = CalculateResidualNorm( domain, dofManager, rhs );

      {
        char output[100];
        sprintf(output,"-- Attempt %2d, Newton %2d, R_abs = %.3e",dtAttempt,newtonIter,residualNorm);
        GEOS_LOG_LEVEL_RANK_0(1,output);
      }

      /* 
      GEOS_LOG_LEVEL_RANK_0( 1, " Attempt: " << dtAttempt << 
                                ", Newton: " << newtonIter << 
                                ", R_abs = " << std::scientific << residualNorm <<  
                                " (R_rel = " << residualNorm / residualNormZero << ")" );
      */

      // if the residual norm is less than the Newton tolerance we denote that we have
      // converged and break from the Newton loop immediately.

      if( residualNorm < newtonTol /*(residualNormZero+1)*/ ) 
      {
        isConverged = 1;

        /*
        if( getLogLevel() >= 1 )
        {
          char output[100] = {0};
          sprintf( output, " Attempt: %2d, NewtonIter: %2d, R = %4.2e ; ", dtAttempt, newtonIter, residualNorm );
          GEOS_LOG_LEVEL_RANK_0( 1, output << m_nlSolverOutputLog );
          m_nlSolverOutputLog.clear();
        }
        */

        break;
      }

      // do line search in case residual has increased
      if( residualNorm > lastResidual )
      {
        residualNorm = lastResidual;
        bool lineSearchSuccess = LineSearch( time_n, stepDt, cycleNumber, domain, dofManager,
                                             matrix, rhs, solution, scaleFactor, residualNorm );

        // if line search failed, then break out of the main Newton loop. Timestep will be cut.
        if( !lineSearchSuccess )
        {
          GEOS_LOG_LEVEL_RANK_0 ( 1, "---- The Line search failed!" );
          break;
        }
      }

      // if using adaptive Krylov tolerance scheme, update tolerance.
      // TODO: need to combine overlapping usage on LinearSolverParameters and SystemSolverParamters
      if(solverParams->useAdaptiveKrylovTol())
      {
        solverParams->m_krylovTol = LinearSolverParameters::eisenstatWalker(residualNorm,lastResidual);
      }

      // call the default linear solver on the system
      SolveSystem( dofManager, matrix, rhs, solution );

      {
        char output[100];
        sprintf(output,"---- Linear solve, Tol = %.2e, Iter = %3d",solverParams->m_krylovTol,solverParams->m_numKrylovIter);
        GEOS_LOG_LEVEL_RANK_0(1,output);
      }

      /*
      if( getLogLevel() >= 1 )
      {
        char output[100] = {0};
        sprintf( output, " Attempt: %2d, NewtonIter: %2d, R = %4.2e ; ", dtAttempt, newtonIter, residualNorm );
        GEOS_LOG_LEVEL_RANK_0( 1, output << m_nlSolverOutputLog );
        m_nlSolverOutputLog.clear();
      }
      */

      scaleFactor = ScalingForSystemSolution( domain, dofManager, solution );

      if( !CheckSystemSolution( domain, dofManager, solution, scaleFactor ) )
      {
        // TODO try chopping (similar to line search)
        GEOS_LOG_RANK_0( "  Solution check failed. Newton loop terminated." );
        break;
      }

      // apply the system solution to the fields/variables
      ApplySystemSolution( dofManager, solution, scaleFactor, domain );

      lastResidual = residualNorm;
    }

    if( isConverged )
    {
      break; // out of outer loop
    }
    else    
    {
      // cut timestep, go back to beginning of step and restart the Newton loop
      stepDt *= dtCutFactor;
      GEOS_LOG_LEVEL_RANK_0 ( 1, "New dt = " <<  stepDt );
    }    
  }

  if( !isConverged )
  {
    GEOS_LOG_RANK_0( "Convergence not achieved." );

    if( allowNonConverged )
    {
      GEOS_LOG_RANK_0( "The accepted solution may be inaccurate." );
    }
    else
    {
      GEOS_ERROR( "Nonconverged solutions not allowed. Terminating..." );
    }
  }

  // return the achieved timestep
  return stepDt;
}

real64 SolverBase::ExplicitStep( real64 const & GEOSX_UNUSED_ARG( time_n ),
                                 real64 const & GEOSX_UNUSED_ARG( dt ),
                                 integer const GEOSX_UNUSED_ARG( cycleNumber ),
                                 DomainPartition * const GEOSX_UNUSED_ARG( domain ) )
{
  GEOS_ERROR( "SolverBase::ExplicitStep called!. Should be overridden." );
  return 0;
}

void SolverBase::ImplicitStepSetup( real64 const & GEOSX_UNUSED_ARG( time_n ),
                                    real64 const & GEOSX_UNUSED_ARG( dt ),
                                    DomainPartition * const GEOSX_UNUSED_ARG( domain ),
                                    DofManager & GEOSX_UNUSED_ARG( dofManager ),
                                    ParallelMatrix & GEOSX_UNUSED_ARG( matrix ),
                                    ParallelVector & GEOSX_UNUSED_ARG( rhs ),
                                    ParallelVector & GEOSX_UNUSED_ARG( solution ) )
{
  GEOS_ERROR( "SolverBase::ImplicitStepSetup called!. Should be overridden." );
}

void SolverBase::SetupDofs( DomainPartition const * const GEOSX_UNUSED_ARG( domain ),
                            DofManager & GEOSX_UNUSED_ARG( dofManager ) ) const
{
  GEOS_ERROR( "SolverBase::SetupDofs called!. Should be overridden." );
}

void SolverBase::SetupSystem( DomainPartition * const domain,
                              DofManager & dofManager,
                              ParallelMatrix & matrix,
                              ParallelVector & rhs,
                              ParallelVector & solution )
{
  GEOSX_MARK_FUNCTION;

  dofManager.setMesh( domain, 0, 0 );

  SetupDofs( domain, dofManager );
  dofManager.close();

  dofManager.setSparsityPattern( matrix );
  dofManager.setVector( rhs );
  dofManager.setVector( solution );
}

void SolverBase::AssembleSystem( real64 const GEOSX_UNUSED_ARG( time ),
                                 real64 const GEOSX_UNUSED_ARG( dt ),
                                 DomainPartition * const GEOSX_UNUSED_ARG( domain ),
                                 DofManager const & GEOSX_UNUSED_ARG( dofManager ),
                                 ParallelMatrix & GEOSX_UNUSED_ARG( matrix ),
                                 ParallelVector & GEOSX_UNUSED_ARG( rhs ) )
{
  GEOS_ERROR( "SolverBase::Assemble called!. Should be overridden." );
}

void SolverBase::ApplyBoundaryConditions( real64 const GEOSX_UNUSED_ARG( time ),
                                          real64 const GEOSX_UNUSED_ARG( dt ),
                                          DomainPartition * const GEOSX_UNUSED_ARG( domain ),
                                          DofManager const & GEOSX_UNUSED_ARG( dofManager ),
                                          ParallelMatrix & GEOSX_UNUSED_ARG( matrix ),
                                          ParallelVector & GEOSX_UNUSED_ARG( rhs ) )
{
  GEOS_ERROR( "SolverBase::SolveSystem called!. Should be overridden." );
}

real64
SolverBase::CalculateResidualNorm( DomainPartition const * const GEOSX_UNUSED_ARG( domain ),
                                   DofManager const & GEOSX_UNUSED_ARG( dofManager ),
                                   ParallelVector const & GEOSX_UNUSED_ARG( rhs ) )
{
  GEOS_ERROR( "SolverBase::CalculateResidualNorm called!. Should be overridden." );
  return 0;
}

void SolverBase::SolveSystem( DofManager const & GEOSX_UNUSED_ARG( dofManager ),
                              ParallelMatrix & matrix,
                              ParallelVector & rhs,
                              ParallelVector & solution )
{
  GEOSX_MARK_FUNCTION;
  // Create a solver from the parameter list
  LinearSolver solver( m_linearSolverParameters );

  // Solve using the iterative solver and compare norms with true solution
  solver.solve( matrix, solution, rhs );
}

bool SolverBase::CheckSystemSolution( DomainPartition const * const GEOSX_UNUSED_ARG( domain ),
                                      DofManager const & GEOSX_UNUSED_ARG( dofManager ),
                                      ParallelVector const & GEOSX_UNUSED_ARG( solution ),
                                      real64 const GEOSX_UNUSED_ARG( scalingFactor ) )
{
  return true;
}

real64 SolverBase::ScalingForSystemSolution( DomainPartition const * const GEOSX_UNUSED_ARG( domain ),
                                             DofManager const & GEOSX_UNUSED_ARG( dofManager ),
                                             ParallelVector const & GEOSX_UNUSED_ARG( solution ) )
{
  return 1.0;
}

void SolverBase::ApplySystemSolution( DofManager const & GEOSX_UNUSED_ARG( dofManager ),
                                      ParallelVector const & GEOSX_UNUSED_ARG( solution ),
                                      real64 const GEOSX_UNUSED_ARG( scalingFactor ),
                                      DomainPartition * const GEOSX_UNUSED_ARG( domain ) )
{
  GEOS_ERROR( "SolverBase::ApplySystemSolution called!. Should be overridden." );
}

void SolverBase::ResetStateToBeginningOfStep( DomainPartition * GEOSX_UNUSED_ARG( const ) )
{
  GEOS_ERROR( "SolverBase::ResetStateToBeginningOfStep called!. Should be overridden." );
}

void SolverBase::ImplicitStepComplete( real64 const & GEOSX_UNUSED_ARG( time ),
                                       real64 const & GEOSX_UNUSED_ARG( dt ),
                                       DomainPartition * const GEOSX_UNUSED_ARG( domain ) )
{
  GEOS_ERROR( "SolverBase::ImplicitStepComplete called!. Should be overridden." );
}

R1Tensor const * SolverBase::globalGravityVector() const
{
  R1Tensor const * rval = nullptr;
  if( getParent()->getName() == "Solvers" )
  {
    rval = &(getParent()->getReference<R1Tensor>( viewKeyStruct::gravityVectorString ));
  }

  return rval;
}


} /* namespace ANST */
