#include <gtest/gtest.h>
#include <mpi.h>
#include <Kokkos_Core.hpp>

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    Kokkos::initialize(argc, argv);

    testing::InitGoogleTest(&argc, argv);
    int returnVal = RUN_ALL_TESTS();

    Kokkos::finalize();
    
    MPI_Finalize();

    return returnVal;
}
