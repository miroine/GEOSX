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
 * @file ProppantTransport.hpp
 */

#ifndef SRC_COMPONENTS_CORE_SRC_PHYSICSSOLVERS_PROPPANTTRANSPORT_HPP_
#define SRC_COMPONENTS_CORE_SRC_PHYSICSSOLVERS_PROPPANTTRANSPORT_HPP_

#include "physicsSolvers/FiniteVolume/FlowSolverBase.hpp"
#include "constitutive/Fluid/SlurryFluidBase.hpp"

class Epetra_FECrsGraph;

namespace geosx
{

namespace dataRepository
{
class ManagedGroup;
}
class FieldSpecificationBase;
class FiniteElementBase;
class DomainPartition;

/**
 * @class ProppantTransport
 *
 * class to perform a proppant finite volume solve.
 */
class ProppantTransport : public FlowSolverBase
{
public:
  /**
   * @brief main constructor for ManagedGroup Objects
   * @param name the name of this instantiation of ManagedGroup in the repository
   * @param parent the parent group of this instantiation of ManagedGroup
   */
  ProppantTransport( const std::string& name,
                   ManagedGroup * const parent );


  /// deleted default constructor
  ProppantTransport() = delete;

  /// deleted copy constructor
  ProppantTransport( ProppantTransport const & ) = delete;

  /// default move constructor
  ProppantTransport( ProppantTransport && ) = default;

  /// deleted assignment operator
  ProppantTransport & operator=( ProppantTransport const & ) = delete;

  /// deleted move operator
  ProppantTransport & operator=( ProppantTransport && ) = delete;

  /**
   * @brief default destructor
   */
  virtual ~ProppantTransport() override = default;

  /**
   * @brief name of the node manager in the object catalog
   * @return string that contains the catalog name to generate a new NodeManager object through the object catalog.
   */
  static string CatalogName() { return "ProppantTransport"; }

  virtual void InitializePreSubGroups(ManagedGroup * const rootGroup) override;

  virtual void RegisterDataOnMesh(ManagedGroup * const MeshBodies) override;

  virtual real64 SolverStep( real64 const& time_n,
                             real64 const& dt,
                             integer const cycleNumber,
                             DomainPartition * domain ) override;

  /**
   * @defgroup Solver Interface Functions
   *
   * These functions provide the primary interface that is required for derived classes
   */
  /**@{*/

  virtual void ImplicitStepSetup( real64 const& time_n,
                                  real64 const& dt,
                                  DomainPartition * const domain,
                                  systemSolverInterface::EpetraBlockSystem * const blockSystem ) override;


  virtual void AssembleSystem( DomainPartition * const domain,
                               systemSolverInterface::EpetraBlockSystem * const blockSystem,
                               real64 const time_n,
                               real64 const dt ) override;

  virtual void ApplyBoundaryConditions( DomainPartition * const domain,
                                        systemSolverInterface::EpetraBlockSystem * const blockSystem,
                                        real64 const time_n,
                                        real64 const dt ) override;

  virtual real64
  CalculateResidualNorm(systemSolverInterface::EpetraBlockSystem const *const blockSystem,
                        DomainPartition *const domain) override;

  virtual void SolveSystem( systemSolverInterface::EpetraBlockSystem * const blockSystem,
                            SystemSolverParameters const * const params ) override;

  virtual void
  ApplySystemSolution( systemSolverInterface::EpetraBlockSystem const * const blockSystem,
                       real64 const scalingFactor,
                       DomainPartition * const domain ) override;

  virtual void ResetStateToBeginningOfStep( DomainPartition * const domain ) override;

  virtual  void ImplicitStepComplete( real64 const & time,
                                      real64 const & dt,
                                      DomainPartition * const domain ) override;

  /**
   * @brief assembles the accumulation terms for all cells
   * @param domain the physical domain object
   * @param blockSystem the entire block system
   * @param time_n previous time value
   * @param dt time step
   */

  void AssembleAccumulationTerms( DomainPartition * const domain,
                                  Epetra_FECrsMatrix * const jacobian,
                                  Epetra_FEVector * const residual,
                                  real64 const time_n,
                                  real64 const dt );

  /**
   * @brief assembles the flux terms for all cells
   * @param domain the physical domain object
   * @param blockSystem the entire block system
   * @param time_n previous time value
   * @param dt time step
   */
  void AssembleFluxTerms( DomainPartition * const domain,
                          Epetra_FECrsMatrix * const jacobian,
                          Epetra_FEVector * const residual,
                          real64 const time_n,
                          real64 const dt );

  /**@}*/


  struct viewKeyStruct : FlowSolverBase::viewKeyStruct
  {

    static constexpr auto blockLocalDofNumberString = "blockLocalDofNumber_ProppantTransport" ;

    static constexpr auto proppantNameString      = "proppantName";
    static constexpr auto proppantIndexString      = "proppantIndex";
    

    // primary solution field
    static constexpr auto pressureString      = "pressure";
    static constexpr auto deltaPressureString = "deltaPressure";

    static constexpr auto proppantConcentrationString      = "proppantConcentration";
    static constexpr auto deltaProppantConcentrationString      = "deltaProppantConcentration";    
    
    static constexpr auto deltaVolumeString = "deltaVolume";

    // these are used to store last converged time step values
    static constexpr auto densityString   = "density";
    static constexpr auto oldProppantConcentrationString  = "oldProppantConcentration";
    static constexpr auto slipVelocityString   = "slipVelocity";
    

    static constexpr auto porosityString  = "porosity";

    static constexpr auto updatePermeabilityString  = "updatePermeability";
    static constexpr auto updateProppantMobilityString  = "updateProppantMobility";        

    using ViewKey = dataRepository::ViewKey;

    // dof numbering
    ViewKey blockLocalDofNumber = { blockLocalDofNumberString };

    // primary solution field
    ViewKey pressure      = { pressureString };
    ViewKey deltaPressure = { deltaPressureString };

    ViewKey proppantConcentration      = { proppantConcentrationString };
    ViewKey deltaProppantConcentration = { deltaProppantConcentrationString };
    
    ViewKey deltaVolume   = { deltaVolumeString };

    ViewKey density      = { densityString };
    ViewKey oldProppantConcentration    = { oldProppantConcentrationString };

    ViewKey porosity     = { porosityString };

    ViewKey proppantName      = { proppantNameString };
    ViewKey proppantIndex      = { proppantIndexString };

    ViewKey updatePermeability = { updatePermeabilityString };
    ViewKey updateProppantMobility = { updateProppantMobilityString };
    
    
  } viewKeysProppantTransport;

  viewKeyStruct & viewKeys() { return viewKeysProppantTransport; }
  viewKeyStruct const & viewKeys() const { return viewKeysProppantTransport; }

  struct groupKeyStruct : SolverBase::groupKeyStruct
  {
  } groupKeysProppantTransport;

  groupKeyStruct & groupKeys() { return groupKeysProppantTransport; }
  groupKeyStruct const & groupKeys() const { return groupKeysProppantTransport; }

protected:

  virtual void InitializePostInitialConditions_PreSubGroups( dataRepository::ManagedGroup * const rootGroup ) override;

private:

  void SetupSystem ( DomainPartition * const domain,
                     systemSolverInterface::EpetraBlockSystem * const blockSystem );

  /**
   * @brief set the sparsity pattern for the linear system
   * @param domain the domain partition
   * @param sparsity the sparsity pattern matrix
   */
  void SetSparsityPattern( DomainPartition const * const domain,
                           Epetra_FECrsGraph * const sparsity );

  /**
   * @brief sets the dof indices for this solver
   * @param meshLevel the mesh object (single level only)
   * @param numLocalRows the number of local rows on this partition
   * @param numGlobalRows the number of global rows in the problem
   * @param localIndices unused TODO delete
   * @param offset the DOF offset for this solver in the case of a non-block system
   *
   * This function sets the number of global rows, and sets the dof numbers for
   * this solver. dof numbers are referred to trilinosIndices currently.
   */
  void SetNumRowsAndTrilinosIndices( MeshLevel * const meshLevel,
                                     localIndex & numLocalRows,
                                     globalIndex & numGlobalRows,
                                     localIndex offset );

  /**
   * @brief Setup stored views into domain data for the current step
   */
  void ResetViews( DomainPartition * const domain ) override;

  /**
   * @brief Function to update all constitutive models
   * @param domain the domain
   */
  void UpdateFluidModel( ManagedGroup * const dataGroup );

  void UpdateProppantModel( ManagedGroup * const dataGroup );

  void UpdateProppantModelStep( ManagedGroup * const dataGroup );    

  void UpdateState( ManagedGroup * dataGroup );

  /// views into primary variable fields

  ElementRegionManager::ElementViewAccessor<arrayView1d<globalIndex>> m_dofNumber; // TODO will move to DofManager

  ElementRegionManager::ElementViewAccessor<arrayView1d<real64>> m_pressure;
  ElementRegionManager::ElementViewAccessor<arrayView1d<real64>> m_deltaPressure;

  ElementRegionManager::ElementViewAccessor<arrayView1d<real64>> m_proppantConcentration;
  ElementRegionManager::ElementViewAccessor<arrayView1d<real64>> m_deltaProppantConcentration;  

  ElementRegionManager::ElementViewAccessor<arrayView1d<real64>> m_deltaVolume;


  /// views into backup fields

  ElementRegionManager::ElementViewAccessor<arrayView1d<real64>> m_densityOld;
  ElementRegionManager::ElementViewAccessor<arrayView1d<real64>> m_proppantConcentrationOld;
  //  ElementRegionManager::ElementViewAccessor<arrayView1d<R1Tensor>> m_slipVelocity;    

  /// views into material fields

  ElementRegionManager::MaterialViewAccessor<arrayView2d<real64>> m_density;
  ElementRegionManager::MaterialViewAccessor<arrayView2d<real64>> m_fluidDensity;  
  ElementRegionManager::MaterialViewAccessor<arrayView2d<real64>> m_dDens_dPres;
  ElementRegionManager::MaterialViewAccessor<arrayView2d<real64>> m_dDens_dConc;
  ElementRegionManager::MaterialViewAccessor<arrayView2d<real64>> m_dFluidDens_dPres;    

  ElementRegionManager::MaterialViewAccessor<arrayView2d<real64>> m_viscosity;
  ElementRegionManager::MaterialViewAccessor<arrayView2d<real64>> m_dVisc_dPres;
  ElementRegionManager::MaterialViewAccessor<arrayView2d<real64>> m_dVisc_dConc;

  static constexpr localIndex MAX_NUM_COMPONENTS = 2;

  ElementRegionManager::MaterialViewAccessor<arrayView1d<real64>> m_settlingFactor;
  ElementRegionManager::MaterialViewAccessor<arrayView1d<real64>> m_dSettlingFactor_dConc;

  ElementRegionManager::MaterialViewAccessor<arrayView1d<real64>> m_collisionFactor;
  ElementRegionManager::MaterialViewAccessor<arrayView1d<real64>> m_dCollisionFactor_dConc;

  ElementRegionManager::MaterialViewAccessor<arrayView1d<bool>> m_isProppantMobile;

  ElementRegionManager::MaterialViewAccessor<arrayView1d<real64>> m_proppantPackPermeability;          

  string m_proppantName;
  localIndex m_proppantIndex;  
  integer m_updatePermeability;
  integer m_updateProppantMobility;  

  //Below is not used in ProppantTransport model
  
  ElementRegionManager::ElementViewAccessor<arrayView1d<real64>> m_porosity;
  
  ElementRegionManager::MaterialViewAccessor<arrayView2d<real64>> m_pvMult;
  ElementRegionManager::MaterialViewAccessor<arrayView2d<real64>> m_dPvMult_dPres;
  
};


} /* namespace geosx */

#endif //SRC_COMPONENTS_CORE_SRC_PHYSICSSOLVERS_PROPPANTTRANSPORT_HPP_