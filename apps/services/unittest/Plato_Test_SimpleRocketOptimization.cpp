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

/*
 * Plato_Test_SimpleRocketOptimization.cpp
 *
 *  Created on: Aug 29, 2018
 */

#include "gtest/gtest.h"

#include "Plato_SimpleRocket.hpp"
#include "Plato_CircularCylinder.hpp"
#include "Plato_GradFreeSimpleRocketObjective.hpp"

#include "Plato_DataFactory.hpp"
#include "Plato_CriterionList.hpp"
#include "Plato_StandardVector.hpp"
#include "Plato_StandardMultiVector.hpp"
#include "Plato_SimpleRocketObjective.hpp"
#include "Plato_TrustRegionAlgorithmDataMng.hpp"
#include "Plato_KelleySachsBoundConstrained.hpp"
#include "Plato_ReducedSpaceTrustRegionStageMng.hpp"

#include "PSL_Random.hpp"
#include "PSL_AbstractAuthority.hpp"
#include "PSL_IterativeSelection.hpp"


#include <ios>
#include <iostream>
#include <fstream>

namespace PlatoTest
{

/******************************************************************************//**
 * @brief Return gold thrust profile for gradient-based unit test example problem
 * @return vector with gold values
**********************************************************************************/
std::vector<double> getGoldThrust()
{
    std::vector<double> tOutput = {0, 1655522.315222185, 1683521.997071911, 1711924.206708037, 1740733.7145938, 1769955.335738611,
                                   1799593.930002549, 1829654.402402195, 1860141.703417779, 1891060.829301665, 1922416.822388162,
                                   1954214.771404665, 1986459.811784134, 2019157.125978916, 2052311.943775886, 2085929.542612965,
                                   2120015.247896948, 2154574.433322704, 2189612.52119371, 2225134.982743949, 2261147.338461152,
                                   2297655.158411417, 2334664.062565152, 2372179.721124417, 2410207.854851608, 2448754.235399517,
                                   2487824.685642763, 2527425.080010583, 2567561.344821009, 2608239.458616416, 2649465.452500456,
                                   2691245.410476362, 2733585.46978665, 2776491.821254185, 2819970.709624668, 2864028.433910482,
                                   2908671.347735961, 2953905.859684023, 2999738.433644236, 3046175.589162255, 3093223.901790677,
                                   3140890.003441307, 3189180.582738817, 3238102.385375825, 3287662.214469391, 3337866.930918911,
                                   3388723.453765457, 3440238.760552502, 3492419.887688101, 3545273.930808475, 3598808.045143031,
                                   3653029.445880811, 3707945.408538376, 3763563.269329108, 3819890.425533986, 3876934.335873749,
                                   3934702.520882559, 3993202.563283048, 4052442.108362866, 4112428.864352653, 4173170.60280544,
                                   4234675.158977559, 4296950.432210954, 4360004.386316981, 4423845.049961671, 4488480.517052433,
                                   4553918.94712624, 4620168.565739292, 4687237.664858119, 4755134.60325219, 4823867.806887973,
                                   4893445.769324492, 4963877.052110332, 5035170.285182183, 5107334.167264794, 5180377.466272492,
                                   5254309.019712122, 5329137.735087518, 5404872.590305486, 5481522.634083196, 5559096.986357204,
                                   5637604.838693859, 5717055.454701263, 5797458.170442756, 5878822.394851873, 5961157.610148815,
                                   6044473.372258459, 6128779.311229875, 6214085.131657324, 6300400.61310283, 6387735.610520262,
                                   6476100.054680888, 6565503.952600521, 6655957.387968196, 6747470.521576302, 6840053.591752338,
                                   6933716.914792155, 7028470.885394753, 7124325.977098601, 7221292.742719544};

    return (tOutput);
}


TEST(PlatoTest, SimpleRocketObjectiveGradFree)
{
    // allocate problem inputs - use default parameters
    Plato::SimpleRocketInuts<double> tRocketInputs;
    std::shared_ptr<Plato::GeometryModel<double>> tGeomModel =
            std::make_shared<Plato::CircularCylinder<double>>(tRocketInputs.mChamberRadius, tRocketInputs.mChamberLength);

    // domain dimension = 10x10=100
    std::vector<int> tNumEvaluationsPerDim = {100, 100};

    /* {chamber_radius_lb, ref_burn_rate_lb},  {chamber_radius_ub, ref_burn_rate_ub} */
    std::pair<std::vector<double>, std::vector<double>> tBounds =
            std::make_pair<std::vector<double>, std::vector<double>>({0.07, 0.004},{0.08, 0.006});
    Plato::GradFreeSimpleRocketObjective tObjective(tRocketInputs, tGeomModel);
    tObjective.setOptimizationInputs(tNumEvaluationsPerDim, tBounds);

    /* {chamber_radius, ref_burn_rate} */
    std::vector<double> tControls = {0.075, 0.005};
    double tValue = tObjective.evaluate(tControls);

    // test objective function evaluation
    double tGold = 0;
    double tTolerance = 1e-6;
    EXPECT_NEAR(tValue, tGold, tTolerance);

    // test inputs for gradient-free algorithm
    std::vector<double> tLowerBounds;
    std::vector<double> tUpperBounds;
    std::vector<int> tEvaluationsPerDim;
    tObjective.get_domain(tLowerBounds, tUpperBounds, tEvaluationsPerDim);

    for(size_t tIndex = 0; tIndex < tNumEvaluationsPerDim.size(); tIndex++)
    {
        ASSERT_DOUBLE_EQ(tLowerBounds[tIndex], tBounds.first[tIndex]);
        ASSERT_DOUBLE_EQ(tUpperBounds[tIndex], tBounds.second[tIndex]);
        EXPECT_EQ(tEvaluationsPerDim[tIndex], tNumEvaluationsPerDim[tIndex]);
    }
}

TEST(PlatoTest, SimpleRocketObjective)
{
    // allocate problem inputs - use default parameters
    Plato::SimpleRocketInuts<double> tRocketInputs;
    std::shared_ptr<Plato::GeometryModel<double>> tGeomModel =
            std::make_shared<Plato::CircularCylinder<double>>(tRocketInputs.mChamberRadius, tRocketInputs.mChamberLength);

    // set normalization constants
    Plato::SimpleRocketObjective<double> tObjective(tRocketInputs, tGeomModel);
    std::vector<double> tUpperBounds = {0.09, 0.007}; /* {chamber_radius_ub, ref_burn_rate_ub} */
    tObjective.setNormalizationConstants(tUpperBounds);

    // evaluate criterion
    const size_t tNumVectors = 1;
    const size_t tNumControls = 2;
    Plato::StandardMultiVector<double> tControl(tNumVectors, tNumControls);
    // set control multi-vector
    const size_t tVECTOR_INDEX = 0;
    tControl(tVECTOR_INDEX, 0) = static_cast<double>(0.075) / tUpperBounds[0];
    tControl(tVECTOR_INDEX, 1) = static_cast<double>(0.005) / tUpperBounds[1];
    const double tObjValue = tObjective.value(tControl);

    // test objective function evaluation
    double tGold = 0;
    double tTolerance = 1e-6;
    EXPECT_NEAR(tObjValue, tGold, tTolerance);

    // evaluate gradient
    Plato::StandardMultiVector<double> tGradient(tNumVectors, tNumControls);
    tControl(tVECTOR_INDEX, 0) = static_cast<double>(0.085) / tUpperBounds[0];
    tControl(tVECTOR_INDEX, 1) = static_cast<double>(0.004) / tUpperBounds[1];
    tObjective.setPerturbationParameter(1e-4);
    tObjective.gradient(tControl, tGradient);
    // test gradient evaluation
    EXPECT_NEAR(tGradient(tVECTOR_INDEX,0), -0.0048183366343682423, tTolerance);
    EXPECT_NEAR(tGradient(tVECTOR_INDEX,1), -0.0057808715527571993, tTolerance);

    // evaluate hessian
    double tValue = 1;
    Plato::StandardMultiVector<double> tHessian(tNumVectors, tNumControls);
    Plato::StandardMultiVector<double> tVector(tNumVectors, tNumControls, tValue);
    tObjective.hessian(tControl, tVector, tHessian);
    // test hessian evaluation
    EXPECT_NEAR(tHessian(tVECTOR_INDEX,0), 1., tTolerance);
    EXPECT_NEAR(tHessian(tVECTOR_INDEX,1), 1., tTolerance);
}

TEST(PlatoTest, CircularCylinder)
{
    // allocate problem inputs - use default parameters
    const double tRadius = 1;
    const double tLength = 2;
    Plato::CircularCylinder<double> tCylinder(tRadius, tLength);

    // test area calculation
    double tTolerance = 1e-6;
    double tArea = tCylinder.area();
    EXPECT_NEAR(tArea, 12.566370614359172, tTolerance);

    // test gradient
    const size_t tNumControls = 2;
    std::vector<double> tGrad(tNumControls);
    tCylinder.gradient(tGrad);
    EXPECT_NEAR(tGrad[0], 12.56637061435917, tTolerance);EXPECT_NEAR(tGrad[1], 6.283185307179586, tTolerance);

    // test update initial configuration
    std::map<std::string, double> tParam;
    tParam.insert(std::pair<std::string, double>("Radius", 2));
    tParam.insert(std::pair<std::string, double>("Configuration", Plato::Configuration::INITIAL));
    tCylinder.update(tParam);
    tArea = tCylinder.area();
    EXPECT_NEAR(tArea, 25.132741228718345, tTolerance);

    // test update dynamics configuration
    tParam.insert(std::pair<std::string, double>("BurnRate", 10));
    tParam.insert(std::pair<std::string, double>("DeltaTime", 0.1));
    tParam.find("Configuration")->second = Plato::Configuration::DYNAMIC;
    tCylinder.update(tParam);
    tArea = tCylinder.area();
    EXPECT_NEAR(tArea, 37.699111843077520, tTolerance);
}

TEST(PlatoTest, GradFreeSimpleRocketOptimization)
{
    PlatoSubproblemLibrary::AbstractAuthority tAuthority;

    // define objective
    Plato::SimpleRocketInuts<double> tRocketInputs;
    std::shared_ptr<Plato::GeometryModel<double>> tGeomModel =
    std::make_shared<Plato::CircularCylinder<double>>(tRocketInputs.mChamberRadius, tRocketInputs.mChamberLength);
    Plato::GradFreeSimpleRocketObjective tObjective(tRocketInputs, tGeomModel);;

    // set inputs for optimization problem
    std::vector<int> tNumEvaluationsPerDim = {100, 100}; // domain dimension = 100x100=10000
    /* {chamber_radius_lb, ref_burn_rate_lb},  {chamber_radius_ub, ref_burn_rate_ub} */
    std::pair<std::vector<double>, std::vector<double>> aBounds =
    std::make_pair<std::vector<double>, std::vector<double>>( {0.07, 0.004}, {0.08, 0.006});
    tObjective.setOptimizationInputs(tNumEvaluationsPerDim, aBounds);

    // define searcher
    PlatoSubproblemLibrary::IterativeSelection tSearcher(&tAuthority);
    tSearcher.set_objective(&tObjective);

    // find a minimum
    std::vector<double> tBestParameters;
    PlatoSubproblemLibrary::set_rand_seed();
    const double tBestObjectiveValue = tSearcher.find_min(tBestParameters);
    std::cout << "BestObjectiveValue = " << tBestObjectiveValue << "\n";
    std::cout << "NumFunctionEvaluations = " << tSearcher.get_number_of_evaluations() << "\n";
    std::cout << "Best1: " << tBestParameters[0] << ", Best2: " << tBestParameters[1] << std::endl;

    // equal by determinism of objective
    EXPECT_FLOAT_EQ(tBestObjectiveValue, tObjective.evaluate(tBestParameters));
}

TEST(PlatoTest, GradBasedSimpleRocketOptimization)
{
    // ********* ALLOCATE DATA FACTORY *********
    std::shared_ptr<Plato::DataFactory<double>> tDataFactory = std::make_shared<Plato::DataFactory<double>>();
    const size_t tNumControls = 2;
    tDataFactory->allocateControl(tNumControls);

    // ********* ALLOCATE TRUST REGION ALGORITHM DATA MANAGER *********
    std::shared_ptr<Plato::TrustRegionAlgorithmDataMng<double>> tDataMng =
            std::make_shared<Plato::TrustRegionAlgorithmDataMng<double>>(tDataFactory);

    // ********* SET CONTROL PARAMETERS *********
    const size_t tVECTOR_INDEX = 0;
    std::vector<double> tNormalizationConstants(tNumControls);
    tNormalizationConstants[0] = 0.08; tNormalizationConstants[1] = 0.006;

    Plato::StandardVector<double> tLowerBounds(tNumControls);
    tLowerBounds[0] = 0.06 / tNormalizationConstants[0]; tLowerBounds[1] = 0.003 / tNormalizationConstants[1];
    tDataMng->setControlLowerBounds(tVECTOR_INDEX, tLowerBounds);

    Plato::StandardVector<double> tUpperBounds(tNumControls);
    tUpperBounds[0] = 1.0; tUpperBounds[1] = 1.0;
    tDataMng->setControlUpperBounds(tVECTOR_INDEX, tUpperBounds);

    Plato::StandardVector<double> tInitialGuess(tNumControls);
    tInitialGuess[0] = 0.074 / tNormalizationConstants[0]; tInitialGuess[1] = 0.0055 / tNormalizationConstants[1];
    tDataMng->setInitialGuess(tVECTOR_INDEX, tInitialGuess);

    // ********* ALLOCATE OBJECTIVE *********
    Plato::SimpleRocketInuts<double> tRocketInputs;
    std::shared_ptr<Plato::GeometryModel<double>> tGeomModel =
            std::make_shared<Plato::CircularCylinder<double>>(tRocketInputs.mChamberRadius, tRocketInputs.mChamberLength);
    std::shared_ptr<Plato::SimpleRocketObjective<double>> tMyObjective =
            std::make_shared<Plato::SimpleRocketObjective<double>>(tRocketInputs, tGeomModel);

    // ********* SET NORMALIZATION PARAMETERS *********
    tMyObjective->setNormalizationConstants(tNormalizationConstants);

    // ********* SET OBJECTIVE FUNCTION LIST *********
    std::shared_ptr<Plato::CriterionList<double>> tObjectiveList = std::make_shared<Plato::CriterionList<double>>();
    tObjectiveList->add(tMyObjective);

    // ********* NEWTON ALGORITHM'S REDUCED SPACE STAGE MANAGER *********
    std::shared_ptr<Plato::ReducedSpaceTrustRegionStageMng<double>> tStageMng =
            std::make_shared<Plato::ReducedSpaceTrustRegionStageMng<double>>(tDataFactory, tObjectiveList);

    // ********* ALLOCATE KELLEY-SACHS ALGORITHM *********
    Plato::KelleySachsBoundConstrained<double> tAlgorithm(tDataFactory, tDataMng, tStageMng);
    tAlgorithm.setMaxNumIterations(500);
    tAlgorithm.setMaxTrustRegionRadius(1e1);
    tAlgorithm.setStationarityTolerance(1e-12);
    tAlgorithm.setStagnationTolerance(1e-12);
    tAlgorithm.setGradientTolerance(1e-12);
    tAlgorithm.setActualReductionTolerance(1e-12);
    tAlgorithm.solve();

    // ********* EQUAL BY DETERMINISM OF OBJECTIVE *********
    EXPECT_FLOAT_EQ(tDataMng->getCurrentObjectiveFunctionValue(), tMyObjective->value(tDataMng->getCurrentControl()));

    // ********* TEST SOLUTION *********
    const double tTolerance = 1e-4;
    const double tBest1 = tDataMng->getCurrentControl(tVECTOR_INDEX)[0] * tNormalizationConstants[0];
    const double tBest2 = tDataMng->getCurrentControl(tVECTOR_INDEX)[1] * tNormalizationConstants[1];
    EXPECT_NEAR(tBest1, 0.0749672, tTolerance);
    EXPECT_NEAR(tBest2, 0.00500131, tTolerance);

    // ********* OUTPUT TO CONSOLE *********
    std::cout << "NumIterationsDone = " << tAlgorithm.getNumIterationsDone() << std::endl;
    std::cout << "NumFunctionEvaluations = " << tMyObjective->getNumFunctionEvaluations() << std::endl;
    std::cout << "BestObjectiveValue = " << tDataMng->getCurrentObjectiveFunctionValue() << std::endl;
    std::cout << "Best1: " << tBest1 << ", Best2: " << tBest2 << std::endl;
    std::ostringstream tMessage;
    Plato::getStopCriterion(tAlgorithm.getStoppingCriterion(), tMessage);
    std::cout << "StoppingCriterion = " << tMessage.str().c_str() << std::endl;

    std::vector<double> tGoldThrustProfile = getGoldThrust();
    std::vector<double> tThrustProfile = tMyObjective->getThrustProfile();
    for(size_t tIndex = 0; tIndex < tThrustProfile.size(); tIndex++)
    {
        EXPECT_NEAR(tThrustProfile[tIndex], tGoldThrustProfile[tIndex], tTolerance);
    }
}

} // namespace PlatoTest
