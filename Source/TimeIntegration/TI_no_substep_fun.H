/**
 *  Wrapper for advancing the solution with the slow RHS in the absence of acoustic substepping
 */
    auto no_substep_fun = [&](Vector<MultiFab>& S_sum,
                              Vector<MultiFab>& S_old,
                              Vector<MultiFab>& F_slow,
                              const Real time_for_fp, const Real slow_dt)
    {
        BL_PROFILE("no_substep_fun");
        int n_data = IntVar::NumVars;

        const auto& dxInv = fine_geom.InvCellSizeArray();

        const amrex::GpuArray<int, IntVar::NumVars> scomp_fast = {0,0,0,0};
        const amrex::GpuArray<int, IntVar::NumVars> ncomp_fast = {2,1,1,1};

        // Update S_sum = S_stage only for the fast variables
#ifdef _OPENMP
#pragma omp parallel if (amrex::Gpu::notInLaunchRegion())
#endif
        {
            for ( MFIter mfi(S_sum[IntVar::cons],TilingIfNotGPU()); mfi.isValid(); ++mfi)
            {
                const Box bx = mfi.tilebox();
                Box tbx = surroundingNodes(mfi.tilebox(),0);
                Box tby = surroundingNodes(mfi.tilebox(),1);
                Box tbz = surroundingNodes(mfi.tilebox(),2);

                Vector<Array4<Real> >  ssum_h(n_data);
                Vector<Array4<Real> >  sold_h(n_data);
                Vector<Array4<Real> > fslow_h(n_data);

                for (int i = 0; i < n_data; ++i) {
                     ssum_h[i]  = S_sum[i].array(mfi);
                     sold_h[i]  = S_old[i].array(mfi);
                    fslow_h[i] = F_slow[i].array(mfi);
                }

                Gpu::AsyncVector<Array4<Real> >  sold_d(n_data);
                Gpu::AsyncVector<Array4<Real> >  ssum_d(n_data);
                Gpu::AsyncVector<Array4<Real> > fslow_d(n_data);

                Gpu::copy(Gpu::hostToDevice,  sold_h.begin(),  sold_h.end(),  sold_d.begin());
                Gpu::copy(Gpu::hostToDevice,  ssum_h.begin(),  ssum_h.end(),  ssum_d.begin());
                Gpu::copy(Gpu::hostToDevice, fslow_h.begin(), fslow_h.end(), fslow_d.begin());

                Array4<Real>*  sold =  sold_d.dataPtr();
                Array4<Real>*  ssum =  ssum_d.dataPtr();
                Array4<Real>* fslow = fslow_d.dataPtr();

                // Moving terrain
                if ( solverChoice.use_terrain && solverChoice.terrain_type == 1 )
                {
                    const Array4<const Real>& dJ_old =     detJ_cc[level]->const_array(mfi);
                    const Array4<const Real>& dJ_new = detJ_cc_new[level]->const_array(mfi);

                    const Array4<const Real>& z_nd_old =     z_phys_nd[level]->const_array(mfi);
                    const Array4<const Real>& z_nd_new = z_phys_nd_new[level]->const_array(mfi);

                    const Array4<Real      >& z_t_arr  =  z_t_rk[level]->array(mfi);

                    // We have already scaled the slow source term to have the extra factor of dJ
                    ParallelFor(bx, ncomp_fast[IntVar::cons],
                    [=] AMREX_GPU_DEVICE (int i, int j, int k, int nn) {
                        const int n = scomp_fast[IntVar::cons] + nn;
                        ssum[IntVar::cons](i,j,k,n) = dJ_old(i,j,k) *  sold[IntVar::cons](i,j,k,n)
                                                          + slow_dt * fslow[IntVar::cons](i,j,k,n);
                        ssum[IntVar::cons](i,j,k,n) /= dJ_new(i,j,k);
                    });

                    // We have already scaled the slow source term to have the extra factor of dJ
                    ParallelFor(tbx, tby,
                    [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept {
                        Real h_zeta_old = Compute_h_zeta_AtIface(i, j, k, dxInv, z_nd_old);
                        Real h_zeta_new = Compute_h_zeta_AtIface(i, j, k, dxInv, z_nd_new);
                        ssum[IntVar::xmom](i,j,k) = ( h_zeta_old *  sold[IntVar::xmom](i,j,k)
                                                       + slow_dt * fslow[IntVar::xmom](i,j,k) ) / h_zeta_new;
                    },
                    [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept {
                        Real h_zeta_old = Compute_h_zeta_AtJface(i, j, k, dxInv, z_nd_old);
                        Real h_zeta_new = Compute_h_zeta_AtJface(i, j, k, dxInv, z_nd_new);
                        ssum[IntVar::ymom](i,j,k) = ( h_zeta_old *  sold[IntVar::ymom](i,j,k)
                                                       + slow_dt * fslow[IntVar::ymom](i,j,k) ) / h_zeta_new;
                    });
                    ParallelFor(tbz,
                    [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept {
                        if (k == 0) {
                            // Here we take advantage of the fact that moving terrain has a slip wall
                            // so we can just use the new value at (i,j,0).
                            Real rho_on_face =  ssum[IntVar::cons](i,j,k,Rho_comp);
                            ssum[IntVar::zmom](i,j,k) = WFromOmega(i,j,k,rho_on_face*z_t_arr(i,j,k),
                                                                   ssum[IntVar::xmom], ssum[IntVar::ymom],
                                                                   z_nd_new,dxInv);
                        } else {
                            Real dJ_old_kface = 0.5 * (dJ_old(i,j,k) + dJ_old(i,j,k-1));
                            Real dJ_new_kface = 0.5 * (dJ_new(i,j,k) + dJ_new(i,j,k-1));
                            ssum[IntVar::zmom](i,j,k) = ( dJ_old_kface *  sold[IntVar::zmom](i,j,k)
                                                             + slow_dt * fslow[IntVar::zmom](i,j,k) ) / dJ_new_kface;
                        }
                    });

                } else { // Fixed or no terrain
                    ParallelFor(bx, ncomp_fast[IntVar::cons],
                    [=] AMREX_GPU_DEVICE (int i, int j, int k, int nn) {
                        const int n = scomp_fast[IntVar::cons] + nn;
                        ssum[IntVar::cons](i,j,k,n) = sold[IntVar::cons](i,j,k,n) + slow_dt *
                           ( fslow[IntVar::cons](i,j,k,n) );
                    });
                    ParallelFor(tbx, tby, tbz,
                    [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept {
                        ssum[IntVar::xmom](i,j,k) = sold[IntVar::xmom](i,j,k)
                                                  + slow_dt * fslow[IntVar::xmom](i,j,k);
                    },
                    [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept {
                        ssum[IntVar::ymom](i,j,k) = sold[IntVar::ymom](i,j,k)
                                                  + slow_dt * fslow[IntVar::ymom](i,j,k);
                    },
                    [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept {
                        ssum[IntVar::zmom](i,j,k) = sold[IntVar::zmom](i,j,k)
                                                  + slow_dt * fslow[IntVar::zmom](i,j,k);
                    });
                }
            } // mfi
        } // omp

        // Even if we update all the conserved variables we don't need
        // to fillpatch the slow ones every acoustic substep
        int ng_cons = S_sum[IntVar::cons].nGrow();
        int ng_vel  = S_sum[IntVar::xmom].nGrow();
        bool fast_only          = true;
        bool vel_and_mom_synced = false;
        apply_bcs(S_sum, time_for_fp, ng_cons, ng_vel, fast_only, vel_and_mom_synced);

#ifdef ERF_USE_POISSON_SOLVE
        if (incompressible) {
            project_velocities(S_sum);
        }
#endif
    };
