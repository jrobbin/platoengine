/*
//@HEADER
// *************************************************************************
//   Plato Engine v.1.0: Copyright 2018, National Technology & Engineering
//                    Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Sandia Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact the Plato team (plato3D-help@sandia.gov)
//
// *************************************************************************
//@HEADER
*/

#include "PSL_UnitTestingHelper.hpp"

#include "PSL_InvertibleUnitRangeFactory.hpp"
#include "PSL_ParameterDataEnums.hpp"
#include "PSL_ParameterData.hpp"
#include "PSL_FreeHelpers.hpp"
#include "PSL_InvertibleUnitRange.hpp"
#include "PSL_Interface_BasicGlobalUtilities.hpp"
#include "PSL_Random.hpp"

#include <vector>

namespace PlatoSubproblemLibrary
{
namespace TestingInvertibleUnitRange
{

void test_invertibleUnitRangeFunctionProperties(InvertibleUnitRange* function)
{
    // pose problem
    const double lower_bound = uniform_rand_double(-10., -5.);
    const double upper_bound = uniform_rand_double(6., 9.);
    const int num_points = 50;

    // sample points
    std::vector<double> points(num_points);
    uniform_rand_double(lower_bound, upper_bound, points);
    std::sort(points.begin(), points.end());

    // change scales num_repeat times
    const int num_repeat = 3;
    for(int repeat = 0; repeat < num_repeat; repeat++)
    {
        const double negative_scale = uniform_rand_double(0.5, 2.5);
        const double positive_scale = uniform_rand_double(0.2, 1.5);
        function->set_scale(negative_scale, positive_scale);

        // for each point
        for(int p = 0; p < num_points; p++)
        {
            const double y = function->evaluate(points[p]);

            // expect monotonic
            if(0 < p)
            {
                const double y_previous = function->evaluate(points[p - 1]);
                EXPECT_GT(y, y_previous);
            }

            // expect invertible
            const double computed_x = function->invert(y);
            EXPECT_FLOAT_EQ(points[p], computed_x);
        }
    }
}

bool test_invertibleUnitRangeFunctionProperties_logistic()
{
    example::Interface_BasicGlobalUtilities utilities;
    const invertible_unit_range_t::invertible_unit_range_t type =
            invertible_unit_range_t::invertible_unit_range_t::logistic_function;
    InvertibleUnitRange* function = build_invertible_unit_range(&utilities, type, 1., 1.);
    test_invertibleUnitRangeFunctionProperties(function);
    safe_free(function);
    return true;
}
PSL_TEST(InvertibleUnitRange, properties_logistic)
{
    set_rand_seed();
    stocastic_test(test_invertibleUnitRangeFunctionProperties_logistic, 5, 5, .99);
}
bool test_invertibleUnitRangeFunctionProperties_algebraic()
{
    example::Interface_BasicGlobalUtilities utilities;
    const invertible_unit_range_t::invertible_unit_range_t type =
            invertible_unit_range_t::invertible_unit_range_t::algebraic_function;
    InvertibleUnitRange* function = build_invertible_unit_range(&utilities, type, 1., 1.);
    test_invertibleUnitRangeFunctionProperties(function);
    safe_free(function);
    return true;
}
PSL_TEST(InvertibleUnitRange, properties_algebraic)
{
    set_rand_seed();
    stocastic_test(test_invertibleUnitRangeFunctionProperties_algebraic, 5, 5, .99);
}

PSL_TEST(InvertibleUnitRange, buildFromFactory)
{
    set_rand_seed();
    // allocate
    example::Interface_BasicGlobalUtilities utilities;
    const double val1 = .25; // arbitrary
    const double val2 = 4.2; // arbitrary

    // list types
    std::vector<invertible_unit_range_t::invertible_unit_range_t> types = {
            invertible_unit_range_t::invertible_unit_range_t::logistic_function,
            invertible_unit_range_t::invertible_unit_range_t::algebraic_function};

    // for each type to try
    InvertibleUnitRange* iur = NULL;
    const int num_types = types.size();
    for(int t = 0; t < num_types; t++)
    {
        iur = build_invertible_unit_range(&utilities, types[t], val1, val2);
        EXPECT_EQ(iur->get_type(), types[t]);
        safe_free(iur);
    }
}

}
}
