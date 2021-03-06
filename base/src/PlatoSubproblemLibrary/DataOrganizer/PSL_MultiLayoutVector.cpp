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

#include "PSL_MultiLayoutVector.hpp"

#include "PSL_DataSequence.hpp"
#include "PSL_Abstract_DenseVectorOperations.hpp"
#include "PSL_Abstract_DenseMatrix.hpp"
#include "PSL_Abstract_DenseMatrixBuilder.hpp"
#include "PSL_Random.hpp"
#include "PSL_AbstractAuthority.hpp"

#include <cassert>

namespace PlatoSubproblemLibrary
{

MultiLayoutVector::MultiLayoutVector(AbstractAuthority* authority) :
        DataSequence(authority, data_flow_t::data_flow_t::inert_data_flow),
        m_authority(authority)
{
}
MultiLayoutVector::~MultiLayoutVector()
{
}

void MultiLayoutVector::zeros(MultiLayoutVector* other)
{
    // clear old values
    resize_doubles(0);
    resize_double_vectors(0);
    resize_matrices(0);

    // allocate scalars
    const int num_scalars = other->get_doubles_size();
    resize_doubles(num_scalars);

    // allocate vectors
    const int num_vectors = other->get_double_vectors_size();
    resize_double_vectors(num_vectors);
    for(int v = 0; v < num_vectors; v++)
    {
        std::vector<double>* other_vector = other->get_double_vector(v);
        std::vector<double>* this_vector = get_double_vector(v);

        this_vector->resize(other_vector->size());
    }

    // allocate matrices
    const int num_matrices = other->get_matrices_size();
    resize_matrices(num_matrices);
    for(int m = 0; m < num_matrices; m++)
    {
        AbstractInterface::DenseMatrix* other_matrix = other->get_matrix(m);
        AbstractInterface::DenseMatrixBuilder* builder = other_matrix->get_builder();
        AbstractInterface::DenseMatrix** this_matrix_ptr = get_matrix_ptr(m);

        *this_matrix_ptr = builder->build_by_fill(other_matrix->get_num_rows(), other_matrix->get_num_columns(), 0.);
    }
}

void MultiLayoutVector::scale(const double& scale)
{
    // update scalars
    const int num_scalars = get_doubles_size();
    for(int s = 0; s < num_scalars; s++)
    {
        const double this_value = get_double(s);
        const double new_value = scale * this_value;
        set_double(s, new_value);
    }

    // update vectors
    const int num_vectors = get_double_vectors_size();
    for(int v = 0; v < num_vectors; v++)
    {
        std::vector<double>* this_vector = get_double_vector(v);
        m_authority->dense_vector_operations->scale(scale, *this_vector);
    }

    // update matrices
    const int num_matrices = get_matrices_size();
    for(int m = 0; m < num_matrices; m++)
    {
        AbstractInterface::DenseMatrix* this_matrix = get_matrix(m);
        this_matrix->scale(scale);
    }
}

void MultiLayoutVector::axpy(const double& scale, MultiLayoutVector* other)
{
    // update scalars
    const int num_scalars = get_doubles_size();
    assert(num_scalars == other->get_doubles_size());
    for(int s = 0; s < num_scalars; s++)
    {
        const double other_value = other->get_double(s);
        const double this_value = get_double(s);

        const double new_value = scale * other_value + this_value;
        set_double(s, new_value);
    }

    // update vectors
    const int num_vectors = get_double_vectors_size();
    assert(num_vectors == other->get_double_vectors_size());
    for(int v = 0; v < num_vectors; v++)
    {
        std::vector<double>* other_vector = other->get_double_vector(v);
        std::vector<double>* this_vector = get_double_vector(v);

        m_authority->dense_vector_operations->axpy(scale, *other_vector, *this_vector);
    }

    // update matrices
    const int num_matrices = get_matrices_size();
    assert(num_matrices == other->get_matrices_size());
    for(int m = 0; m < num_matrices; m++)
    {
        AbstractInterface::DenseMatrix* other_matrix = other->get_matrix(m);
        AbstractInterface::DenseMatrix* this_matrix = get_matrix(m);

        this_matrix->aXpY(scale, other_matrix);
    }
}

double MultiLayoutVector::dot(MultiLayoutVector* other)
{
    double result = 0.;

    // dot scalars
    assert(get_doubles_size() == other->get_doubles_size());
    std::vector<double> this_scalar_double;
    get_all_doubles(this_scalar_double);
    std::vector<double> other_scalar_double;
    other->get_all_doubles(other_scalar_double);
    result += m_authority->dense_vector_operations->dot(this_scalar_double, other_scalar_double);

    // dot vectors
    const int num_vectors = get_double_vectors_size();
    assert(num_vectors == other->get_double_vectors_size());
    for(int v = 0; v < num_vectors; v++)
    {
        std::vector<double>* other_vector = other->get_double_vector(v);
        std::vector<double>* this_vector = get_double_vector(v);

        result += m_authority->dense_vector_operations->dot(*this_vector, *other_vector);
    }

    // dot matrices
    const int num_matrices = get_matrices_size();
    assert(num_matrices == other->get_matrices_size());
    for(int m = 0; m < num_matrices; m++)
    {
        AbstractInterface::DenseMatrix* other_matrix = other->get_matrix(m);
        AbstractInterface::DenseMatrix* this_matrix = get_matrix(m);

        result += this_matrix->dot(other_matrix);
    }

    return result;
}

void MultiLayoutVector::random_double(const double& lower, const double& upper)
{
    // update scalars
    const int num_scalars = get_doubles_size();
    for(int s = 0; s < num_scalars; s++)
    {
        const double value = ::PlatoSubproblemLibrary::uniform_rand_double(lower, upper);
        set_double(s, value);
    }

    // update vectors
    const int num_vectors = get_double_vectors_size();
    for(int v = 0; v < num_vectors; v++)
    {
        std::vector<double>* this_vector = get_double_vector(v);
        ::PlatoSubproblemLibrary::uniform_rand_double(lower, upper, *this_vector);
    }

    // update matrices
    const int num_matrices = get_matrices_size();
    for(int m = 0; m < num_matrices; m++)
    {
        AbstractInterface::DenseMatrix* this_matrix = get_matrix(m);

        // get random values
        const size_t num_row = this_matrix->get_num_rows();
        const size_t num_columns = this_matrix->get_num_columns();
        std::vector<double> random_doubles(num_row * num_columns);
        ::PlatoSubproblemLibrary::uniform_rand_double(lower, upper, random_doubles);

        // fill random values
        this_matrix->fill_by_row_major(random_doubles);
    }
}

}
