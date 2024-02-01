#pragma once

#include <vector>

/** @brief Class which contains information about the state of a ClusterDynamics simulation.
*/
struct ClusterDynamicsState
{
    /** @brief False if this is in an invalid state. True otherwise.
     * 
     * The state is considered invalid if any of the interstitial or
     * vacancy concentrations or the dislocation density are nan, 
     * infinity, or negative.
    */
    bool valid = true;

    /** Marks how much time in seconds has been simulated to create this state.
    */
    double time = 0.0;

    /** @brief The current concentration of each size of interstitial cluster
     * in units of [TODO].
     * 
     * The concentration of clusters of size `n` will be found at interstitials[n].
     * The 0th element is always 0. The size of this data is determined by the
     * `concentration_boundary` that the ClusterDynamics simulation was constructed
     * with.
    */
    std::vector<double> interstitials;

    /** @brief The current concentration of each size of vacancy cluster
     *  in units of [TODO].
     * 
     * The concentration of clusters of size `n` will be found at interstitials[n].
     * The 0th element is always 0. The size of this data is determined by the
     * `concentration_boundary` that the ClusterDynamics simulation was constructed
     * with.
    */
    std::vector<double> vacancies;

    /** @brief The current density of the dislocation network in units of [TODO].
    */
    double dislocation_density = 0.0;
};
