/*
 * Plato_DistributedVector.hpp
 *
 *  Created on: Oct 27, 2017
 */

#ifndef PLATO_DISTRIBUTEDVECTOR_HPP_
#define PLATO_DISTRIBUTEDVECTOR_HPP_

#include <mpi.h>

#include <vector>
#include <memory>
#include <numeric>
#include <cassert>

#include "Plato_Vector.hpp"

namespace Plato
{

template<typename ScalarType, typename OrdinalType = size_t>
class DistributedVector : public Plato::Vector<ScalarType, OrdinalType>
{
public:
    DistributedVector(MPI_Comm aComm, const std::vector<ScalarType> & aInput) :
            mComm(aComm),
            mData(aInput)
    {
    }
    DistributedVector(MPI_Comm aComm, const OrdinalType & aNumElements, ScalarType aValue = 0) :
            mComm(aComm),
            mData(std::vector<ScalarType>(aNumElements, aValue))
    {
    }
    virtual ~DistributedVector()
    {
    }

    //! Scales a Vector by a ScalarType constant.
    void scale(const ScalarType & aInput)
    {
        OrdinalType tLength = this->size();
        for(OrdinalType tIndex = 0; tIndex < tLength; tIndex++)
        {
            mData[tIndex] = aInput * mData[tIndex];
        }
    }
    //! Entry-Wise product of two vectors.
    void entryWiseProduct(const Plato::Vector<ScalarType, OrdinalType> & aInput)
    {
        OrdinalType tMyDataSize = mData.size();
        assert(aInput.size() == tMyDataSize);

        for(OrdinalType tIndex = 0; tIndex < tMyDataSize; tIndex++)
        {
            mData[tIndex] = aInput[tIndex] * mData[tIndex];
        }
    }
    //! Update vector values with scaled values of A, this = beta*this + alpha*A.
    void update(const ScalarType & aAlpha, const Plato::Vector<ScalarType, OrdinalType> & aInputVector, const ScalarType & aBeta)
    {
        OrdinalType tMyDataSize = mData.size();
        assert(aInputVector.size() == tMyDataSize);
        for(OrdinalType tIndex = 0; tIndex < tMyDataSize; tIndex++)
        {
            mData[tIndex] = aBeta * mData[tIndex] + aAlpha * aInputVector[tIndex];
        }
    }
    //! Computes the absolute value of each element in the container.
    void modulus()
    {
        OrdinalType tLength = this->size();
        for(OrdinalType tIndex = 0; tIndex < tLength; tIndex++)
        {
            mData[tIndex] = std::abs(mData[tIndex]);
        }
    }
    //! Returns the inner product of two vectors.
    ScalarType dot(const Plato::Vector<ScalarType, OrdinalType> & aInputVector) const
    {
        assert(aInputVector.size() == static_cast<OrdinalType>(mData.size()));

        const Plato::DistributedVector<ScalarType, OrdinalType>& tInputVector =
                dynamic_cast<const Plato::DistributedVector<ScalarType, OrdinalType>&>(aInputVector);

        ScalarType tBaseValue = 0;
        ScalarType tLocalInnerProduct = std::inner_product(mData.begin(), mData.end(), tInputVector.mData.begin(), tBaseValue);

        ScalarType tGlobalInnerProduct = 0;
        MPI_Allreduce(&tLocalInnerProduct, &tGlobalInnerProduct, 1, MPI_DOUBLE, MPI_SUM, mComm);

        return (tGlobalInnerProduct);
    }
    //! Assigns new contents to the Vector, replacing its current contents, and not modifying its size.
    void fill(const ScalarType & aValue)
    {
        std::fill(mData.begin(), mData.end(), aValue);
    }
    //! Returns the number of local elements in the Vector.
    OrdinalType size() const
    {
        OrdinalType tOutput = mData.size();
        return (tOutput);
    }
    //! Creates an object of type Plato::Vector
    std::shared_ptr<Plato::Vector<ScalarType, OrdinalType>> create() const
    {
        const ScalarType tBaseValue = 0;
        const OrdinalType tNumElements = this->size();
        std::shared_ptr<Plato::Vector<ScalarType, OrdinalType>> tOutput =
                std::make_shared<Plato::DistributedVector<ScalarType, OrdinalType>>(mComm, tNumElements, tBaseValue);
        return (tOutput);
    }
    //! Operator overloads the square bracket operator
    ScalarType & operator [](const OrdinalType & aIndex)
    {
        assert(aIndex < this->size());
        assert(aIndex >= static_cast<OrdinalType>(0));

        return (mData[aIndex]);
    }
    //! Operator overloads the square bracket operator
    const ScalarType & operator [](const OrdinalType & aIndex) const
    {
        assert(aIndex < this->size());
        assert(aIndex >= static_cast<OrdinalType>(0));

        return (mData[aIndex]);
    }
    //! Returns a direct pointer to the memory array used internally by the vector to store its owned elements.
    ScalarType* data()
    {
        return (mData.data());
    }
    //! Returns a direct const pointer to the memory array used internally by the vector to store its owned elements.
    const ScalarType* data() const
    {
        return (mData.data());
    }
    //! Returns a direct reference to underlying array used internally by the vector to store its owned elements.
    std::vector<ScalarType> & vector()
    {
        return (mData);
    }
    //! Returns a direct const reference to underlying array used internally by the vector to store its owned elements.
    const std::vector<ScalarType> & vector() const
    {
        return (mData);
    }

private:
    MPI_Comm mComm;
    std::vector<ScalarType> mData;

private:
    DistributedVector(const Plato::DistributedVector<ScalarType, OrdinalType>&);
    Plato::DistributedVector<ScalarType, OrdinalType> & operator=(const Plato::DistributedVector<ScalarType, OrdinalType>&);
};

} // namespace Plato

#endif /* PLATO_DISTRIBUTEDVECTOR_HPP_ */
