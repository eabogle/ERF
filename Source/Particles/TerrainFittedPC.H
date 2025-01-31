#ifndef TERRAIN_FITTED_PC_H_
#define TERRAIN_FITTED_PC_H_

#include <AMReX_Particles.H>

struct RealIdx
{
    enum {
        vx = 0,
        vy, vz,
        ncomps
    };
};

struct IntIdx
{
    enum {
        k = 0,
        ncomps
    };
};

class TerrainFittedPC
    : public amrex::ParticleContainer<RealIdx::ncomps, IntIdx::ncomps>
{

public:

    TerrainFittedPC (amrex::ParGDBBase* gdb)
        : amrex::ParticleContainer<RealIdx::ncomps, IntIdx::ncomps>(gdb)
        {}

    TerrainFittedPC (const amrex::Geometry            & geom,
                     const amrex::DistributionMapping & dmap,
                     const amrex::BoxArray            & ba)
        : amrex::ParticleContainer<RealIdx::ncomps, IntIdx::ncomps>(geom, dmap, ba)
        {}

    void InitParticles (const amrex::MultiFab& a_z_height);

    void AdvectWithUmac (amrex::MultiFab* umac, int level, amrex::Real dt,
                         const amrex::MultiFab& a_z_height);
};

#endif
