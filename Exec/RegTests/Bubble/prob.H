#ifndef _PROB_H_
#define _PROB_H_

#include "EOS.H"
#include "AMReX_REAL.H"

struct ProbParm {
  // background conditions
  // if init_type != "" then these are perturbations and should be 0
  amrex::Real T_0 = 300.0;
  amrex::Real U_0 = 0.0;
  amrex::Real V_0 = 0.0;

  // center of thermal perturbation
  amrex::Real x_c = 0.0;
  amrex::Real y_c = 0.0;
  amrex::Real z_c = 0.0;

  // radial extent of thermal perturbation
  amrex::Real x_r = 0.0;
  amrex::Real y_r = 0.0;
  amrex::Real z_r = 0.0;

  // perturbation temperature
  amrex::Real T_pert = -15.0;
  bool T_pert_is_airtemp = true; // T_pert input is air temperature
  bool perturb_rho = true; // not rho*theta (i.e., p is constant); otherwise perturb rho*theta

  // rayleigh damping
  amrex::Real dampcoef = 0.0; // inverse time scale [1/s]
  amrex::Real zdamp = 5000.0; // damping depth [m] from model top
}; // namespace ProbParm

extern ProbParm parms;

/*
 * Calculate perturbation to potential temperature and density at a
 * given (x,y,z). Note that pressure is not perturbed.
 */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void
perturb_rho_theta (const amrex::Real x,
                   const amrex::Real y,
                   const amrex::Real z,
                   const amrex::Real p_hse,
                   const amrex::Real r_hse,
                   const ProbParm pp,
                   const amrex::Real rdOcp,
                   amrex::Real& rho,
                   amrex::Real& rhotheta)
{
    // Perturbation air temperature
    // - The bubble is either cylindrical (for 2-D problems, if two
    //   radial extents are specified) or an ellipsoid (if all three
    //   radial extents are specified).
    amrex::Real L = 0.0;
    if (pp.x_r > 0) L += std::pow((x - pp.x_c)/pp.x_r, 2);
    if (pp.y_r > 0) L += std::pow((y - pp.y_c)/pp.y_r, 2);
    if (pp.z_r > 0) L += std::pow((z - pp.z_c)/pp.z_r, 2);
    L = std::sqrt(L);
    amrex::Real dT;
    if (L > 1.0) {
        dT = 0.0;
    }
    else {
        dT = pp.T_pert * (std::cos(PI*L) + 1.0)/2.0;
    }

    // Temperature that satisfies the EOS given the hydrostatically balanced (r,p)
    const amrex::Real Tbar_hse = p_hse / (R_d * r_hse);

    // Note: theta_perturbed is theta PLUS perturbation in theta
    amrex::Real theta_perturbed;
    if (pp.T_pert_is_airtemp) {
        // dT is air temperature
        theta_perturbed = (Tbar_hse + dT)*std::pow(p_0/p_hse, rdOcp);
    } else {
        // dT is potential temperature
        theta_perturbed = Tbar_hse*std::pow(p_0/p_hse, rdOcp) + dT;
    }

    if (pp.perturb_rho)
    {
        // this version perturbs rho but not p (i.e., rho*theta)
        // - hydrostatic rebalance is needed
        // - this is the approach taken in the density current problem

        if (pp.T_0 > 0)
        {
            // background field has not been initialized, update full state,
            // need to calculate HSE later
            rhotheta = getRhoThetagivenP(p_hse);
            rho = rhotheta / theta_perturbed;
        } else {
            // already have a background field initialized, need to
            // return rho and rhotheta as deviations from the base state
            rhotheta = 0.0; // i.e., hydrostatically balanced pressure stays const
            rho = getRhoThetagivenP(p_hse) / theta_perturbed - r_hse;
        }
    }
    else
    {
        // this version perturbs rho*theta (i.e., p) but not rho

        if (pp.T_0 > 0)
        {
            // background field has not been initialized, update full state,
            // need to calculate HSE later
            rho = r_hse;
            rhotheta = r_hse * theta_perturbed;
        } else {
            // already have a background field initialized, need to
            // return rho and rhotheta as deviations from the base state
            rho = 0.0; // i.e., hydrostatically balanced density stays const
            rhotheta = r_hse * theta_perturbed - getRhoThetagivenP(p_hse);
        }
    }
}

#endif

