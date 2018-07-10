#pragma once

#include "PSL_DistanceVoter.hpp"
#include <vector>

namespace PlatoSubproblemLibrary
{
class AbstractAuthority;

class LinearDistanceVoter : public DistanceVoter
{
public:
    LinearDistanceVoter(AbstractAuthority* authority);
    virtual ~LinearDistanceVoter();

    // implement at least one of these
    virtual int choose_output(const int& output_class_size,
                              const std::vector<double>& distances_per_instance,
                              const std::vector<int>& classes_per_instance);

protected:
    double m_tolerance;

};

}
