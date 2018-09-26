/*
 * Plato_KelleySachsBoundConstrained.hpp
 *
 *  Created on: Dec 20, 2017
 */

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

#ifndef PLATO_KELLEYSACHSBOUNDCONSTRAINED_HPP_
#define PLATO_KELLEYSACHSBOUNDCONSTRAINED_HPP_

#include <cmath>
#include <memory>
#include <sstream>
#include <cassert>
#include <iostream>
#include <algorithm>

#include "Plato_Types.hpp"
#include "Plato_ErrorChecks.hpp"
#include "Plato_DataFactory.hpp"
#include "Plato_MultiVector.hpp"
#include "Plato_OptimizersIO.hpp"
#include "Plato_LinearAlgebra.hpp"
#include "Plato_KelleySachsStepMng.hpp"
#include "Plato_KelleySachsAlgorithm.hpp"
#include "Plato_ProjectedSteihaugTointPcg.hpp"
#include "Plato_TrustRegionAlgorithmDataMng.hpp"
#include "Plato_ReducedSpaceTrustRegionStageMng.hpp"

namespace Plato
{

template<typename ScalarType, typename OrdinalType = size_t>
class KelleySachsBoundConstrained : public Plato::KelleySachsAlgorithm<ScalarType, OrdinalType>
{
public:
    /******************************************************************************//**
     * @brief Constructor
     * @param [in] aDataFactory enables construction of core vector and multi-vector data structures
     * @param [in] aDataMng data manager for Kelley-Sachs algorithms
     * @param [in] aStageMng interface to objective and constraint evaluations
    **********************************************************************************/
    KelleySachsBoundConstrained(const std::shared_ptr<Plato::DataFactory<ScalarType, OrdinalType>> & aDataFactory,
                                const std::shared_ptr<Plato::TrustRegionAlgorithmDataMng<ScalarType, OrdinalType>> & aDataMng,
                                const std::shared_ptr<Plato::ReducedSpaceTrustRegionStageMng<ScalarType, OrdinalType>> & aStageMng) :
            Plato::KelleySachsAlgorithm<ScalarType, OrdinalType>(*aDataFactory),
            mPrintDiagnostics(false),
            mOptimalityTolerance(1e-5),
            mOutputData(),
            mGradient(aDataFactory->control().create()),
            mStepMng(std::make_shared<Plato::KelleySachsStepMng<ScalarType, OrdinalType>>(*aDataFactory)),
            mSolver(std::make_shared<Plato::ProjectedSteihaugTointPcg<ScalarType, OrdinalType>>(*aDataFactory)),
            mDataMng(aDataMng),
            mStageMng(aStageMng)
    {
        this->initialize();
    }

    /******************************************************************************//**
     * @brief Destructor
    **********************************************************************************/
    virtual ~KelleySachsBoundConstrained()
    {
    }

    /******************************************************************************//**
     * @brief Enable output of diagnostics (i.e. optimization problem status)
    **********************************************************************************/
    void enableDiagnostics()
    {
        mPrintDiagnostics = true;
    }

    /******************************************************************************//**
     * @brief Set optimality tolerance, i.e. \f$\Vert\nabla{F}(x)\Vert\leq\epsilon\f$
     * @return [in] aInput optimality tolerance
    **********************************************************************************/
    void setOptimalityTolerance(const ScalarType & aInput)
    {
        mOptimalityTolerance = aInput;
    }

    /******************************************************************************//**
     * @brief Set maximum number of trust region subproblem iterations
     * @return [in] aInput maximum number of trust region subproblem iterations
    **********************************************************************************/
    void setMaxNumTrustRegionSubProblemIterations(const OrdinalType & aInput)
    {
        mStepMng->setMaxNumTrustRegionSubProblemIterations(aInput);
    }

    /******************************************************************************//**
     * @brief Set minimum trust region radius
     * @return [in] aInput minimum trust region radius
    **********************************************************************************/
    void setMinTrustRegionRadius(const ScalarType & aInput)
    {
        mStepMng->setMinTrustRegionRadius(aInput);
    }

    /******************************************************************************//**
     * @brief Set maximum trust region radius
     * @return [in] aInput maximum trust region radius
    **********************************************************************************/
    void setMaxTrustRegionRadius(const ScalarType & aInput)
    {
        mStepMng->setMaxTrustRegionRadius(aInput);
    }

    /******************************************************************************//**
     * @brief Set trust region radius expansion factor
     * @return [in] aInput trust region radius expansion factor
    **********************************************************************************/
    void setTrustRegionExpansion(const ScalarType & aInput)
    {
        mStepMng->setTrustRegionExpansion(aInput);
    }

    /******************************************************************************//**
     * @brief Set trust region radius contraction factor
     * @return [in] aInput trust region radius contraction factor
    **********************************************************************************/
    void setTrustRegionContraction(const ScalarType & aInput)
    {
        mStepMng->setTrustRegionContraction(aInput);
    }

    /******************************************************************************//**
     * @brief Solve optimization problem
    **********************************************************************************/
    void solve()
    {
        this->openOutputFile();
        this->checkInitialGuess();
        this->setInitialProblemData();
        mStageMng->updateOptimizationData(*mDataMng);
        this->outputDiagnostics();

        OrdinalType tIteration = 0;
        while(1)
        {
            tIteration++;
            this->setNumIterationsDone(tIteration);
            mDataMng->cacheCurrentStageData();
            // Compute adaptive constants to ensure superlinear convergence
            this->computeAdaptiveConstant();
            // Solve trust region subproblem
            mStepMng->solveSubProblem(*mDataMng, *mStageMng, *mSolver);
            // Update mid objective, control, and gradient information if necessary
            this->update();
            // Update stage manager data
            mStageMng->updateOptimizationData(*mDataMng);
            // output diagnostics
            this->outputDiagnostics();
            if(this->checkStoppingCriteria() == true)
            {
                this->outputMyStoppingCriterion();
                this->closeOutputFile();
                break;
            }
        }
    }

private:
    /******************************************************************************//**
     * @brief Open output file (i.e. diagnostics file)
    **********************************************************************************/
    void openOutputFile()
    {
        if(mPrintDiagnostics == true)
        {
            const Plato::CommWrapper& tMyCommWrapper = mDataMng->getCommWrapper();
            if(tMyCommWrapper.myProcID() == 0)
            {
                mOutputStream.open("plato_ksbc_algorithm_diagnostics.txt");
                Plato::print_ksbc_diagnostics_header(mOutputStream, mPrintDiagnostics);
            }
        }
    }

    /******************************************************************************//**
     * @brief Close output file (i.e. diagnostics file)
    **********************************************************************************/
    void closeOutputFile()
    {
        if(mPrintDiagnostics == true)
        {
            const Plato::CommWrapper& tMyCommWrapper = mDataMng->getCommWrapper();
            if(tMyCommWrapper.myProcID() == 0)
            {
                mOutputStream.close();
            }
        }
    }

    /******************************************************************************//**
     * @brief Print diagnostics for Kelley-Sachs bound constrained algorithm
    **********************************************************************************/
    void outputDiagnostics()
    {
        if(mPrintDiagnostics == false)
        {
            return;
        }

        const Plato::CommWrapper& tMyCommWrapper = mDataMng->getCommWrapper();
        if(tMyCommWrapper.myProcID() == 0)
        {
            mOutputData.mNumIter = this->getNumIterationsDone();
            mOutputData.mNumIterPCG = mSolver->getNumIterationsDone();
            mOutputData.mObjFuncValue = mDataMng->getCurrentObjectiveFunctionValue();
            mOutputData.mObjFuncCount = mDataMng->getNumObjectiveFunctionEvaluations();
            mOutputData.mNumTrustRegionIter = mStepMng->getNumTrustRegionSubProblemItrDone();

            mOutputData.mActualRed = mStepMng->getActualReduction();
            mOutputData.mAredOverPred = mStepMng->getActualOverPredictedReduction();
            mOutputData.mNormObjFuncGrad = mDataMng->getNormProjectedGradient();
            mOutputData.mTrustRegionRadius = mStepMng->getTrustRegionRadius();
            mOutputData.mStationarityMeasure = mDataMng->getStationarityMeasure();
            mOutputData.mControlStagnationMeasure = mDataMng->getControlStagnationMeasure();
            mOutputData.mObjectiveStagnationMeasure = mDataMng->getObjectiveStagnationMeasure();

            Plato::print_ksbc_diagnostics(mOutputData, mOutputStream, mPrintDiagnostics);
        }
    }

    /******************************************************************************//**
     * @brief Print stopping criterion into diagnostics file.
    **********************************************************************************/
    void outputMyStoppingCriterion()
    {
        if(mPrintDiagnostics == true)
        {
            const Plato::CommWrapper& tMyCommWrapper = mDataMng->getCommWrapper();
            if(tMyCommWrapper.myProcID() == 0)
            {
                std::string tReason;
                Plato::algorithm::stop_t tStopCriterion = this->getStoppingCriterion();
                Plato::get_stop_criterion(tStopCriterion, tReason);
                mOutputStream << tReason.c_str();
            }
        }
    }

    /******************************************************************************//**
     * @brief Compute adaptive constants to ensure superlinear convergence
    **********************************************************************************/
    void computeAdaptiveConstant()
    {
        ScalarType tStationarityMeasure = mDataMng->getStationarityMeasure();
        ScalarType tValue = std::pow(tStationarityMeasure, static_cast<ScalarType>(0.75));
        ScalarType tEpsilon = std::min(static_cast<ScalarType>(1e-3), tValue);
        mStepMng->setEpsilonConstant(tEpsilon);
        tValue = std::pow(tStationarityMeasure, static_cast<ScalarType>(0.95));
        ScalarType tEta = static_cast<ScalarType>(0.1) * std::min(static_cast<ScalarType>(1e-1), tValue);
        mStepMng->setEtaConstant(tEta);
    }

    /******************************************************************************//**
     * @brief Set initial trust region radius to initial norm of the gradient
    **********************************************************************************/
    void setInitialTrustRegionRadius()
    {
        if(mStepMng->isInitialTrustRegionRadiusSetToNormProjectedGradient() == true)
        {
            ScalarType tTrustRegionRadius = mDataMng->getNormProjectedGradient();
            const ScalarType tMaxTrustRegionRadius = mStepMng->getMaxTrustRegionRadius();
            if(tTrustRegionRadius > tMaxTrustRegionRadius)
            {
                tTrustRegionRadius = tMaxTrustRegionRadius;
            }
            mStepMng->setTrustRegionRadius(tTrustRegionRadius);
        }
    }

    /******************************************************************************//**
     * @brief Compute initial objective value and gradient.
    **********************************************************************************/
    void setInitialProblemData()
    {
        const Plato::MultiVector<ScalarType, OrdinalType> & tCurrentControl = mDataMng->getCurrentControl();
        ScalarType tTolerance = mStepMng->getObjectiveInexactnessTolerance();
        ScalarType tCurrentObjFuncValue = mStageMng->evaluateObjective(tCurrentControl, tTolerance);
        mDataMng->setCurrentObjectiveFunctionValue(tCurrentObjFuncValue);
        mStageMng->cacheData();
        mStageMng->computeGradient(tCurrentControl, *mGradient);
        mDataMng->setCurrentGradient(*mGradient);
        mDataMng->computeNormProjectedGradient();
        this->setInitialTrustRegionRadius();
        mDataMng->computeStationarityMeasure();
    }

    /******************************************************************************//**
     * @brief Check and initialize control lower and upper bounds
    **********************************************************************************/
    void initialize()
    {
        // Check Bounds
        try
        {
            const Plato::MultiVector<ScalarType, OrdinalType> & tLowerBounds = mDataMng->getControlLowerBounds();
            const Plato::MultiVector<ScalarType, OrdinalType> & tUpperBounds = mDataMng->getControlUpperBounds();
            Plato::error::checkBounds(tLowerBounds, tUpperBounds);
        }
        catch(const std::invalid_argument & tError)
        {
            std::cout << tError.what() << std::flush;
            throw tError;
        }
    }

    /******************************************************************************//**
     * @brief Check that control initial guess is within bounds
    **********************************************************************************/
    void checkInitialGuess()
    {
        try
        {
            bool tIsInitialGuessSet = mDataMng->isInitialGuessSet();
            Plato::error::checkInitialGuessIsSet(tIsInitialGuessSet);
        }
        catch(const std::invalid_argument & tError)
        {
            std::cout << tError.what() << std::flush;
            throw tError;
        }

        try
        {
            const Plato::MultiVector<ScalarType, OrdinalType> & tControl = mDataMng->getCurrentControl();
            const Plato::MultiVector<ScalarType, OrdinalType> & tLowerBounds = mDataMng->getControlLowerBounds();
            const Plato::MultiVector<ScalarType, OrdinalType> & tUpperBounds = mDataMng->getControlUpperBounds();
            Plato::error::checkInitialGuess(tControl, tLowerBounds, tUpperBounds);
        }
        catch(const std::invalid_argument & tError)
        {
            std::cout << tError.what() << std::flush;
            throw tError;
        }

        const Plato::MultiVector<ScalarType, OrdinalType> & tControl = mDataMng->getCurrentControl();
        const Plato::MultiVector<ScalarType, OrdinalType> & tLowerBounds = mDataMng->getControlLowerBounds();
        const Plato::MultiVector<ScalarType, OrdinalType> & tUpperBounds = mDataMng->getControlUpperBounds();
        std::shared_ptr<Plato::MultiVector<ScalarType, OrdinalType>> tWorkMultiVector = tControl.create();
        Plato::update(static_cast<ScalarType>(1), tControl, static_cast<ScalarType>(0), *tWorkMultiVector);
        mDataMng->bounds().project(tLowerBounds, tUpperBounds, *tWorkMultiVector);
        mDataMng->setCurrentControl(*tWorkMultiVector);
    }

    /******************************************************************************//**
     * @brief Update trial control if current mid-point actual reduction tolerance is violated.
    **********************************************************************************/
    void update()
    {
        // Cache state data since trial control was accepted
        mStageMng->cacheData();
        // Compute gradient at new midpoint
        const Plato::MultiVector<ScalarType, OrdinalType> & tMidControl = mStepMng->getMidPointControls();
        mStageMng->computeGradient(tMidControl, *mGradient);

        if(this->updateControl(*mGradient, *mStepMng, *mDataMng, *mStageMng) == true)
        {
            // Update new gradient and inequality constraint values since control
            // was successfully updated; else, keep mid gradient and thus mid control.
            mStageMng->cacheData();
            const Plato::MultiVector<ScalarType, OrdinalType> & tCurrentControl = mDataMng->getCurrentControl();
            mStageMng->computeGradient(tCurrentControl, *mGradient);
            mDataMng->setCurrentGradient(*mGradient);
        }
        else
        {
            // Keep current objective function, control, and gradient values at mid point
            const ScalarType tMidObjectiveFunctionValue = mStepMng->getMidPointObjectiveFunctionValue();
            mDataMng->setCurrentObjectiveFunctionValue(tMidObjectiveFunctionValue);
            mDataMng->setCurrentControl(tMidControl);
            mDataMng->setCurrentGradient(*mGradient);
        }

        // Compute stationarity measure
        mDataMng->computeStationarityMeasure();
        // Compute norm of projected gradient
        mDataMng->computeNormProjectedGradient();
        // Compute control stagnation measure
        mDataMng->computeControlStagnationMeasure();
        // Compute objective stagnation measure
        mDataMng->computeObjectiveStagnationMeasure();
        // compute gradient inexactness bound for next trust region sub-problem solve
        ScalarType tNormProjectedGradient = mDataMng->getNormProjectedGradient();
        mStepMng->updateGradientInexactnessTolerance(tNormProjectedGradient);
    }

    /******************************************************************************//**
     * @brief Check stopping criteria
     * @return stopping criteria flag (true = stop algorithm, false = continue to next iteration)
    **********************************************************************************/
    bool checkStoppingCriteria()
    {
        bool tStop = false;
        if(this->checkNaN() == true)
        {
            // Stop optimization algorithm: NaN criterion measure detected
            tStop = true;
            mDataMng->resetCurrentStageDataToPreviousStageData();
        }
        else
        {
            // Get termination criteria
            const OrdinalType tNumIterations = this->getNumIterationsDone();
            const ScalarType tActualReduction = mStepMng->getActualReduction();
            const ScalarType tStationarityMeasure = mDataMng->getStationarityMeasure();
            const ScalarType tCurrentTrustRegionRadius = mStepMng->getTrustRegionRadius();
            const ScalarType tStagnationMeasure = mDataMng->getObjectiveStagnationMeasure();
            const ScalarType tControlStagnationMeasure = mDataMng->getControlStagnationMeasure();
            // Get termination criteria tolerances
            const OrdinalType tMaxNumIterations = this->getMaxNumIterations();
            const ScalarType tStagnationTolerance = this->getStagnationTolerance();
            const ScalarType tStationarityTolerance = this->getStationarityTolerance();
            const ScalarType tMinTrustRegionRadius = mStepMng->getMinTrustRegionRadius();
            const ScalarType tActualReductionTolerance = this->getActualReductionTolerance();
            const ScalarType tControlStagnationTolerance = this->getControlStagnationTolerance();
            if( tStationarityMeasure <= tStationarityTolerance )
            {
                tStop = true;
                this->setStoppingCriterion(Plato::algorithm::stop_t::STATIONARITY_MEASURE);
            }
            else if( (tNumIterations > static_cast<OrdinalType>(1)) && (std::abs(tActualReduction) < tActualReductionTolerance) )
            {
                this->setStoppingCriterion(Plato::algorithm::stop_t::ACTUAL_REDUCTION_TOLERANCE);
                tStop = true;
            }
            else if( tStagnationMeasure < tStagnationTolerance )
            {
                tStop = true;
                this->setStoppingCriterion(Plato::algorithm::stop_t::OBJECTIVE_STAGNATION);
            }
            else if( tCurrentTrustRegionRadius < tMinTrustRegionRadius )
            {
                tStop = true;
                this->setStoppingCriterion(Plato::algorithm::stop_t::SMALL_TRUST_REGION_RADIUS);
            }
            else if( tControlStagnationMeasure < tControlStagnationTolerance )
            {
                tStop = true;
                this->setStoppingCriterion(Plato::algorithm::stop_t::CONTROL_STAGNATION);
            }
            else if( tNumIterations >= tMaxNumIterations )
            {
                tStop = true;
                this->setStoppingCriterion(Plato::algorithm::stop_t::MAX_NUMBER_ITERATIONS);
            }
        }

        return (tStop);
    }

    /******************************************************************************//**
     * @brief Check for NaN values
     * @return NaN flag (true = NaN identified, false = NaN not identified)
    **********************************************************************************/
    bool checkNaN()
    {
        const ScalarType tStationarityMeasure = mDataMng->getStationarityMeasure();
        const ScalarType tOptimalityMeasure = mDataMng->getNormProjectedGradient();
        const ScalarType tControlStagnationMeasure = mDataMng->getControlStagnationMeasure();

        bool tNaN_ValueDetected = false;
        if(std::isfinite(tStationarityMeasure) == false)
        {
            tNaN_ValueDetected = true;
            this->setStoppingCriterion(Plato::algorithm::stop_t::NaN_STATIONARITY_MEASURE);
        }
        else if(std::isfinite(tOptimalityMeasure) == false)
        {
            tNaN_ValueDetected = true;
            this->setStoppingCriterion(Plato::algorithm::stop_t::NaN_OBJECTIVE_GRADIENT);
        }
        else if(std::isfinite(tControlStagnationMeasure) == false)
        {
            tNaN_ValueDetected = true;
            this->setStoppingCriterion(Plato::algorithm::stop_t::NaN_CONTROL_STAGNATION);
        }

        return (tNaN_ValueDetected);
    }

private:
    bool mPrintDiagnostics;
    std::ofstream mOutputStream;

    ScalarType mOptimalityTolerance;

    Plato::OutputDataKelleySachs<ScalarType, OrdinalType> mOutputData;
    std::shared_ptr<Plato::MultiVector<ScalarType, OrdinalType>> mGradient;
    std::shared_ptr<Plato::KelleySachsStepMng<ScalarType, OrdinalType>> mStepMng;
    std::shared_ptr<Plato::ProjectedSteihaugTointPcg<ScalarType,OrdinalType>> mSolver;
    std::shared_ptr<Plato::TrustRegionAlgorithmDataMng<ScalarType,OrdinalType>> mDataMng;
    std::shared_ptr<Plato::ReducedSpaceTrustRegionStageMng<ScalarType,OrdinalType>> mStageMng;

private:
    KelleySachsBoundConstrained(const Plato::KelleySachsBoundConstrained<ScalarType, OrdinalType>&);
    Plato::KelleySachsBoundConstrained<ScalarType, OrdinalType> & operator=(const Plato::KelleySachsBoundConstrained<ScalarType, OrdinalType>&);
}; // class KelleySachsBoundConstrained

} // namespace Plato

#endif /* PLATO_KELLEYSACHSBOUNDCONSTRAINED_HPP_ */
