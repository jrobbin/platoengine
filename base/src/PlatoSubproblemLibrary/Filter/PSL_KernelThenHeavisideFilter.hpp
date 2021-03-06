// PlatoSubproblemLibraryVersion(8): a stand-alone library for the kernel filter for plato.
#pragma once

/* Class: Kernel then heaviside filter for density method topology optimization.
*
* Smooth a field, and then apply a heaviside projection. This accomplishes the length scale
* of the kernel, but retains a somewhat crisp zero-one design.
*/

#include "PSL_Filter.hpp"
#include "PSL_ParameterDataEnums.hpp"

#include <vector>
#include <cstddef>

namespace PlatoSubproblemLibrary
{

namespace AbstractInterface
{
class PointCloud;
class ParallelVector;
class ParallelExchanger;
}
class ParameterData;
class AbstractAuthority;
class KernelFilter;

class KernelThenHeavisideFilter : public Filter
{
public:
    KernelThenHeavisideFilter(AbstractAuthority* authority,
                              ParameterData* data,
                              AbstractInterface::PointCloud* points,
                              AbstractInterface::ParallelExchanger* exchanger);
    virtual ~KernelThenHeavisideFilter();

    void set_authority(AbstractAuthority* authority);
    void set_input_data(ParameterData* data);
    void set_points(AbstractInterface::PointCloud* points);
    void set_parallel_exchanger(AbstractInterface::ParallelExchanger* exchanger);
    void announce_radius();

    // Filter operations
    virtual void build();
    virtual void apply(AbstractInterface::ParallelVector* field);
    virtual void apply(AbstractInterface::ParallelVector* base_field, AbstractInterface::ParallelVector* gradient);
    virtual void advance_continuation();

private:

    bool m_built;
    bool m_announce_radius;
    AbstractAuthority* m_authority;
    ParameterData* m_input_data;
    AbstractInterface::PointCloud* m_original_points;
    AbstractInterface::ParallelExchanger* m_parallel_exchanger;
    KernelFilter* m_kernel;
    double m_current_heaviside_parameter;
    double m_heaviside_parameter_continuation_scale;
    double m_max_heaviside_parameter;

    void check_input_data();
};

}
