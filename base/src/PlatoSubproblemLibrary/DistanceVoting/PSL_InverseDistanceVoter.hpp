#pragma once

/* Chooses nearest class by maximizer of an inverse distance from center metric.
 */

#include "PSL_DistanceVoter.hpp"
#include <vector>

namespace PlatoSubproblemLibrary
{
class AbstractAuthority;

class InverseDistanceVoter : public DistanceVoter
{
public:
    InverseDistanceVoter(AbstractAuthority* authority);
    virtual ~InverseDistanceVoter();

    using DistanceVoter::choose_output;
    virtual int choose_output(const int& output_class_size,
                              const std::vector<double>& distances_per_instance,
                              const std::vector<int>& classes_per_instance);

protected:
    double m_tolerance;

};

}
