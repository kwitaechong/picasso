/****************************************************************************
 * Copyright (c) 2021 by the Picasso authors                                *
 * All rights reserved.                                                     *
 *                                                                          *
 * This file is part of the Picasso library. Picasso is distributed under a *
 * BSD 3-clause license. For the licensing terms see the LICENSE file in    *
 * the top-level directory.                                                 *
 *                                                                          *
 * SPDX-License-Identifier: BSD-3-Clause                                    *
 ****************************************************************************/

#ifndef PICASSO_INTERPOLATIONKERNELS_HPP
#define PICASSO_INTERPOLATIONKERNELS_HPP

#include <Picasso_APIC.hpp>
#include <Picasso_BatchedLinearAlgebra.hpp>
#include <Picasso_FieldTypes.hpp>
#include <Picasso_PolyPIC.hpp>

#include <Cabana_Grid.hpp>

#include <type_traits>

namespace Picasso
{
//---------------------------------------------------------------------------//
template <int InterpolationOrder, class ParticleFieldType, class OldFieldType,
          class InterpolationType>
struct Particle2Grid;

//---------------------------------------------------------------------------//
// Project particle enthalpy/momentum to grid. PolyPIC variant
//---------------------------------------------------------------------------//
template <int InterpolationOrder, class ParticleFieldType, class OldFieldType>
struct Particle2Grid<InterpolationOrder, ParticleFieldType, OldFieldType,
                     PolyPicTag>
{
    // Explicit time step.
    double _dt;

    // Constructor
    Particle2Grid( const double dt )
        : _dt( dt )
    {
    }

    template <class LocalMeshType, class GatherDependencies,
              class ScatterDependencies, class LocalDependencies,
              class ParticleViewType>
    KOKKOS_INLINE_FUNCTION void
    operator()( const LocalMeshType& local_mesh, const GatherDependencies&,
                const ScatterDependencies& scatter_deps,
                const LocalDependencies&, ParticleViewType& particle ) const
    {
        // Get particle data.
        auto f_p = Picasso::get( particle, ParticleFieldType() );
        auto v_p = Picasso::get( particle, PolyPIC::Field::Velocity() );
        auto m_p = Picasso::get( particle, Field::Mass() );
        auto x_p = Picasso::get( particle, Field::Position() );

        // Get the scatter dependencies.
        auto m_i =
            scatter_deps.get( Picasso::FieldLocation::Node(), Field::Mass() );
        auto f_i =
            scatter_deps.get( Picasso::FieldLocation::Node(), OldFieldType() );

        // Node interpolant.
        auto spline = Picasso::createSpline(
            Picasso::FieldLocation::Node(),
            Picasso::InterpolationOrder<InterpolationOrder>(), local_mesh, x_p,
            Picasso::SplineValue(), Picasso::SplineDistance() );

        // Interpolate mass and mass-weighted enthalpy/momentum to grid with
        // PolyPIC.
        Picasso::PolyPIC::p2g( m_p, v_p, f_p, f_i, m_i, _dt, spline );
    }
};

//---------------------------------------------------------------------------//
// Project particle enthalpy/momentum to grid. APIC variant
//---------------------------------------------------------------------------//
template <int InterpolationOrder, class ParticleFieldType, class OldFieldType>
struct Particle2Grid<InterpolationOrder, ParticleFieldType, OldFieldType,
                     APicTag>
{
    // Explicit time step.
    double _dt;

    // Constructor
    Particle2Grid( const double dt )
        : _dt( dt )
    {
    }

    template <class LocalMeshType, class GatherDependencies,
              class ScatterDependencies, class LocalDependencies,
              class ParticleViewType>
    KOKKOS_INLINE_FUNCTION void
    operator()( const LocalMeshType& local_mesh, const GatherDependencies&,
                const ScatterDependencies& scatter_deps,
                const LocalDependencies&, ParticleViewType& particle ) const
    {
        // Get particle data.
        auto f_p = Picasso::get( particle, ParticleFieldType() );
        auto m_p = Picasso::get( particle, Field::Mass() );
        auto x_p = Picasso::get( particle, Field::Position() );

        // Get the scatter dependencies.
        auto m_i =
            scatter_deps.get( Picasso::FieldLocation::Node(), Field::Mass() );
        auto f_i =
            scatter_deps.get( Picasso::FieldLocation::Node(), OldFieldType() );

        // Node interpolant.
        auto spline = Picasso::createSpline(
            Picasso::FieldLocation::Node(),
            Picasso::InterpolationOrder<InterpolationOrder>(), local_mesh, x_p,
            Picasso::SplineValue(), Picasso::SplineDistance(),
            Picasso::SplineGradient() );

        // Interpolate mass and mass-weighted enthalpy/momentum to grid with
        // APIC.
        Picasso::APIC::p2g( m_p, f_p, m_i, f_i, spline );
    }
};

//---------------------------------------------------------------------------//
// Project particle enthalpy/momentum to grid. FLIP/PIC variant
//---------------------------------------------------------------------------//
template <int InterpolationOrder, class ParticleFieldType, class OldFieldType>
struct Particle2Grid<InterpolationOrder, ParticleFieldType, OldFieldType,
                     FlipTag>
{
    // Explicit time step.
    double _dt;

    // Constructor
    Particle2Grid( const double dt )
        : _dt( dt )
    {
    }

    template <class LocalMeshType, class GatherDependencies,
              class ScatterDependencies, class LocalDependencies,
              class ParticleViewType>
    KOKKOS_INLINE_FUNCTION void
    operator()( const LocalMeshType& local_mesh, const GatherDependencies&,
                const ScatterDependencies& scatter_deps,
                const LocalDependencies&, ParticleViewType& particle ) const
    {
        // Get particle data.
        auto f_p = Picasso::get( particle, ParticleFieldType() );
        auto m_p = Picasso::get( particle, Field::Mass() );
        auto x_p = Picasso::get( particle, Field::Position() );

        // Get the scatter dependencies.
        auto m_i =
            scatter_deps.get( Picasso::FieldLocation::Node(), Field::Mass() );
        auto f_i =
            scatter_deps.get( Picasso::FieldLocation::Node(), OldFieldType() );

        // Node interpolant.
        auto spline = Picasso::createSpline(
            Picasso::FieldLocation::Node(),
            Picasso::InterpolationOrder<InterpolationOrder>(), local_mesh, x_p,
            Picasso::SplineValue(), Picasso::SplineDistance() );

        // Interpolate mass and mass-weighted enthalpy/momentum to grid.
        Picasso::P2G::value( spline, m_p, m_i );
        Picasso::P2G::value( spline, m_p * f_p, f_i );
    }
};

template <int InterpolationOrder, class InterpolationType>
struct Grid2ParticleVelocity;

//---------------------------------------------------------------------------//
// Update particle state. PolyPIC variant.
//---------------------------------------------------------------------------//
template <int InterpolationOrder>
struct Grid2ParticleVelocity<InterpolationOrder, PolyPicTag>
{
    // Explicit time step.
    double _dt;

    // Primary constructor
    Grid2ParticleVelocity( const double dt )
        : _dt( dt )
    {
    }

    // Constructor for easier compatibility with FLIP
    Grid2ParticleVelocity( const double dt, const double )
        : _dt( dt )
    {
    }

    template <class LocalMeshType, class GatherDependencies,
              class ScatterDependencies, class LocalDependencies,
              class ParticleViewType>
    KOKKOS_INLINE_FUNCTION void
    operator()( const LocalMeshType& local_mesh,
                const GatherDependencies& gather_deps,
                const ScatterDependencies&, const LocalDependencies& local_deps,
                ParticleViewType& particle ) const
    {
        // Get particle data.
        auto u_p = Picasso::get( particle, PolyPIC::Field::Velocity() );
        auto x_p = Picasso::get( particle, Field::Position() );

        // Get the gather dependencies.
        auto m_i =
            gather_deps.get( Picasso::FieldLocation::Node(), Field::Mass() );
        auto u_i = gather_deps.get( Picasso::FieldLocation::Node(),
                                    Field::Velocity() );
        // Get the local dependencies for getting physcial location of node
        auto x_i = local_deps.get( Picasso::FieldLocation::Node(),
                                   Picasso::Field::PhysicalPosition<3>() );

        // Node interpolant.
        auto spline = Picasso::createSpline(
            Picasso::FieldLocation::Node(),
            Picasso::InterpolationOrder<InterpolationOrder>(), local_mesh, x_p,
            Picasso::SplineValue(), Picasso::SplineGradient() );

        // Update particle velocity using a PolyPIC update.
        Picasso::PolyPIC::g2p( u_i, u_p, spline );

        // Update particle position.
        auto x_i_updated =
            [=]( const int i, const int j, const int k, const int d )
        { return x_i( i, j, k, d ) + _dt * u_i( i, j, k, d ); };
        Picasso::G2P::value( spline, x_i_updated, x_p );
    }
};

//---------------------------------------------------------------------------//
// Update particle state. APIC variant.
//---------------------------------------------------------------------------//
template <int InterpolationOrder>
struct Grid2ParticleVelocity<InterpolationOrder, APicTag>
{
    // Explicit time step.
    double _dt;

    // Primary constructor
    Grid2ParticleVelocity( const double dt )
        : _dt( dt )
    {
    }

    // Constructor for easier compatibility with FLIP
    Grid2ParticleVelocity( const double dt, const double )
        : _dt( dt )
    {
    }

    template <class LocalMeshType, class GatherDependencies,
              class ScatterDependencies, class LocalDependencies,
              class ParticleViewType>
    KOKKOS_INLINE_FUNCTION void
    operator()( const LocalMeshType& local_mesh,
                const GatherDependencies& gather_deps,
                const ScatterDependencies&, const LocalDependencies& local_deps,
                ParticleViewType& particle ) const
    {
        // Get particle data.
        auto f_p = Picasso::get( particle, APIC::Field::Velocity() );
        auto x_p = Picasso::get( particle, Field::Position() );

        // Get the gather dependencies.
        auto m_i =
            gather_deps.get( Picasso::FieldLocation::Node(), Field::Mass() );
        auto u_i = gather_deps.get( Picasso::FieldLocation::Node(),
                                    Field::Velocity() );
        // Get the local dependencies for getting physcial location of node
        auto x_i = local_deps.get( Picasso::FieldLocation::Node(),
                                   Picasso::Field::PhysicalPosition<3>() );

        // Node interpolant.
        auto spline = Picasso::createSpline(
            Picasso::FieldLocation::Node(),
            Picasso::InterpolationOrder<InterpolationOrder>(), local_mesh, x_p,
            Picasso::SplineValue(), Picasso::SplineDistance(),
            Picasso::SplineGradient() );

        // Update particle velocity using a APIC update.
        Picasso::APIC::g2p( u_i, f_p, spline );

        // Update particle position.
        auto x_i_updated =
            [=]( const int i, const int j, const int k, const int d )
        { return x_i( i, j, k, d ) + _dt * u_i( i, j, k, d ); };
        Picasso::G2P::value( spline, x_i_updated, x_p );
    }
};

//---------------------------------------------------------------------------//
// Update particle state. FLIP/PIC variant.
//---------------------------------------------------------------------------//
template <int InterpolationOrder>
struct Grid2ParticleVelocity<InterpolationOrder, FlipTag>
{
    // Explicit time step.
    double _dt;
    // FLIP/PIC ratio
    double _beta = 0.99;

    // Primary constructor
    Grid2ParticleVelocity( const double dt, const double beta )
        : _dt( dt )
        , _beta( beta )
    {
    }

    // Default beta constructor
    Grid2ParticleVelocity( const double dt )
        : _dt( dt )
    {
    }

    template <class LocalMeshType, class GatherDependencies,
              class ScatterDependencies, class LocalDependencies,
              class ParticleViewType, class PositionType = Field::Position,
              class VelocityType = Field::Velocity>
    KOKKOS_INLINE_FUNCTION void
    operator()( const LocalMeshType& local_mesh,
                const GatherDependencies& gather_deps,
                const ScatterDependencies&, const LocalDependencies& local_deps,
                ParticleViewType& particle ) const
    {
        // Get particle data.
        auto u_p = Picasso::get( particle, VelocityType{} );
        auto x_p = Picasso::get( particle, PositionType{} );

        // Get the gather dependencies.
        auto m_i =
            gather_deps.get( Picasso::FieldLocation::Node(), Field::Mass() );
        auto u_i = gather_deps.get( Picasso::FieldLocation::Node(),
                                    Field::Velocity() );
        auto old_u_i =
            gather_deps.get( Picasso::FieldLocation::Node(), Field::OldU() );

        // Get the local dependencies for getting physcial location of node
        auto x_i = local_deps.get( Picasso::FieldLocation::Node(),
                                   Picasso::Field::PhysicalPosition<3>() );

        // Node interpolant.
        auto spline = Picasso::createSpline(
            Picasso::FieldLocation::Node(),
            Picasso::InterpolationOrder<InterpolationOrder>(), local_mesh, x_p,
            Picasso::SplineValue(), Picasso::SplineGradient() );

        // Update particle velocity using a hybrid PIC/FLIP update.
        // Note the need for 4 indices here because this is passed to the Cabana
        // grid.
        auto d_u_i = [=]( const int i, const int j, const int k, const int d )
        {
            return ( m_i( i, j, k ) > 0.0 )
                       ? u_i( i, j, k, d ) - old_u_i( i, j, k, d )
                       : 0.0;
        };

        Picasso::Vec3<double> u_p_pic;
        // auto u_i_pic = [=]( const int i, const int j, const int k,
        //                    const int d ) { return u_i( i, j, k, d ); };
        Picasso::G2P::value( spline, u_i, u_p_pic );

        Picasso::Vec3<double> d_u_p;
        Picasso::G2P::value( spline, d_u_i, d_u_p );

        Picasso::Vec3<double> u_p_flip;

        u_p_flip = u_p + d_u_p;

        // Update particle velocity.
        u_p = _beta * u_p_flip + ( 1.0 - _beta ) * u_p_pic;

        // Update particle position.
        auto x_i_updated =
            [=]( const int i, const int j, const int k, const int d )
        { return x_i( i, j, k, d ) + _dt * u_i( i, j, k, d ); };
        Picasso::G2P::value( spline, x_i_updated, x_p );
    }
};

} // end namespace Picasso

#endif // end PICASSO_INTERPOLATIONKERNELS_HPP