#ifndef _PROBCOMMON_H_
#define _PROBCOMMON_H_

#include <AMReX_Geometry.H>
#include <AMReX_FArrayBox.H>
#include "DataStruct.H"

/**
 * Function to init the physical bounds of the domain
*/
void amrex_probinit (
  const amrex_real* problo,
  const amrex_real* probhi);

/**
 * Function to initialize the hydrostatic reference density
 *
 * @param[out] rho_hse hydrostatic reference density
 * @param[in] z_phys_nd height coordinate at nodes
 * @param[in] z_phys_cc height coordinate at cell centers
 * @param[in] geom container for geometric information
*/
void
erf_init_dens_hse (amrex::MultiFab& rho_hse,
                   std::unique_ptr<amrex::MultiFab>& z_phys_nd,
                   std::unique_ptr<amrex::MultiFab>& z_phys_cc,
                   amrex::Geometry const& geom);

/**
 * Function to perform custom initialization of a test problem
 *
 * @param[in]  bx cell-centered box on which to initialize scalars
 * @param[in] xbx box on which to initialize x-component of velocity
 * @param[in] ybx box on which to initialize y-component of velocity
 * @param[in] zbx box on which to initialize z-component of velocity
 * @param[out] state cell-centered variables to be filled in this routine
 * @param[out] x_velocity  x-component of velocity to be filled in this routine
 * @param[out] y_velocity  y-component of velocity to be filled in this routine
 * @param[out] z_velocity  z-component of velocity to be filled in this routine
 * @param[out] r_hse       hydrostatic reference density
 * @param[out] p_hse       hydrostatic reference pressure
 * @param[in] z_nd         height coordinate at nodes
 * @param[in] z_cc         height coordinate at cell centers
 * @param[in] qv           water vapor
 * @param[in] qc           cloud water
 * @param[in] qi           cloud ice
 * @param[in] mf_m         map factor on cell centers
 * @param[in] mf_u         map factor on x-faces
 * @param[in] mf_v         map factor on y-faces
 * @param[in] sc           SolverChoice structure that carries parameters
*/

void
init_custom_prob (
  const amrex::Box&  bx,
  const amrex::Box& xbx,
  const amrex::Box& ybx,
  const amrex::Box& zbx,
  amrex::Array4<amrex::Real      > const& state,
  amrex::Array4<amrex::Real      > const& x_vel,
  amrex::Array4<amrex::Real      > const& y_vel,
  amrex::Array4<amrex::Real      > const& z_vel,
  amrex::Array4<amrex::Real      > const& r_hse,
  amrex::Array4<amrex::Real      > const& p_hse,
  amrex::Array4<amrex::Real const> const& z_nd,
  amrex::Array4<amrex::Real const> const& z_cc,
#if defined(ERF_USE_MOISTURE)
  amrex::Array4<amrex::Real      > const& qv,
  amrex::Array4<amrex::Real      > const& qc,
  amrex::Array4<amrex::Real      > const& qi,
#elif defined(ERF_USE_WARM_NO_PRECIP)
  amrex::Array4<amrex::Real      > const& qv,
  amrex::Array4<amrex::Real      > const& qc,
#endif
  amrex::GeometryData const& geomdata,
  amrex::Array4<amrex::Real const> const& mf_m,
  amrex::Array4<amrex::Real const> const& mf_u,
  amrex::Array4<amrex::Real const> const& mf_v,
  const SolverChoice& sc
  );

/**
 * Function to perform custom initialization of terrain
 *
 * @param[in] geom container for geometric information
 * @param[out] z_phys_nd height coordinate at nodes
 * @param[in] time current time
*/
void
init_custom_terrain (const amrex::Geometry& geom,
                           amrex::MultiFab& z_phys_nd,
                     const amrex::Real& time);

amrex::Real compute_terrain_velocity(const amrex::Real time);

/**
 * Function to define the quantities needed to impose Rayleigh damping
 *
 * @param[in] tau  strength of Rayleigh damping
 * @param[in] ubar reference value for x-velocity used to define Rayleigh damping
 * @param[in] vbar reference value for y-velocity used to define Rayleigh damping
 * @param[in] wbar reference value for z-velocity used to define Rayleigh damping
 * @param[in] thetabar reference value for potential temperature used to define Rayleigh damping
 * @param[in] geom container for geometric information
*/
void
erf_init_rayleigh (amrex::Vector<amrex::Real>& tau,
                   amrex::Vector<amrex::Real>& ubar,
                   amrex::Vector<amrex::Real>& vbar,
                   amrex::Vector<amrex::Real>& wbar,
                   amrex::Vector<amrex::Real>& thetabar,
                   amrex::Geometry      const& geom);
#endif
