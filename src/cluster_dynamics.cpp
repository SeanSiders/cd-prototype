#include <stdio.h>
#include <cstring>

#include "material.hpp"
#include "nuclear_reactor.hpp"

#include "cluster_dynamics.hpp"

// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 1a-1e
    The rate of production of interstital defects from the irradiation cascade for size (n) clusters.
*/
__CUDADECL__ double ClusterDynamics::i_defect_production(size_t n)
{
    switch (n)
    {
        case 1: return reactor.recombination * reactor.flux *
                       (1. - reactor.i_bi - reactor.i_tri - reactor.i_quad);
        case 2: return reactor.recombination * reactor.flux * reactor.i_bi;
        case 3: return reactor.recombination * reactor.flux * reactor.i_tri;
        case 4: return reactor.recombination * reactor.flux * reactor.i_quad;

        default: break;
    }

    // cluster sizes > greater than 4 always zero
    return 0.;
};

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 1a-1e
    The rate of production of vacancy defects from the irradiation cascade for size (n) clusters.
*/
__CUDADECL__ double ClusterDynamics::v_defect_production(size_t n)
{
    switch (n)
    {
        case 1: return reactor.recombination * reactor.flux *
                       (1. - reactor.v_bi - reactor.v_tri - reactor.v_quad);
        case 2: return reactor.recombination * reactor.flux * reactor.v_bi;
        case 3: return reactor.recombination * reactor.flux * reactor.v_tri;
        case 4: return reactor.recombination * reactor.flux * reactor.v_quad;

        default: break;
    }

    // cluster sizes > greater than 4 always zero
    return 0.;
};
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 2a
    The number of clusters that contain (in) interstitials per unit volume.

                  (1)     (2)                    (3)              (4)
    dCi(n) / dt = Gi(n) + a[i,n+1] * Ci(n + 1) - b[i,n] * Ci(n) + c[i,n-1] * Ci(n-1)
*/
__CUDADECL__ double ClusterDynamics::i_clusters_delta(size_t in)
{
    return
        // (1)
        i_defect_production(in)
        // (2)
        + iemission_vabsorption_np1(in + 1) * interstitials[in + 1]
        // // (3)
        - iemission_vabsorption_n(in) * interstitials[in]
        // // (4)
        + iemission_vabsorption_nm1(in - 1) * interstitials[in - 1];
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 2a
    The number of clusters that contain (vn) vacancies per unit volume.

                  (1)     (2)                    (3)              (4)
    dCv(n) / dt = Gv(n) + a[v,n+1] * Cv(n + 1) - b[v,n] * Cv(n) + c[v,n-1] * Cv(n-1)
*/
__CUDADECL__ double ClusterDynamics::v_clusters_delta(size_t vn)
{
    return
        // (1)
        v_defect_production(vn)
        // (2)
        + vemission_iabsorption_np1(vn + 1) * vacancies[vn + 1]
        // (3)
        - vemission_iabsorption_n(vn) * vacancies[vn]
        // (4)
        + vemission_iabsorption_nm1(vn - 1) * vacancies[vn - 1];
}
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 2b
    The combined rate of emission of an interstitial and absorption of a vacancy by an interstitial loop of size (np1),
    both events leading to an interstitial loop of size n.

               (1)             (2)     (3)
    a[i,n+1] = B[i,v](n + 1) * Cv(1) + E[i,i](n + 1)
*/
__CUDADECL__ double ClusterDynamics::iemission_vabsorption_np1(size_t np1)
{
    return
        // (1)
        iv_absorption(np1) *
        // (2)
        vacancies[1] + 
        // (3)
        ii_emission(np1);
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 2b
    The combined rate of emission of an interstitial and absorption of a vacancy by an interstitial loop of size (np1),
    both events leading to an interstitial loop of size n.

               (1)             (2)     (3)
    a[v,n+1] = B[v,i](n + 1) * Ci(1) + E[v,v](n + 1)
*/
__CUDADECL__ double ClusterDynamics::vemission_iabsorption_np1(size_t np1)
{
    return 
        // (1)
        vi_absorption(np1) * 
        // (2)
        interstitials[1] + 
        // (3)
        vv_emission(np1);
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 2c
    The rate that a loop of size n can evolve toward a loop of size
    n + 1 absorbing an interstitial, or toward a loop of size
    n - 1 absorbing a vacancy or emitting an interstitial.

             (1)                 (2)                 (3)
    b[i,n] = B[i,v](n) * Cv(1) + B[i,i](n) * Ci(1) + E[i,i](n)
*/
__CUDADECL__ double ClusterDynamics::iemission_vabsorption_n(size_t n)
{
    return
        // (1)
        iv_absorption(n) * vacancies[1]
        // (2)
        + ii_absorption(n) * interstitials[1] * (1 - dislocation_promotion_probability(n))
        // (3)
        + ii_emission(n);
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 2c
    The rate that a loop of size n can evolve toward a loop of size
    n + 1 absorbing an vacancy, or toward a loop of size
    n - 1 absorbing an interstitial or emitting a vacancy.

             (1)                 (2)                 (3)
    b[v,n] = B[v,i](n) * Ci(1) + B[v,v](n) * Cv(1) + E[v,v](n)
*/
__CUDADECL__ double ClusterDynamics::vemission_iabsorption_n(size_t n)
{
    return 
        // (1)
        vi_absorption(n) * interstitials[1] 
        // (2)
        + vv_absorption(n) * vacancies[1]
        // (3)
        + vv_emission(n);
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 2d
    The rate that an interstitial loop of size n - 1 can evolve into a
    loop of size n by absorbing an interstitial.

               (1)           (2)
    c[i,n-1] = B[i,i](n-1) * Ci(1)
*/
__CUDADECL__ double ClusterDynamics::iemission_vabsorption_nm1(size_t nm1)
{
    return
        // (1)
        ii_absorption(nm1) *
        // (2)
        interstitials[1];
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 2d
    The rate that a vacancy loop of size n - 1 can evolve into a
    loop of size n by absorbing a vacancy.

               (1)           (2)
    c[v,n-1] = B[v,v](n-1) * Cv(1)
*/
__CUDADECL__ double ClusterDynamics::vemission_iabsorption_nm1(size_t nm1)
{
    return
        // (1)
        vv_absorption(nm1) * 
        // (2)
        vacancies[1];
}
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 3a
    Point defects concentrations per unit volume given a cluster size (in).
    ** in is only used in characteristic time calculations which rely on cluster sizes up to (in).

                (1)     (2)
    dCi(1)/dt = Gi(1) - Riv * Ci(1) * Cv(1) - 
            (3)
            Ci(1) / (tAdi) - 
            (4)
            Ci(1) / (tAgb) -
            (5)
            Ci(1) / (tAi) +
            (6)
            1 / (tEi)
*/
__CUDADECL__ double ClusterDynamics::i1_cluster_delta(size_t nmax)
{
    return 
        // (1)
        i_defect_production(1)
        // (2)
        - annihilation_rate() * interstitials[1] * vacancies[1]
        // (3)
        - interstitials[1] * i_dislocation_annihilation_time()
        // (4)
        - interstitials[1] * i_grain_boundary_annihilation_time(nmax)
        // (5)
        - interstitials[1] * i_absorption_time(nmax)
        // (6)
        + i_emission_time(nmax);
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 3a
    Point defects concentrations per unit volume given a cluster size (vn).
    ** vn is only used in characteristic time calculations which rely on cluster sizes up to (vn).

            (1)     (2)
    Cv(1) = Gv(1) - Riv * Ci(1) * Cv(1) - 
            (3)
            Cv(1) / (tAdv) - 
            (4)
            Cv(1) / (tAgb) -
            (5)
            Cv(1) / (tAv) +
            (6)
            1 / (tEv)
*/
__CUDADECL__ double ClusterDynamics::v1_cluster_delta(size_t nmax)
{
    return 
        // (1)
        v_defect_production(1)
        // (2)
        - annihilation_rate() * interstitials[1] * vacancies[1]
        // (3)
        - vacancies[1] * v_dislocation_annihilation_time()
        // (4)
        - vacancies[1] * v_grain_boundary_annihilation_time(nmax)
        // (5)
        - vacancies[1] * v_absorption_time(nmax)
        // (6)
        + v_emission_time(nmax);
}
// --------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 3b
    Characteristic time for emitting an interstitial by the population of interstital or vacancy
    clusters of size up to (nmax).

                (1)                      (2)                     (3)
    tEi(n) = SUM ( E[i,i](n) * Ci(n) ) + 4 * E[i,i](2) * Ci(2) + B[i,v](2) * Cv(2) * Ci(2)
*/
double ClusterDynamics::i_emission_time(size_t nmax)
{
    double time = 0.;
    for (size_t in = 3; in < nmax; ++in)
    {
          time +=
            // (1)
            ii_emission(in) * interstitials[in];
    }

    time +=
        // (2)
        4 * ii_emission(2) * interstitials[2]
        // (3)
        + iv_absorption(2) * vacancies[2] * interstitials[2];

    return time;
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 3b
    Characteristic time for emitting a vacancy by the population of interstital or vacancy
    clusters of size up to (nmax).

                (1)                           (2)                     (3)
    tEv(n) = SUM[n>0] ( E[v,v](n) * Cv(n) ) + 4 * E[v,v](2) * Cv(2) + B[v,i](2) * Cv(2) * Ci(2)
*/
double ClusterDynamics::v_emission_time(size_t nmax)
{
    double time = 0.;
    for (size_t vn = 3; vn < nmax; ++vn)
    {
        time += 
            // (1)
            vv_emission(vn) * vacancies[vn];
    }

    time +=
        // (2)
        4 * vv_emission(2) * vacancies[2]
        // (3)
        + vi_absorption(2) * vacancies[2] * interstitials[2];

    return time;
}
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 3c
    Characteristic time for absorbing an interstitial by the population of interstital or vacancy
    clusters of size up to (nmax).

                        (1)                              (2)
    tAi(n) = SUM[n>0] ( B[i,i](n) * Ci(n) ) + SUM[n>1] ( B[v,i](n) * Cv(n) )
*/
double ClusterDynamics::i_absorption_time(size_t nmax)
{
    double time = ii_absorption(1) * interstitials[1];
    for (size_t in = 1; in < nmax; ++in)
    {
        time +=
            // (1)
            ii_absorption(in) * interstitials[in]
            // (2)
            + vi_absorption(in) * vacancies[in];
    }

    return time;
}


/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 3c
    Characteristic time for absorbing a vacancy by the population of interstital or vacancy
    clusters of size up to (nmax).

                        (1)                              (2)
    tAv(n) = SUM[n>0] ( B[v,v](n) * Cv(n) ) + SUM[n>1] ( B[i,v](n) * Ci(n) )
*/
double ClusterDynamics::v_absorption_time(size_t nmax)
{
    double time = vv_absorption(1) * vacancies[1];
    for (size_t vn = 1; vn < nmax; ++vn)
    {
        time +=
            // (1)
            vv_absorption(vn) * vacancies[vn]
            // (2)
            + iv_absorption(vn) * interstitials[vn];
    }

    return time;
}
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 3d
    Annihilation rate of vacancies and insterstitals.

          (1)      (2)         (3)
    Riv = 4 * PI * (Di + Dv) * riv
*/
__CUDADECL__ double ClusterDynamics::annihilation_rate()
{
    return 
        // (1)
        4 * M_PI * 
        // (2)
        (i_diffusion() + v_diffusion()) *
        // (3)
        material.recombination_radius;
}
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 3e
    Characteristic time for annihilation of interstitials on dislocations.

           (1)   (2)    (3)
    tAdi = p  *  Di  *  Zi
*/
__CUDADECL__ double ClusterDynamics::i_dislocation_annihilation_time()
{
    return
        // (1)
        dislocation_density *
        // (2)
        i_diffusion() *
        // (3)
        material.i_dislocation_bias;
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 3e
    Characteristic time for annihilation of vacancies on dislocations.

           (1)   (2)    (3)
    tAdv = p  *  Dv  *  Zv
*/
__CUDADECL__ double ClusterDynamics::v_dislocation_annihilation_time()
{
    return
        // (1)
        dislocation_density *
        // (2)
        v_diffusion() *
        // (3)
        material.v_dislocation_bias;
}
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 3f
    Characteristic time for annihilation of interstitials on grain boundaries.

           (1)            (2)      (3)                            (4)                              (5)
    tAdi = 6 * Di * sqrt( p * Zi + SUM[n] ( B[i,i](n) * Ci(n) ) + SUM[n] ( B[v,i](n) * Cv(n) ) ) / d
*/
__CUDADECL__ double ClusterDynamics::i_grain_boundary_annihilation_time(size_t in)
{
    return
        // (1)
        6 * i_diffusion() *
        sqrt
        (
            // (2)
            dislocation_density * 
            material.i_dislocation_bias
            // (3)
            + ii_sum_absorption(in)
            // (4)
            + vi_sum_absorption(in)
        ) /
        // (5)
        material.grain_size;
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 3f
    Characteristic time for annihilation of interstitials on grain boundaries.

           (1)            (2)      (3)                            (4)                              (5)
    tAdv = 6 * Di * sqrt( p * Zv + SUM[n] ( B[v,v](n) * Cv(n) ) + SUM[n] ( B[i,v](n) * Ci(n) ) ) / d
*/
__CUDADECL__ double ClusterDynamics::v_grain_boundary_annihilation_time(size_t vn)
{
    return
        // (1)
        6 * v_diffusion() *
        sqrt
        (
            // (2)
            dislocation_density *
            material.v_dislocation_bias
            // (3)
            + vv_sum_absorption(vn)
            // (4)
            + iv_sum_absorption(vn)
        ) /
        // (5)
        material.grain_size;
}

double ClusterDynamics::ii_sum_absorption(size_t nmax)
{
    double emission = 0.;
    for (size_t vn = 1; vn < nmax; ++vn)
    {
        emission += ii_absorption(vn) * interstitials[vn];
    }

    return emission;
}

double ClusterDynamics::iv_sum_absorption(size_t nmax)
{
    double emission = 0.;
    for (size_t n = 1; n < nmax; ++n)
    {
        emission += iv_absorption(n) * interstitials[n];
    }

    return emission;
}

double ClusterDynamics::vv_sum_absorption(size_t nmax)
{
    double emission = 0.;
    for (size_t n = 1; n < nmax; ++n)
    {
        emission += vv_absorption(n) * vacancies[n];
    }

    return emission;
}

double ClusterDynamics::vi_sum_absorption(size_t nmax)
{
    double emission = 0.;
    for (size_t n = 1; n < nmax; ++n)
    {
        emission += vi_absorption(n) * vacancies[n];
    }

    return emission;
}

// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 4a
    Rate of emission of an interstitial by an interstital loop of size (n).
*/
__CUDADECL__ double ClusterDynamics::ii_emission(size_t n)
{
    return 
        2 * M_PI * cluster_radius(n) *
        i_bias_factor(n) *
        i_diffusion() / material.atomic_volume *
        exp
        (
            -i_binding_energy(n) /
            (BOLTZMANN_EV_KELVIN * reactor.temperature)
        );
}


/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 4b
    Rate of absorption of an interstitial by an interstital loop of size (n).
*/
__CUDADECL__ double ClusterDynamics::ii_absorption(size_t n)
{
    return 
        2 * M_PI * cluster_radius(n) *
        i_bias_factor(n) *
        i_diffusion();
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 4c
    Rate of absorption of an interstitial by a vacancy loop of size (n).
*/
__CUDADECL__ double ClusterDynamics::iv_absorption(size_t n)
{
    return 
        2 * M_PI * cluster_radius(n) *
        v_bias_factor(n) *
        v_diffusion();
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 4d
    Rate of emission of a vacancy by a vacancy loop of size (n).
*/
__CUDADECL__ double ClusterDynamics::vv_emission(size_t n)
{
    return 
        2 * M_PI * cluster_radius(n) *
        v_bias_factor(n) *
        v_diffusion() *
        exp
        (
            -v_binding_energy(n) /
            (BOLTZMANN_EV_KELVIN * reactor.temperature)
        );
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 4e
    Rate of absorption of a vacancy by a vacancy loop of size (n).
*/
__CUDADECL__ double ClusterDynamics::vv_absorption(size_t n)
{
    return 
        2 * M_PI * cluster_radius(n) *
        v_bias_factor(n) *
        v_diffusion();
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 4f
    Rate of absorption of a vacancy by an interstitial loop of size (n).
*/
__CUDADECL__ double ClusterDynamics::vi_absorption(size_t n)
{
    return 
        2 * M_PI * cluster_radius(n) *
        i_bias_factor(n) *
        i_diffusion();
}
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 5
    Interstitial bias factor.
*/
__CUDADECL__ double ClusterDynamics::i_bias_factor(size_t in)
{
    return 
        material.i_dislocation_bias +
        (
            std::sqrt
            (
                    material.burgers_vector /
                    (8 * M_PI * material.lattice_param)
            ) *
            material.i_loop_bias -
            material.i_dislocation_bias
        ) *
        1 /
        std::pow
        (
            in,
            material.i_dislocation_bias_param / 2.
        );
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 5
    Vacancy bias factor.
*/
__CUDADECL__ double ClusterDynamics::v_bias_factor(size_t vn)
{
    return 
        material.v_dislocation_bias +
        (
            std::sqrt
            (
                    material.burgers_vector /
                    (8 * M_PI * material.lattice_param)
            ) *
            material.v_loop_bias -
            material.v_dislocation_bias
        ) *
        1 /
        std::pow
        (
            vn,
            material.v_dislocation_bias_param / 2.
        );
}
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 6
    Interstitial binding energy.
*/
__CUDADECL__ double ClusterDynamics::i_binding_energy(size_t in)
{
    return
        material.i_formation
        + (material.i_binding - material.i_formation) / (std::pow(2., .8) - 1) *
        (std::pow(in, .8) - std::pow(in - 1., .8));
}

/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 6
    Vacancy binding energy.
*/
__CUDADECL__ double ClusterDynamics::v_binding_energy(size_t vn)
{
    return
        material.v_formation
        + (material.v_binding - material.v_formation) / (std::pow(2., .8) - 1) *
        (std::pow(vn, .8) - std::pow(vn - 1., .8));
}
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
/*  G. Was / Fundamentals of Radiation Materials Science (2nd Edition) (2017), pg 193, 4.59
*/  
__CUDADECL__ double ClusterDynamics::i_diffusion()
{
    return material.i_diffusion_0 * std::exp(-material.i_migration / (BOLTZMANN_EV_KELVIN * reactor.temperature));
}

/*  G. Was / Fundamentals of Radiation Materials Science (2nd Edition) (2017), pg 193, 4.59
*/  
__CUDADECL__ double ClusterDynamics::v_diffusion()
{
    return material.v_diffusion_0 * std::exp(-material.v_migration / (BOLTZMANN_EV_KELVIN * reactor.temperature));
}
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
/*  N. Sakaguchi / Acta Materialia 1131 (2001), 3.10
*/
__CUDADECL__ double ClusterDynamics::mean_dislocation_cell_radius()
{
    double r_0_factor = 0.;
    for (size_t i = 1; i < concentration_boundary; ++i)
    {
        r_0_factor += cluster_radius(i) * interstitials[i];
    }

    return 1 / std::sqrt((2 * M_PI * M_PI / material.atomic_volume) * r_0_factor + M_PI * dislocation_density);
}

// --------------------------------------------------------------------------------------------
/*  N. Sakaguchi / Acta Materialia 1131 (2001), 3.12
*/
__CUDADECL__ double ClusterDynamics::dislocation_promotion_probability(size_t n)
{
    double dr = cluster_radius(n + 1) - cluster_radius(n);

    return (2 * cluster_radius(n) * dr + std::pow(dr, 2)) 
         / (M_PI * mean_dislocation_radius_val / 2. - std::pow(cluster_radius(n), 2)); 
}
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
/*  C. Pokor / Journal of Nuclear Materials 326 (2004), 8
*/
double ClusterDynamics::dislocation_density_delta()
{
    double gain = 0.0;
    for (size_t n = 1; n < concentration_boundary; ++n)
    {
        gain += cluster_radius(n) * dislocation_promotion_probability(n) * ii_absorption(n) * interstitials[n];
    }

    gain *= 2 * M_PI / material.atomic_volume;

    return 
        gain
        - reactor.dislocation_density_evolution * 
        std::pow(material.burgers_vector, 2) *
        std::pow(dislocation_density, 3./2.);
}

// --------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------
/*  G. Was / Fundamentals of Radiation Materials Science (2nd Edition) (2017), pg. 346, 7.63
*/
__CUDADECL__ double ClusterDynamics::cluster_radius(size_t n)
{
    return std::sqrt(std::sqrt(3.0) * std::pow(material.lattice_param, 2) * (double)n / (4 * M_PI));
}
// --------------------------------------------------------------------------------------------

Material ClusterDynamics::get_material()
{
    return material;
}

void ClusterDynamics::set_material(Material material)
{
    this->material = material;
}

NuclearReactor ClusterDynamics::get_reactor()
{
    return reactor;
}

void ClusterDynamics::set_reactor(NuclearReactor reactor)
{
    this->reactor = reactor;
}



// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
/*
 *  SIMULATION CONTROL FUNCTIONS
 */
// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------


bool ClusterDynamics::software_update_clusters(double delta_time)
{
    bool state_is_valid = true;

    for (size_t n = 2; n < concentration_boundary; ++n)
    {
        interstitials_temp[n] += i_clusters_delta(n) * delta_time;
        vacancies_temp[n] += v_clusters_delta(n) * delta_time;

        state_is_valid = state_is_valid && validate(n);
    }

    return state_is_valid;
}

bool ClusterDynamics::update_clusters(double delta_time)
{
    mean_dislocation_radius_val = mean_dislocation_cell_radius();

    // Calculate the change in size 1 defects
    interstitials_temp[1] += i1_cluster_delta(concentration_boundary - 1) * delta_time;
    vacancies_temp[1] += v1_cluster_delta(concentration_boundary - 1) * delta_time;
    bool state_is_valid = validate(1);

    #ifdef USE_CUDA
        state_is_valid = state_is_valid && CUDA_update_clusters(delta_time);
    #else
        state_is_valid = state_is_valid && software_update_clusters(delta_time);
    #endif

    dislocation_density += dislocation_density_delta() * delta_time;

    std::memcpy(interstitials, interstitials_temp, concentration_boundary * sizeof(double));
    std::memcpy(vacancies, vacancies_temp, concentration_boundary * sizeof(double));

    return state_is_valid;
}

bool ClusterDynamics::validate(size_t n)
{
    return 
        !std::isnan(interstitials_temp[n]) &&
        !std::isinf(interstitials_temp[n]) &&
        !std::isnan(vacancies_temp[n]) &&
        !std::isinf(vacancies_temp[n]) &&
        !(interstitials_temp[n] < 0) &&
        !(vacancies_temp[n] < 0);
}





// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
/*
 *  PUBLIC INTERFACE FUNCTIONS
 */
// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------




ClusterDynamics::ClusterDynamics(size_t concentration_boundary, NuclearReactor reactor, Material material)
: concentration_boundary(concentration_boundary), reactor(reactor), material(material)
{
    interstitials = new double[concentration_boundary];
    vacancies = new double[concentration_boundary];
    interstitials_temp = new double[concentration_boundary];
    vacancies_temp = new double[concentration_boundary];
    dislocation_density = material.dislocation_density_0;
}

ClusterDynamicsState ClusterDynamics::run(double delta_time, double total_time)
{
    bool state_is_valid = true;
    for (double endtime = time + total_time; time < endtime; time += delta_time)
    {
        state_is_valid = update_clusters(delta_time);
        if (!state_is_valid) break;
    }

    return ClusterDynamicsState 
    {
        .valid = state_is_valid,
        .time = time,
        .interstitials = std::vector<double>(interstitials, interstitials + concentration_boundary),
        .vacancies = std::vector<double>(vacancies, vacancies + concentration_boundary),
        .dislocation_density = dislocation_density
    };
}




// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
/*
 *  CUDA FUNCTIONS
 */
// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------



#ifdef USE_CUDA

__CUDADECL__ void ClusterDynamics::update_cluster_n(size_t n, double delta_time)
{
  if (n >= concentration_boundary) return;
  
  interstitials_temp[n] += i_clusters_delta(n) * delta_time;
  vacancies_temp[n] += v_clusters_delta(n) * delta_time;
}

__global__ void update_clusters_kernel(ClusterDynamics* simulation, double* is_in, double* is_out, double* vs_in, double* vs_out, double delta_time)
{
  simulation->interstitials = is_in;
  simulation->interstitials_temp = is_out;
  simulation->vacancies = vs_in;
  simulation->vacancies_temp = vs_out;

  size_t index = 32 * blockIdx.x + threadIdx.x + 2; // Idx 0 is empty, 1 is handled outside CUDA
  simulation->update_cluster_n(index, delta_time);
}

__host__ bool ClusterDynamics::CUDA_update_clusters(double delta_time)
{
  double* d_is_in;
  double* d_is_out;
  double* d_vs_in;
  double* d_vs_out;
  ClusterDynamics* d_simulation;

  size_t arraysize = concentration_boundary * sizeof(double);

  cudaMalloc((void**)&d_is_in, arraysize);
  cudaMalloc((void**)&d_is_out, arraysize);
  cudaMalloc((void**)&d_vs_in, arraysize);
  cudaMalloc((void**)&d_vs_out, arraysize);
  cudaMalloc((void**)&d_simulation, sizeof(ClusterDynamics));

  cudaMemcpy(d_is_in, interstitials, arraysize, cudaMemcpyHostToDevice);
  cudaMemcpy(d_vs_in, vacancies, arraysize, cudaMemcpyHostToDevice);
  cudaMemcpy(d_is_out, interstitials_temp, arraysize, cudaMemcpyHostToDevice);
  cudaMemcpy(d_vs_out, vacancies_temp, arraysize, cudaMemcpyHostToDevice);

  cudaMemcpy(d_simulation, this, sizeof(ClusterDynamics), cudaMemcpyHostToDevice);

  update_clusters_kernel<<<concentration_boundary - 2, 32>>>(d_simulation, d_is_in, d_is_out, d_vs_in, d_vs_out, delta_time);
  cudaError_t error = cudaGetLastError();
  if (error != cudaSuccess)
  {
    fprintf(stderr, "\nCUDA Error: %s", cudaGetErrorString(error));
  }

  cudaMemcpy(interstitials_temp + 2, d_is_out + 2, arraysize - 2 * sizeof(double), cudaMemcpyDeviceToHost);
  cudaMemcpy(vacancies_temp + 2, d_vs_out + 2, arraysize - 2 * sizeof(double), cudaMemcpyDeviceToHost);

  cudaFree(d_is_in);
  cudaFree(d_is_out);
  cudaFree(d_vs_in);
  cudaFree(d_vs_out);
  cudaFree(d_simulation); // TODO - Really probably shouldn't reallocate and free all this in every step

  return true; // TODO - get validation for each concentration
}
#endif