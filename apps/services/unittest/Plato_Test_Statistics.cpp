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
 * Plato_Test_Statistics.cpp
 *
 *  Created on: Nov 17, 2017
 */

#include "gtest/gtest.h"

#include <numeric>
#include <cmath>

#include "Plato_DataFactory.hpp"
#include "Plato_Diagnostics.hpp"
#include "Plato_SromObjective.hpp"
#include "Plato_Communication.hpp"
#include "Plato_SromConstraint.hpp"
#include "Plato_SromStatistics.hpp"
#include "Plato_StandardVector.hpp"
#include "Plato_BetaDistribution.hpp"
#include "Plato_NormalDistribution.hpp"
#include "Plato_StandardMultiVector.hpp"
#include "Plato_UniformDistribution.hpp"
#include "Plato_TrustRegionAlgorithmDataMng.hpp"
#include "Plato_AugmentedLagrangianStageMng.hpp"
#include "Plato_KelleySachsAugmentedLagrangian.hpp"
#include "Plato_StandardVectorReductionOperations.hpp"
#include "Plato_StatisticsUtils.hpp"
#include "Plato_SolveUncertaintyProblem.hpp"

#include "Plato_UnitTestUtils.hpp"

namespace PlatoTest
{

TEST(PlatoTest, Uniform)
{
    const double tLowerBound = 2;
    const double tUpperBound = 7;
    Plato::UniformDistribution<double> tDistribution(tLowerBound, tUpperBound);

    // ********** TEST STATISTICS FOR UNIFORM DISTRIBUTION **********
    const double tTolerance = 1e-5;
    EXPECT_NEAR(tLowerBound, tDistribution.lower(), tTolerance);
    EXPECT_NEAR(tUpperBound, tDistribution.upper(), tTolerance);
    double tGoldValue = 4.5;
    EXPECT_NEAR(tGoldValue, tDistribution.mean(), tTolerance);
    tGoldValue = (1. / 12.) * (tUpperBound - tLowerBound) * (tUpperBound - tLowerBound);
    EXPECT_NEAR(tGoldValue, tDistribution.variance(), tTolerance);
    tGoldValue = std::log(tUpperBound - tLowerBound);
    EXPECT_NEAR(tGoldValue, tDistribution.entropy(), tTolerance);
    // ********** TEST PDF **********
    std::vector<double> tSamples;
    tSamples.push_back(1);
    tSamples.push_back(2);
    tSamples.push_back(3);
    tSamples.push_back(4);
    tSamples.push_back(5);
    tSamples.push_back(6);
    tSamples.push_back(7);
    tSamples.push_back(8);
    std::vector<double> tGoldPDF;
    tGoldPDF.push_back(0);
    tGoldPDF.push_back(0.2);
    tGoldPDF.push_back(0.2);
    tGoldPDF.push_back(0.2);
    tGoldPDF.push_back(0.2);
    tGoldPDF.push_back(0.2);
    tGoldPDF.push_back(0.2);
    tGoldPDF.push_back(0);
    for(size_t tIndex = 0; tIndex < tGoldPDF.size(); tIndex++)
    {
        EXPECT_NEAR(tGoldPDF[tIndex], tDistribution.pdf(tSamples[tIndex]), tTolerance);
    }
    // ********** TEST CDF **********
    std::vector<double> tGoldCDF;
    tGoldCDF.push_back(0);
    tGoldCDF.push_back(0);
    tGoldCDF.push_back(.2);
    tGoldCDF.push_back(.4);
    tGoldCDF.push_back(.6);
    tGoldCDF.push_back(.8);
    tGoldCDF.push_back(1);
    tGoldCDF.push_back(1);
    for(size_t tIndex = 0; tIndex < tGoldCDF.size(); tIndex++)
    {
        EXPECT_NEAR(tGoldCDF[tIndex], tDistribution.cdf(tSamples[tIndex]), tTolerance);
    }
    // ********** TEST MOMENTS **********
    size_t tNumMoments = 8;
    for(size_t tOrder = 0; tOrder <= tNumMoments; tOrder++)
    {
        double tExponent = tOrder + 1u;
        double tGold = (std::pow(tUpperBound, tExponent) - std::pow(tLowerBound, tExponent))
                / (tExponent * (tUpperBound - tLowerBound));
        EXPECT_NEAR(tGold, tDistribution.moment(tOrder), tTolerance);
    }
}

TEST(PlatoTest, Normal)
{
    const double tMean = 0;
    const double tSigma = 1;

    double tEndRange = 1e3;
    double tBeginRange = -1e3;
    double tIncrement = 0.1;
    double tRange = ((tEndRange - tBeginRange) / tIncrement) + 1;
    std::vector<double> tNumbers(tRange);

    std::vector<double> tPDF(tRange);
    std::vector<double> tCDF(tRange);
    for(size_t tIndex = 0; tIndex < tRange; tIndex++)
    {
        tNumbers[tIndex] = tBeginRange + (tIncrement * tIndex);
        tPDF[tIndex] = Plato::normal_pdf<double>(tNumbers[tIndex], tMean, tSigma);
        tCDF[tIndex] = Plato::normal_cdf<double>(tNumbers[tIndex], tMean, tSigma);
    }

    const double tBaseValue = 0;
    const double tTolerance = 1e-3;
    double tValue = std::accumulate(tPDF.begin(), tPDF.end(), tBaseValue) / tRange;
    EXPECT_NEAR(tValue, tMean, tTolerance);
}

TEST(PlatoTest, SromCDF)
{
    double tSample = 0.276806509167094;
    Plato::StandardVector<double> tSamples(4, 0.);
    tSamples[0] = 0.276806509167094;
    tSamples[1] = 0.431107226622461;
    tSamples[2] = 0.004622102620248;
    tSamples[3] = 0.224162021074166;
    Plato::StandardVector<double> tSampleProbabilities(4, 0.);
    tSampleProbabilities[0] = 0.25;
    tSampleProbabilities[1] = 0.25;
    tSampleProbabilities[2] = 0.25;
    tSampleProbabilities[3] = 0.25;
    double tSigma = 1e-3;

    double tOutput = Plato::compute_srom_cdf<double>(tSample, tSigma, tSamples, tSampleProbabilities);

    double tGold = 0.625;
    double tTolerance = 1e-4;
    EXPECT_NEAR(tGold, tOutput, tTolerance);
}

TEST(PlatoTest, SromMoments)
{
    Plato::StandardVector<double> tSamples(4, 0.);
    tSamples[0] = 0.276806509167094;
    tSamples[1] = 0.431107226622461;
    tSamples[2] = 0.004622102620248;
    tSamples[3] = 0.224162021074166;
    Plato::StandardVector<double> tSampleProbabilities(4, 0.);
    tSampleProbabilities[0] = 0.25;
    tSampleProbabilities[1] = 0.25;
    tSampleProbabilities[2] = 0.25;
    tSampleProbabilities[3] = 0.25;

    Plato::StandardVector<double> tMoments(4, 0.);
    for(size_t tIndex = 0; tIndex < tMoments.size(); tIndex++)
    {
        double tOrder = tIndex + static_cast<size_t>(1);
        tMoments[tIndex] = Plato::compute_srom_moment<double>(tOrder, tSamples, tSampleProbabilities);
    }

    Plato::StandardVector<double> tGold(4, 0.);
    tGold[0] = 0.234174464870992;
    tGold[1] = 0.078186314972017;
    tGold[2] = 0.028149028892565;
    tGold[3] = 0.010734332952929;
    PlatoTest::checkVectorData(tMoments, tGold);
}

TEST(PlatoTest, factorial)
{
    size_t tGold = 1;
    size_t tValue = Plato::factorial<size_t>(0);
    EXPECT_EQ(tGold, tValue);

    tGold = 1;
    tValue = Plato::factorial<size_t>(1);
    EXPECT_EQ(tGold, tValue);

    tGold = 362880;
    tValue = Plato::factorial<size_t>(9);
    EXPECT_EQ(tGold, tValue);
}

TEST(PlatoTest, Beta)
{
    double tAlpha = 1;
    double tBeta = 3;
    double tValue = Plato::beta<double>(tAlpha, tBeta);

    double tTolerance = 1e-6;
    double tGold = 1. / 3.;
    EXPECT_NEAR(tGold, tValue, tTolerance);
}

TEST(PlatoTest, PochhammerSymbol)
{
    // TEST ONE: NON-FINITE NUMBER CASE
    double tOutput = Plato::pochhammer_symbol<double>(-2, 0);
    const double tTolerance = 1e-5;
    EXPECT_NEAR(0, tOutput, tTolerance);

    // TEST TWO: FINITE NUMBER CASE
    tOutput = Plato::pochhammer_symbol<double>(2.166666666666666, 4.333333333333333);
    double tGold = 265.98433449717857;
    EXPECT_NEAR(tGold, tOutput, tTolerance);
}

TEST(PlatoTest, BetaMoment)
{
    size_t tOrder = 3;
    double tAlpha = 2.166666666666666;
    double tBeta = 4.333333333333333;
    double tValue = Plato::beta_moment<double>(tOrder, tAlpha, tBeta);

    double tTolerance = 1e-6;
    double tGold = 0.068990559186638;
    EXPECT_NEAR(tGold, tValue, tTolerance);
}

TEST(PlatoTest, ComputeShapeParameters)
{
    const double tMean = 90;
    const double tMaxValue = 135;
    const double tMinValue = 67.5;
    const double tVariance = 135;
    double tAlphaShapeParameter = 0;
    double tBetaShapeParameter = 0;
    Plato::shape_parameters<double>(tMinValue, tMaxValue, tMean, tVariance, tAlphaShapeParameter, tBetaShapeParameter);

    const double tTolerance = 1e-6;
    const double tGoldAlpha = 2.166666666666666;
    const double tGoldBeta = 4.333333333333333;
    EXPECT_NEAR(tGoldBeta, tBetaShapeParameter, tTolerance);
    EXPECT_NEAR(tGoldAlpha, tAlphaShapeParameter, tTolerance);
}

TEST(PlatoTest, IncompleteBeta)
{
    double tSample = 1.;
    const double tAlpha = 2.166666666666666;
    const double tBeta = 4.333333333333333;

    double tOutput = Plato::incomplete_beta<double>(tSample, tAlpha, tBeta);

    const double tTolerance = 1e-5;
    const double tGold = Plato::beta<double>(tAlpha, tBeta);
    EXPECT_NEAR(tOutput, tGold, tTolerance);
}

TEST(PlatoTest, BetaPDF)
{
    const size_t tRange = 4;
    std::vector<double> tPDF(tRange);
    std::vector<double> tShapeParam(tRange);
    tShapeParam[0] = 0.5;
    tShapeParam[1] = 1;
    tShapeParam[2] = 2;
    tShapeParam[3] = 4;
    const double tSample = 0.5;
    for(size_t tIndex = 0; tIndex < tRange; tIndex++)
    {
        tPDF[tIndex] = Plato::beta_pdf<double>(tSample, tShapeParam[tIndex], tShapeParam[tIndex]);
    }

    const double tTolerance = 1e-6;
    std::vector<double> tGoldPDF(tRange, 0.);
    tGoldPDF[0] = 0.636619772367582;
    tGoldPDF[1] = 1.0;
    tGoldPDF[2] = 1.5;
    tGoldPDF[3] = 2.1875;
    for(size_t tIndex = 0; tIndex < tRange; tIndex++)
    {
        EXPECT_NEAR(tPDF[tIndex], tGoldPDF[tIndex], tTolerance);
    }
}

TEST(PlatoTest, BetaCDF)
{
    const double tBeta = 3;
    const double tSample = 0.5;
    const size_t tRange = 11;
    std::vector<double> tCDF(tRange, 0.);
    for(size_t tIndex = 0; tIndex < tRange; tIndex++)
    {
        tCDF[tIndex] = Plato::beta_cdf<double>(tSample, tIndex, tBeta);
    }

    const double tTolerance = 1e-6;
    std::vector<double> tGoldCDF(tRange, 0.);
    tGoldCDF[0] = 1;
    tGoldCDF[1] = 0.875;
    tGoldCDF[2] = 0.6875;
    tGoldCDF[3] = 0.5;
    tGoldCDF[4] = 0.34375;
    tGoldCDF[5] = 0.2265625;
    tGoldCDF[6] = 0.14453125;
    tGoldCDF[7] = 0.08984375;
    tGoldCDF[8] = 0.0546875;
    tGoldCDF[9] = 0.03271484375;
    tGoldCDF[10] = 0.019287109375;
    for(size_t tIndex = 0; tIndex < tRange; tIndex++)
    {
        EXPECT_NEAR(tCDF[tIndex], tGoldCDF[tIndex], tTolerance);
    }
}

TEST(PlatoTest, BetaDistribution)
{
    const double tMean = 90;
    const double tMax = 135;
    const double tMin = 67.5;
    const double tVariance = 135;
    Plato::BetaDistribution<double> tDistribution(tMin, tMax, tMean, tVariance);

    // TEST INPUTS
    const double tTolerance = 1e-5;
    EXPECT_NEAR(tMin, tDistribution.min(), tTolerance);
    EXPECT_NEAR(tMax, tDistribution.max(), tTolerance);
    EXPECT_NEAR(tMean, tDistribution.mean(), tTolerance);
    EXPECT_NEAR(tVariance, tDistribution.variance(), tTolerance);

    // TEST BETA PDF & CDF
    double tSample = 0.276806509167094;
    double tGoldPDF = 2.179085850493935;
    EXPECT_NEAR(tGoldPDF, tDistribution.pdf(tSample), tTolerance);
    double tGoldCDF = 0.417022004702574;
    EXPECT_NEAR(tGoldCDF, tDistribution.cdf(tSample), tTolerance);

    // TEST BETA MOMENTS
    const size_t tNumMoments = 4;
    Plato::StandardVector<double> tGoldMoments(tNumMoments);
    tGoldMoments[0] = 0.333333333333333;
    tGoldMoments[1] = 0.140740740740740;
    tGoldMoments[2] = 0.068990559186638;
    tGoldMoments[3] = 0.037521181312031;
    for(size_t tIndex = 0; tIndex < tNumMoments; tIndex++)
    {
        size_t tOrder = tIndex + 1;
        EXPECT_NEAR(tGoldMoments[tIndex], tDistribution.moment(tOrder), tTolerance);
    }
}

TEST(PlatoTest, SromObjectiveTestOne)
{
    // ********* ALLOCATE BETA DISTRIBUTION *********
    const double tMean = 90;
    const double tMax = 135;
    const double tMin = 67.5;
    const double tVariance = 135;
    std::shared_ptr<Plato::BetaDistribution<double>> tDistribution =
            std::make_shared<Plato::BetaDistribution<double>>(tMin, tMax, tMean, tVariance);

    // ********* SET TEST DATA: SAMPLES AND PROBABILITIES *********
    const size_t tNumVectors = 2;
    const size_t tNumSamples = 4;
    Plato::StandardMultiVector<double> tControl(tNumVectors, tNumSamples);
    size_t tVectorIndex = 0;
    tControl(tVectorIndex, 0) = 0.276806509167094;
    tControl(tVectorIndex, 1) = 0.431107226622461;
    tControl(tVectorIndex, 2) = 0.004622102620248;
    tControl(tVectorIndex, 3) = 0.224162021074166;
    tVectorIndex = 1;
    tControl[tVectorIndex].fill(0.25);

    // ********* TEST OBJECTIVE FUNCTION *********
    const size_t tMaxNumMoments = 4;
    Plato::SromObjective<double> tObjective(tDistribution, tMaxNumMoments, tNumSamples);
    tObjective.setCdfMisfitTermWeight(1);
    tObjective.setMomentMisfitTermWeight(1);
    double tValue = tObjective.value(tControl);
    double tTolerance = 1e-5;
    double tGold = 0.617109315688096;
    EXPECT_NEAR(tGold, tValue, tTolerance);

    // ********* TEST OBJECTIVE GRADIENT *********
    Plato::StandardMultiVector<double> tGradient(tNumVectors, tNumSamples);
    tObjective.gradient(tControl, tGradient);
    Plato::StandardMultiVector<double> tGradientGold(tNumVectors, tNumSamples);
    tVectorIndex = 0;
    tGradientGold(tVectorIndex, 0) = -2.010045017107233;
    tGradientGold(tVectorIndex, 1) = -3.878346258927178;
    tGradientGold(tVectorIndex, 2) = -0.237208262654126;
    tGradientGold(tVectorIndex, 3) = -1.271234346951175;
    tVectorIndex = 1;
    tGradientGold(tVectorIndex, 0) = -0.524038145360132;
    tGradientGold(tVectorIndex, 1) = -2.239056684273221;
    tGradientGold(tVectorIndex, 2) = 0.493570515676146;
    tGradientGold(tVectorIndex, 3) = -0.104442116117926;
    PlatoTest::checkMultiVectorData(tGradient, tGradientGold);
}

TEST(PlatoTest, SromObjectiveTestTwo)
{
    // ********* ALLOCATE BETA DISTRIBUTION *********
    const double tMean = 90;
    const double tMax = 135;
    const double tMin = 67.5;
    const double tVariance = 135;
    std::shared_ptr<Plato::BetaDistribution<double>> tDistribution =
            std::make_shared<Plato::BetaDistribution<double>>(tMin, tMax, tMean, tVariance);

    // ********* SET TEST DATA: SAMPLES AND PROBABILITIES *********
    const size_t tNumVectors = 3;
    const size_t tNumSamples = 4;
    Plato::StandardMultiVector<double> tControl(tNumVectors, tNumSamples);
    size_t tVectorIndex = 0;
    tControl(tVectorIndex, 0) = 0.276806509167094;
    tControl(tVectorIndex, 1) = 0.004622102620248;
    tControl(tVectorIndex, 2) = 0.376806509167094;
    tControl(tVectorIndex, 3) = 0.104622102620248;
    tVectorIndex = 1;
    tControl(tVectorIndex, 0) = 0.431107226622461;
    tControl(tVectorIndex, 1) = 0.224162021074166;
    tControl(tVectorIndex, 2) = 0.531107226622461;
    tControl(tVectorIndex, 3) = 0.324162021074166;
    tVectorIndex = 2;
    tControl[tVectorIndex].fill(0.25);

    // ********* TEST OBJECTIVE FUNCTION *********
    const size_t tRandomVecDim = 2;
    const size_t tMaxNumMoments = 4;
    Plato::SromObjective<double> tObjective(tDistribution, tMaxNumMoments, tNumSamples, tRandomVecDim);
    tObjective.setCdfMisfitTermWeight(1);
    tObjective.setMomentMisfitTermWeight(1);
    double tValue = tObjective.value(tControl);
    double tTolerance = 1e-5;
    double tGold = 1.032230626961365;
    EXPECT_NEAR(tGold, tValue, tTolerance);

    // ********* TEST OBJECTIVE GRADIENT *********
    Plato::StandardMultiVector<double> tGradient(tNumVectors, tNumSamples);
    tObjective.gradient(tControl, tGradient);
    Plato::StandardMultiVector<double> tGradientGold(tNumVectors, tNumSamples);
    tVectorIndex = 0;
    tGradientGold(tVectorIndex, 0) = -2.42724408126656;
    tGradientGold(tVectorIndex, 1) = -0.337450847795798;
    tGradientGold(tVectorIndex, 2) = -3.887791716578634;
    tGradientGold(tVectorIndex, 3) = -1.076413326527892;
    tVectorIndex = 1;
    tGradientGold(tVectorIndex, 0) = 0.096246202011561;
    tGradientGold(tVectorIndex, 1) = 0.520617569090164;
    tGradientGold(tVectorIndex, 2) = -0.321363712239195;
    tGradientGold(tVectorIndex, 3) = 0.384504837554259;
    tVectorIndex = 2;
    tGradientGold(tVectorIndex, 0) = -0.53206506489113;
    tGradientGold(tVectorIndex, 1) = 0.619653114279367;
    tGradientGold(tVectorIndex, 2) = -1.84853491196106;
    tGradientGold(tVectorIndex, 3) = 0.426963908092988;
    PlatoTest::checkMultiVectorData(tGradient, tGradientGold);
}

TEST(PlatoTest, SromConstraint)
{
    // ********* SET TEST DATA: SAMPLES AND PROBABILITIES *********
    const size_t tNumVectors = 2;
    const size_t tNumSamples = 4;
    Plato::StandardMultiVector<double> tControl(tNumVectors, tNumSamples);
    size_t tVectorIndex = 0;
    tControl(tVectorIndex, 0) = 0.183183326166505;
    tControl(tVectorIndex, 1) = 0.341948604575779;
    tControl(tVectorIndex, 2) = 0.410656896223290;
    tControl(tVectorIndex, 3) = 0.064209040541960;
    tVectorIndex = 1;
    tControl(tVectorIndex, 0) = 0.434251989288042;
    tControl(tVectorIndex, 1) = 0.351721349341024;
    tControl(tVectorIndex, 2) = 0.001250000000000;
    tControl(tVectorIndex, 3) = 0.212776663693648;

    // ********* TEST CONSTRAINT EVALUATION *********
    std::shared_ptr<Plato::StandardVectorReductionOperations<double>> tReductions =
            std::make_shared<Plato::StandardVectorReductionOperations<double>>();
    Plato::SromConstraint<double> tConstraint(tReductions);
    double tValue = tConstraint.value(tControl);

    double tGoldValue = 0;
    double tTolerance = 1e-6;
    EXPECT_NEAR(tGoldValue, tValue, tTolerance);

    // ********* TEST CONSTRAINT GRADIENT *********
    Plato::StandardMultiVector<double> tGradient(tNumVectors, tNumSamples);
    tConstraint.gradient(tControl, tGradient);
    Plato::StandardMultiVector<double> tGradientGold(tNumVectors, tNumSamples);
    tVectorIndex = 0;
    tGradientGold(tVectorIndex, 0) = 0;
    tGradientGold(tVectorIndex, 1) = 0;
    tGradientGold(tVectorIndex, 2) = 0;
    tGradientGold(tVectorIndex, 3) = 0;
    tVectorIndex = 1;
    tGradientGold(tVectorIndex, 0) = 1;
    tGradientGold(tVectorIndex, 1) = 1;
    tGradientGold(tVectorIndex, 2) = 1;
    tGradientGold(tVectorIndex, 3) = 1;
    PlatoTest::checkMultiVectorData(tGradient, tGradientGold);
}

TEST(PlatoTest, NormalDistribution)
{
    double tMean = 1;
    double tStandardDeviation = 0.5;
    Plato::NormalDistribution<double> tDistribution(tMean, tStandardDeviation);

    double tGoldValue = 1;
    double tTolerance = 1e-6;
    EXPECT_NEAR(tGoldValue, tDistribution.mean(), tTolerance);
    tGoldValue = 0.5;
    EXPECT_NEAR(tGoldValue, tDistribution.sigma(), tTolerance);

    // ********* TEST GAUSSIAN PDF AND CDF *********
    double tSample = 0.2;
    tGoldValue = 0.221841669358911;
    EXPECT_NEAR(tGoldValue, tDistribution.pdf(tSample), tTolerance);
    tGoldValue = 0.054799291699558;
    EXPECT_NEAR(tGoldValue, tDistribution.cdf(tSample), tTolerance);

    // ********* TEST GAUSSIAN MOMENTS *********
    size_t tMaxNumMoments = 8;
    std::vector<double> tGoldMoments(tMaxNumMoments + 1u);
    tGoldMoments[0] = 1;
    tGoldMoments[1] = tMean;
    tGoldMoments[2] = std::pow(tMean, 2.) + std::pow(tStandardDeviation, 2.);
    tGoldMoments[3] = std::pow(tMean, 3.) + 3 * tMean * std::pow(tStandardDeviation, 2.);
    tGoldMoments[4] = std::pow(tMean, 4.) + 6 * std::pow(tMean, 2.) * std::pow(tStandardDeviation, 2.)
                      + 3 * std::pow(tStandardDeviation, 4.);
    tGoldMoments[5] = std::pow(tMean, 5.) + 10 * std::pow(tMean, 3.) * std::pow(tStandardDeviation, 2.)
                      + 15 * tMean * std::pow(tStandardDeviation, 4.);
    tGoldMoments[6] = std::pow(tMean, 6.) + 15 * std::pow(tMean, 4.) * std::pow(tStandardDeviation, 2.)
                      + 45 * std::pow(tMean, 2.) * std::pow(tStandardDeviation, 4.) + 15 * std::pow(tStandardDeviation, 6.);
    tGoldMoments[7] = std::pow(tMean, 7.) + 21 * std::pow(tMean, 5.) * std::pow(tStandardDeviation, 2.)
                      + 105 * std::pow(tMean, 3.) * std::pow(tStandardDeviation, 4.)
                      + 105 * tMean * std::pow(tStandardDeviation, 6.);
    tGoldMoments[8] = std::pow(tMean, 8.) + 28 * std::pow(tMean, 6.) * std::pow(tStandardDeviation, 2.)
                      + 210 * std::pow(tMean, 4.) * std::pow(tStandardDeviation, 4.)
                      + 420 * std::pow(tMean, 2.) * std::pow(tStandardDeviation, 6.)
                      + 105 * tMean * std::pow(tStandardDeviation, 8.);
    for(size_t tOrder = 0; tOrder <= tMaxNumMoments; tOrder++)
    {
        double tValue = tDistribution.moment(tOrder);
        EXPECT_NEAR(tGoldMoments[tOrder], tValue, tTolerance);
    }
}

TEST(PlatoTest, CheckSromObjectiveGradient)
{
    // ********* ALLOCATE BETA DISTRIBUTION *********
    const double tMean = 90;
    const double tMax = 135;
    const double tMin = 67.5;
    const double tVariance = 135;
    std::shared_ptr<Plato::BetaDistribution<double>> tDistribution =
            std::make_shared<Plato::BetaDistribution<double>>(tMin, tMax, tMean, tVariance);

    // ********* CHECK OBJECTIVE GRADIENT *********
    std::ostringstream tOutputMsg;
    const size_t tNumSamples = 4;
    const size_t tMaxNumMoments = 4;
    Plato::Diagnostics<double> tDiagnostics;
    Plato::SromObjective<double> tObjective(tDistribution, tMaxNumMoments, tNumSamples);

    const size_t tNumVectors = 2;
    Plato::StandardMultiVector<double> tControl(tNumVectors, tNumSamples);
    tDiagnostics.checkCriterionGradient(tObjective, tControl, tOutputMsg);
    EXPECT_TRUE(tDiagnostics.didGradientTestPassed());

    int tMyRank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &tMyRank);
    if(tMyRank == static_cast<int>(0))
    {
        std::cout << tOutputMsg.str().c_str();
    }
}

TEST(PlatoTest, CheckSromConstraintGradient)
{
    // ********* CHECK CONSTRAINT GRADIENT *********
    std::shared_ptr<Plato::StandardVectorReductionOperations<double>> tReductions =
            std::make_shared<Plato::StandardVectorReductionOperations<double>>();
    Plato::SromConstraint<double> tConstraint(tReductions);

    const size_t tNumVectors = 2;
    const size_t tNumSamples = 4;
    Plato::StandardMultiVector<double> tControl(tNumVectors, tNumSamples);

    std::ostringstream tOutputMsg;
    Plato::Diagnostics<double> tDiagnostics;
    tDiagnostics.checkCriterionGradient(tConstraint, tControl, tOutputMsg);
    EXPECT_TRUE(tDiagnostics.didGradientTestPassed());

    int tMyRank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &tMyRank);
    if(tMyRank == static_cast<int>(0))
    {
        std::cout << tOutputMsg.str().c_str();
    }
}

TEST(PlatoTest, SromOptimizationProblem)
{
    // ********* ALLOCATE DATA FACTORY *********
    std::shared_ptr<Plato::DataFactory<double>> tDataFactory =
            std::make_shared<Plato::DataFactory<double>>();
    const size_t tNumDuals = 1;
    const size_t tNumSamples = 4;
    const size_t tNumControlVectors = 2;
    tDataFactory->allocateDual(tNumDuals);
    tDataFactory->allocateControl(tNumSamples, tNumControlVectors);

    // ********* ALLOCATE BETA DISTRIBUTION *********
    const double tMean = 90;
    const double tMax = 135;
    const double tMin = 67.5;
    const double tVariance = 135;
    std::shared_ptr<Plato::BetaDistribution<double>> tDistribution =
            std::make_shared<Plato::BetaDistribution<double>>(tMin, tMax, tMean, tVariance);

    // ********* ALLOCATE OBJECTIVE AND CONSTRAINT CRITERIA *********
    const size_t tMaxNumMoments = 4;
    std::shared_ptr<Plato::SromObjective<double>> tSromObjective =
            std::make_shared<Plato::SromObjective<double>>(tDistribution, tMaxNumMoments, tNumSamples);
    EXPECT_EQ(tNumSamples, tSromObjective->getNumSamples());
    EXPECT_EQ(tMaxNumMoments, tSromObjective->getMaxNumMoments());

    std::shared_ptr<Plato::ReductionOperations<double>> tReductions = tDataFactory->getControlReductionOperations().create();
    std::shared_ptr<Plato::SromConstraint<double>> tSromConstraint = std::make_shared<Plato::SromConstraint<double>>(tReductions);
    std::shared_ptr<Plato::CriterionList<double>> tConstraintList = std::make_shared<Plato::CriterionList<double>>();
    tConstraintList->add(tSromConstraint);

    // ********* AUGMENTED LAGRANGIAN STAGE MANAGER *********
    std::shared_ptr<Plato::AugmentedLagrangianStageMng<double>> tStageMng =
            std::make_shared<Plato::AugmentedLagrangianStageMng<double>>(tDataFactory, tSromObjective, tConstraintList);

    // ********* ALLOCATE TRUST REGION ALGORITHM DATA MANAGER *********
    std::shared_ptr<Plato::TrustRegionAlgorithmDataMng<double>> tDataMng =
            std::make_shared<Plato::TrustRegionAlgorithmDataMng<double>>(tDataFactory);

    // ********* SET INTIAL GUESS FOR VECTOR OF SAMPLES *********
    Plato::StandardVector<double> tVector(tNumSamples);
    for(size_t tIndex = 0; tIndex < tNumSamples; tIndex++)
    {
        double tValue = (1. / static_cast<double>(tNumSamples + 1u));
        tVector[tIndex] = static_cast<double>(tIndex + 1u) * tValue;
    }
    size_t tVectorIndex = 0;
    tDataMng->setInitialGuess(tVectorIndex, tVector);
    // ********* SET INTIAL GUESS FOR VECTOR OF PROBABILITIES *********
    double tScalarValue = 1. / static_cast<double>(4);
    tVector.fill(tScalarValue);
    tVectorIndex = 1;
    tDataMng->setInitialGuess(tVectorIndex, tVector);
    // ********* SET UPPER AND LOWER BOUNDS *********
    tScalarValue = 0;
    tDataMng->setControlLowerBounds(tScalarValue);
    tScalarValue = 1;
    tDataMng->setControlUpperBounds(tScalarValue);

    // ********* ALLOCATE KELLEY-SACHS ALGORITHM *********
    Plato::KelleySachsAugmentedLagrangian<double> tAlgorithm(tDataFactory, tDataMng, tStageMng);
    tAlgorithm.solve();

    size_t tGoldIterations = 38;
    EXPECT_EQ(tGoldIterations, tAlgorithm.getNumIterationsDone());

    // ********* TEST RESULTS *********
    size_t tNumVectors = 1;
    Plato::StandardMultiVector<double> tConstraintValues(tNumVectors, tNumDuals);
    tStageMng->getCurrentConstraintValues(tConstraintValues);
    double tTolerance = 1e-5;
    double tGoldValue = -0.00014696838926697708;
    EXPECT_NEAR(tGoldValue, tConstraintValues(0,0), tTolerance);

    Plato::StandardMultiVector<double> tGoldControl(tNumControlVectors, tNumSamples);
    // GOLD SAMPLES
    tGoldControl(0,0) = 0.16377870487690938;
    tGoldControl(0,1) = 0.409813078171542;
    tGoldControl(0,2) = 0.56692565516265081;
    tGoldControl(0,3) = 0.28805773602398665;
    // GOLD PROBABILITIES
    tGoldControl(1,0) = 0.24979311097918996;
    tGoldControl(1,1) = 0.24999286050154013;
    tGoldControl(1,2) = 0.25018122221660277;
    tGoldControl(1,3) = 0.24988583791340019;
    PlatoTest::checkMultiVectorData(tDataMng->getCurrentControl(), tGoldControl);

    // TEST ACCESSOR FUNCTIONS
    EXPECT_NEAR(0.011192979601439203, tSromObjective->getCumulativeDistributionFunctionError(), tTolerance);
    EXPECT_EQ(1, tSromObjective->getMomentError().getNumVectors());
    const size_t tRANDOM_VEC_DIM = 0;
    EXPECT_EQ(tMaxNumMoments, tSromObjective->getMomentError(tRANDOM_VEC_DIM).size());
    EXPECT_NEAR(0.0051166144050895333, tSromObjective->getMomentError(tRANDOM_VEC_DIM)[0], tTolerance);
    EXPECT_NEAR(0.0041708441435645144, tSromObjective->getMomentError(tRANDOM_VEC_DIM)[1], tTolerance);
    EXPECT_NEAR(0.0001597576589617835, tSromObjective->getMomentError(tRANDOM_VEC_DIM)[2], tTolerance);
    EXPECT_NEAR(0.0052769725525790403, tSromObjective->getMomentError(tRANDOM_VEC_DIM)[3], tTolerance);
    EXPECT_EQ(tMaxNumMoments, tSromObjective->getTrueMoments().size());
    EXPECT_NEAR(0.33333333333333337, tSromObjective->getTrueMoments()[0], tTolerance);
    EXPECT_NEAR(0.1407407407407405, tSromObjective->getTrueMoments()[1], tTolerance);
    EXPECT_NEAR(0.06899055918663749, tSromObjective->getTrueMoments()[2], tTolerance);
    EXPECT_NEAR(0.037521181312030978, tSromObjective->getTrueMoments()[3], tTolerance);
    EXPECT_EQ(tMaxNumMoments, tSromObjective->getSromMoments().size());
    EXPECT_NEAR(0.35717683793472566, tSromObjective->getSromMoments()[0], tTolerance);
    EXPECT_NEAR(0.14983006952307662, tSromObjective->getSromMoments()[1], tTolerance);
    EXPECT_NEAR(0.069862567266374576, tSromObjective->getSromMoments()[2], tTolerance);
    EXPECT_NEAR(0.034795538633737751, tSromObjective->getSromMoments()[3], tTolerance);
    EXPECT_EQ(1, tSromObjective->getTrueCDF().getNumVectors());
    EXPECT_EQ(tNumSamples, tSromObjective->getTrueCDF(tRANDOM_VEC_DIM).size());
    EXPECT_NEAR(0.17821214529480175, tSromObjective->getTrueCDF(tRANDOM_VEC_DIM)[0], tTolerance);
    EXPECT_NEAR(0.68420782522934132, tSromObjective->getTrueCDF(tRANDOM_VEC_DIM)[1], tTolerance);
    EXPECT_NEAR(0.89353339757926531, tSromObjective->getTrueCDF(tRANDOM_VEC_DIM)[2], tTolerance);
    EXPECT_NEAR(0.44147406832303915, tSromObjective->getTrueCDF(tRANDOM_VEC_DIM)[3], tTolerance);
    EXPECT_EQ(1, tSromObjective->getSromCDF().getNumVectors());
    EXPECT_EQ(tNumSamples, tSromObjective->getSromCDF(tRANDOM_VEC_DIM).size());
    EXPECT_NEAR(0.12489655548959624, tSromObjective->getSromCDF(tRANDOM_VEC_DIM)[0], tTolerance);
    EXPECT_NEAR(0.62467537914336646, tSromObjective->getSromCDF(tRANDOM_VEC_DIM)[1], tTolerance);
    EXPECT_NEAR(0.87476242050244046, tSromObjective->getSromCDF(tRANDOM_VEC_DIM)[2], tTolerance);
    EXPECT_NEAR(0.37473602993589383, tSromObjective->getSromCDF(tRANDOM_VEC_DIM)[3], tTolerance);
}

TEST(PlatoTest, solveUncertaintyProblem_checkSize)
{
    // POSE INPUT DISTRIBUTION
    Plato::UncertaintyInputStruct<double, size_t> tInput;
    tInput.mDistribution = Plato::DistrubtionName::type_t::beta;
    tInput.mMean = 1.0;
    tInput.mLowerBound = 0.0;
    tInput.mUpperBound = 2.0;
    tInput.mVariance = 0.5;
    tInput.mNumSamples = 5;

    // SOLVE
    Plato::AlgorithmParamStruct<double, size_t> tParam;
    Plato::SromProblemDiagnosticsStruct<double> tDiagnostics;
    std::vector<Plato::UncertaintyOutputStruct<double>> tOutput;
    Plato::solve_uncertainty(tInput, tParam, tDiagnostics, tOutput);

    // CHECK
    EXPECT_EQ(tOutput.size(), tInput.mNumSamples);

    // CHECK DIAGNOSTICS OUTPUT
    EXPECT_EQ(tInput.mNumSamples, tDiagnostics.mSromCDF.size());
    EXPECT_EQ(tInput.mNumSamples, tDiagnostics.mTrueCDF.size());
    EXPECT_EQ(tInput.mMaxNumDistributionMoments, tDiagnostics.mSromMoments.size());
    EXPECT_EQ(tInput.mMaxNumDistributionMoments, tDiagnostics.mTrueMoments.size());
    EXPECT_EQ(tInput.mMaxNumDistributionMoments, tDiagnostics.mMomentErrors.size());

    const double tTolerance = 1e-6;
    EXPECT_NEAR(0.031222598068113901, tDiagnostics.mCumulativeDistributionFunctionError, tTolerance);
    EXPECT_NEAR(0.0080309885157028253, tDiagnostics.mMomentErrors[0], tTolerance);
    EXPECT_NEAR(0.00059230417679088476, tDiagnostics.mMomentErrors[1], tTolerance);
    EXPECT_NEAR(0.0003054453504334667, tDiagnostics.mMomentErrors[2], tTolerance);
    EXPECT_NEAR(0.0016131419470630369, tDiagnostics.mMomentErrors[3], tTolerance);
    EXPECT_NEAR(0.54480789136888397, tDiagnostics.mSromMoments[0], tTolerance);
    EXPECT_NEAR(0.38412648754238005, tDiagnostics.mSromMoments[1], tTolerance);
    EXPECT_NEAR(0.30703843909644246, tDiagnostics.mSromMoments[2], tTolerance);
    EXPECT_NEAR(0.26245517309625521, tDiagnostics.mSromMoments[3], tTolerance);
    EXPECT_NEAR(0.5, tDiagnostics.mTrueMoments[0], tTolerance);
    EXPECT_NEAR(0.375, tDiagnostics.mTrueMoments[1], tTolerance);
    EXPECT_NEAR(0.3125, tDiagnostics.mTrueMoments[2], tTolerance);
    EXPECT_NEAR(0.2734375, tDiagnostics.mTrueMoments[3], tTolerance);
    EXPECT_NEAR(0.099861401416886472, tDiagnostics.mSromCDF[0], tTolerance);
    EXPECT_NEAR(0.29957760047036575, tDiagnostics.mSromCDF[1], tTolerance);
    EXPECT_NEAR(0.49931471064563576, tDiagnostics.mSromCDF[2], tTolerance);
    EXPECT_NEAR(0.69915967725924877, tDiagnostics.mSromCDF[3], tTolerance);
    EXPECT_NEAR(0.24487127477497825, tDiagnostics.mTrueCDF[0], tTolerance);
    EXPECT_NEAR(0.3909744339238192, tDiagnostics.mTrueCDF[1], tTolerance);
    EXPECT_NEAR(0.51924481068784534, tDiagnostics.mTrueCDF[2], tTolerance);
    EXPECT_NEAR(0.66196029192642658, tDiagnostics.mTrueCDF[3], tTolerance);
}

TEST(PlatoTest, solveUncertaintyProblem_beta)
{
    const double tTol = 1e-6;

    // POSE PROBLEM WITH KNOWN SOLUTION
    Plato::UncertaintyInputStruct<double, size_t> tInput;
    tInput.mDistribution = Plato::DistrubtionName::type_t::beta;
    tInput.mMean = 90.;
    tInput.mUpperBound = 135.;
    tInput.mLowerBound = 67.5;
    tInput.mVariance = 135.;
    tInput.mNumSamples = 4;
    tInput.mMaxNumDistributionMoments = 4;

    // SOLVE
    Plato::AlgorithmParamStruct<double, size_t> tParam;
    Plato::SromProblemDiagnosticsStruct<double> tDiagnostics;
    std::vector<Plato::UncertaintyOutputStruct<double>> tOutput;
    Plato::solve_uncertainty(tInput, tParam, tDiagnostics, tOutput);

    // CHECK
    ASSERT_EQ(tOutput.size(), tInput.mNumSamples);
    // GOLD SAMPLES
    EXPECT_NEAR(tOutput[0].mSampleValue, 78.555110973752846, tTol);
    EXPECT_NEAR(tOutput[1].mSampleValue, 95.162418927276619, tTol);
    EXPECT_NEAR(tOutput[2].mSampleValue, 105.76745339378641, tTol);
    EXPECT_NEAR(tOutput[3].mSampleValue, 86.943956034155335, tTol);
    // GOLD PROBABILITIES
    EXPECT_NEAR(tOutput[0].mSampleWeight, 0.2496939761249817, tTol);
    EXPECT_NEAR(tOutput[1].mSampleWeight, 0.24989448275152742, tTol);
    EXPECT_NEAR(tOutput[2].mSampleWeight, 0.25008291182335385, tTol);
    EXPECT_NEAR(tOutput[3].mSampleWeight, 0.24978709886031122, tTol);
    // expect total probability of unity
    const double tTotalProbability = tOutput[0].mSampleWeight + tOutput[1].mSampleWeight + tOutput[2].mSampleWeight
                                     + tOutput[3].mSampleWeight;
    EXPECT_NEAR(tTotalProbability, 0.99945846956017426, tTol);
}

TEST(PlatoTest, solveUncertaintyProblem_uniform)
{
    const double tTol = 1e-6;

    // POSE PROBLEM WITH KNOWN SOLUTION
    Plato::UncertaintyInputStruct<double, size_t> tInput;
    tInput.mDistribution = Plato::DistrubtionName::type_t::uniform;
    tInput.mMean = 0.;
    tInput.mUpperBound = 75.;
    tInput.mLowerBound = 25.;
    tInput.mVariance = 0.;
    tInput.mNumSamples = 5;
    tInput.mMaxNumDistributionMoments = 4;

    // SOLVE
    Plato::AlgorithmParamStruct<double, size_t> tParam;
    Plato::SromProblemDiagnosticsStruct<double> tDiagnostics;
    std::vector<Plato::UncertaintyOutputStruct<double>> tOutput;
    Plato::solve_uncertainty(tInput, tParam, tDiagnostics, tOutput);

    // CHECK
    ASSERT_EQ(tOutput.size(), tInput.mNumSamples);
    // GOLD SAMPLES
    EXPECT_NEAR(tOutput[0].mSampleValue, 33.36670189468537, tTol);
    EXPECT_NEAR(tOutput[1].mSampleValue, 41.700254108657575, tTol);
    EXPECT_NEAR(tOutput[2].mSampleValue, 50.033818262841237, tTol);
    EXPECT_NEAR(tOutput[3].mSampleValue, 58.367394569475238, tTol);
    EXPECT_NEAR(tOutput[4].mSampleValue, 66.700983245423743, tTol);
    // GOLD PROBABILITIES
    EXPECT_NEAR(tOutput[0].mSampleWeight, 0.199629021, tTol);
    EXPECT_NEAR(tOutput[1].mSampleWeight, 0.199696349, tTol);
    EXPECT_NEAR(tOutput[2].mSampleWeight, 0.199829875, tTol);
    EXPECT_NEAR(tOutput[3].mSampleWeight, 0.200029615, tTol);
    EXPECT_NEAR(tOutput[4].mSampleWeight, 0.200295595, tTol);
    // expect total probability of unity
    double tTotalProbability = 0;
    for(size_t tIndex = 0; tIndex < tInput.mNumSamples; tIndex++)
    {
        tTotalProbability += tOutput[tIndex].mSampleWeight;
    }
    EXPECT_NEAR(tTotalProbability, 0.99948045716825895, tTol);
}

TEST(PlatoTest, solveUncertaintyProblem_normal)
{
    const double tTol = 1e-6;

    // POSE PROBLEM WITH KNOWN SOLUTION
    Plato::UncertaintyInputStruct<double, size_t> tInput;
    tInput.mDistribution = Plato::DistrubtionName::type_t::normal;
    tInput.mMean = 90.;
    tInput.mUpperBound = 0.;
    tInput.mLowerBound = 0.;
    tInput.mVariance = 45. * 45.;
    tInput.mNumSamples = 6;
    tInput.mMaxNumDistributionMoments = 5;

    // SOLVE
    Plato::AlgorithmParamStruct<double, size_t> tParam;
    Plato::SromProblemDiagnosticsStruct<double> tDiagnostics;
    std::vector<Plato::UncertaintyOutputStruct<double>> tOutput;
    Plato::solve_uncertainty(tInput, tParam, tDiagnostics, tOutput);

    // CHECK
    ASSERT_EQ(tOutput.size(), tInput.mNumSamples);
    // GOLD SAMPLES
    EXPECT_NEAR(tOutput[0].mSampleValue, 0.,tTol);//0.14289452, tTol);
    EXPECT_NEAR(tOutput[1].mSampleValue, 0.,tTol);//0.28575572, tTol);
    EXPECT_NEAR(tOutput[2].mSampleValue, 0.,tTol);//0.42861694, tTol);
    EXPECT_NEAR(tOutput[3].mSampleValue, 0.,tTol);//0.57147825, tTol);
    EXPECT_NEAR(tOutput[4].mSampleValue, 0.,tTol);//0.71433961, tTol);
    EXPECT_NEAR(tOutput[5].mSampleValue, 0.,tTol);//0.85720098, tTol);
    // GOLD PROBABILITIES
    EXPECT_NEAR(tOutput[0].mSampleWeight, 0.13995699, tTol);
    EXPECT_NEAR(tOutput[1].mSampleWeight, 0.14407919, tTol);
    EXPECT_NEAR(tOutput[2].mSampleWeight, 0.15301207, tTol);
    EXPECT_NEAR(tOutput[3].mSampleWeight, 0.16684967, tTol);
    EXPECT_NEAR(tOutput[4].mSampleWeight, 0.18573693, tTol);
    EXPECT_NEAR(tOutput[5].mSampleWeight, 0.20986992, tTol);
    // expect total probability of unity
    const double tTotalProbability = tOutput[0].mSampleWeight + tOutput[1].mSampleWeight + tOutput[2].mSampleWeight
                                     + tOutput[3].mSampleWeight
                                     + tOutput[4].mSampleWeight
                                     + tOutput[5].mSampleWeight;
    EXPECT_NEAR(tTotalProbability, 0.99950480285612575, tTol);
}

TEST(PlatoTest, OutputSromDiagnostics)
{
    Plato::SromProblemDiagnosticsStruct<double> tDiagnostics;
    tDiagnostics.mCumulativeDistributionFunctionError = 0.01;
    tDiagnostics.mMomentErrors.resize(4);
    tDiagnostics.mMomentErrors[0] = 0.020;
    tDiagnostics.mMomentErrors[1] = 0.021;
    tDiagnostics.mMomentErrors[2] = 0.022;
    tDiagnostics.mMomentErrors[3] = 0.023;
    Plato::output_srom_diagnostics<double, size_t>(tDiagnostics);

    std::ifstream tInputFile;
    tInputFile.open("plato_srom_diagnostics.txt");
    std::string tData;
    std::stringstream tDataFromFile;
    while(tInputFile >> tData)
    {
        tDataFromFile << tData.c_str();
    }
    tInputFile.close();
    std::system("rm -f plato_srom_diagnostics.txt");

    std::stringstream tGold;
    tGold << "PlatoEnginev.1.0:Copyright2018,NationalTechnology&EngineeringSolutionsofSandia,LLC(NTESS).";
    tGold << "CumulativeDistributionFunction(CDF)Mismatch=1.000000e-02--------------------------------|";
    tGold << "StatisticalMomentsMismatch|--------------------------------|NameOrderError|--------------------------------|";
    tGold << "Mean12.000e-02||Variance22.100e-02||Skewness32.200e-02||Kurtosis42.300e-02|--------------------------------";
    ASSERT_STREQ(tDataFromFile.str().c_str(), tGold.str().c_str());
}

} //namespace PlatoTest
