#include <Cajita_Types.hpp>
#include <Cajita_GlobalGrid.hpp>
#include <Cajita_UniformDimPartitioner.hpp>
#include <Cajita_Block.hpp>

#include <gtest/gtest.h>

#include <mpi.h>

#include <numeric>

using namespace Cajita;

namespace Test
{

//---------------------------------------------------------------------------//
void periodicTest()
{
    // Let MPI compute the partitioning for this test.
    UniformDimPartitioner partitioner;

    // Create the global grid.
    double cell_size = 0.23;
    std::vector<int> global_num_cell = { 101, 85, 99 };
    std::vector<bool> is_dim_periodic = {true,true,true};
    std::vector<double> global_low_corner = { 1.2, 3.3, -2.8 };
    std::vector<double> global_high_corner =
        { global_low_corner[0] + cell_size * global_num_cell[0],
          global_low_corner[1] + cell_size * global_num_cell[1],
          global_low_corner[2] + cell_size * global_num_cell[2] };
    auto global_grid = createGlobalGrid( MPI_COMM_WORLD,
                                         partitioner,
                                         is_dim_periodic,
                                         global_low_corner,
                                         global_high_corner,
                                         cell_size );

    // Create a  grid block.
    int halo_width = 2;
    auto grid_block = createBlock( global_grid, halo_width );

    // Check sizes
    EXPECT_EQ( grid_block->haloWidth(), halo_width );

    // Get the local number of cells.
    auto owned_cell_space = grid_block->ownedIndexSpace( Cell() );
    std::vector<int> local_num_cells(3);
    for ( int d = 0; d < 3; ++d )
        local_num_cells[d] = owned_cell_space.extent(d);

    // Compute a global set of local cell size arrays.
    auto grid_comm = global_grid->comm();
    std::vector<int> cart_dims( 3 );
    std::vector<int> cart_period( 3 );
    std::vector<int> cart_rank( 3 );
    MPI_Cart_get(
        grid_comm, 3, cart_dims.data(), cart_period.data(), cart_rank.data() );
    std::vector<int> local_num_cell_i( cart_dims[Dim::I], 0 );
    std::vector<int> local_num_cell_j( cart_dims[Dim::J], 0 );
    std::vector<int> local_num_cell_k( cart_dims[Dim::K], 0 );
    local_num_cell_i[ cart_rank[Dim::I] ] = local_num_cells[Dim::I];
    local_num_cell_j[ cart_rank[Dim::J] ] = local_num_cells[Dim::J];
    local_num_cell_k[ cart_rank[Dim::K] ] = local_num_cells[Dim::K];
    MPI_Allreduce( MPI_IN_PLACE, local_num_cell_i.data(), cart_dims[Dim::I],
                   MPI_INT, MPI_MAX, grid_comm );
    MPI_Allreduce( MPI_IN_PLACE, local_num_cell_j.data(), cart_dims[Dim::J],
                   MPI_INT, MPI_MAX, grid_comm );
    MPI_Allreduce( MPI_IN_PLACE, local_num_cell_k.data(), cart_dims[Dim::K],
                   MPI_INT, MPI_MAX, grid_comm );

    // Check the neighbor rank
    for ( int i = -1; i < 2; ++i )
        for ( int j = -1; j < 2; ++j )
            for ( int k = -1; k < 2; ++k )
            {
                std::vector<int> nr = { cart_rank[Dim::I] + i,
                                        cart_rank[Dim::J] + j,
                                        cart_rank[Dim::K] + k };
                int nrank;
                MPI_Cart_rank( grid_comm, nr.data(), &nrank );
                EXPECT_EQ( grid_block->neighborRank(i,j,k), nrank );
            }

    // Check to make sure we got the right number of total cells in each
    // dimension.
    EXPECT_EQ( global_num_cell[Dim::I],
               std::accumulate(
                   local_num_cell_i.begin(), local_num_cell_i.end(), 0 ) );
    EXPECT_EQ( global_num_cell[Dim::J],
               std::accumulate(
                   local_num_cell_j.begin(), local_num_cell_j.end(), 0 ) );
    EXPECT_EQ( global_num_cell[Dim::K],
               std::accumulate(
                   local_num_cell_k.begin(), local_num_cell_k.end(), 0 ) );

    // Check the local cell bounds.
    EXPECT_EQ( owned_cell_space.min(Dim::I), halo_width );
    EXPECT_EQ( owned_cell_space.max(Dim::I),
               local_num_cells[Dim::I] + halo_width );
    EXPECT_EQ( owned_cell_space.min(Dim::J), halo_width );
    EXPECT_EQ( owned_cell_space.max(Dim::J),
               local_num_cells[Dim::J] + halo_width );
    EXPECT_EQ( owned_cell_space.min(Dim::K), halo_width );
    EXPECT_EQ( owned_cell_space.max(Dim::K),
               local_num_cells[Dim::K] + halo_width );

    // Get the local number of nodes.
    auto owned_node_space = grid_block->ownedIndexSpace( Node() );
    std::vector<int> local_num_nodes(3);
    for ( int d = 0; d < 3; ++d )
        local_num_nodes[d] = owned_node_space.extent(d);

    // Compute a global set of local node size arrays.
    std::vector<int> local_num_node_i( cart_dims[Dim::I], 0 );
    std::vector<int> local_num_node_j( cart_dims[Dim::J], 0 );
    std::vector<int> local_num_node_k( cart_dims[Dim::K], 0 );
    local_num_node_i[ cart_rank[Dim::I] ] = local_num_nodes[Dim::I];
    local_num_node_j[ cart_rank[Dim::J] ] = local_num_nodes[Dim::J];
    local_num_node_k[ cart_rank[Dim::K] ] = local_num_nodes[Dim::K];
    MPI_Allreduce( MPI_IN_PLACE, local_num_node_i.data(), cart_dims[Dim::I],
                   MPI_INT, MPI_MAX, grid_comm );
    MPI_Allreduce( MPI_IN_PLACE, local_num_node_j.data(), cart_dims[Dim::J],
                   MPI_INT, MPI_MAX, grid_comm );
    MPI_Allreduce( MPI_IN_PLACE, local_num_node_k.data(), cart_dims[Dim::K],
                   MPI_INT, MPI_MAX, grid_comm );

    // Check to make sure we got the right number of total cells in each
    // dimension.
    EXPECT_EQ( global_num_cell[Dim::I],
               std::accumulate(
                   local_num_node_i.begin(), local_num_node_i.end(), 0 ) );
    EXPECT_EQ( global_num_cell[Dim::J],
               std::accumulate(
                   local_num_node_j.begin(), local_num_node_j.end(), 0 ) );
    EXPECT_EQ( global_num_cell[Dim::K],
               std::accumulate(
                   local_num_node_k.begin(), local_num_node_k.end(), 0 ) );

    // Check the local node bounds.
    EXPECT_EQ( owned_node_space.min(Dim::I), halo_width );
    EXPECT_EQ( owned_node_space.max(Dim::I),
               local_num_nodes[Dim::I] + halo_width );
    EXPECT_EQ( owned_node_space.min(Dim::J), halo_width );
    EXPECT_EQ( owned_node_space.max(Dim::J),
               local_num_nodes[Dim::J] + halo_width );
    EXPECT_EQ( owned_node_space.min(Dim::K), halo_width );
    EXPECT_EQ( owned_node_space.max(Dim::K),
               local_num_nodes[Dim::K] + halo_width );

    // Check the ghosted cell bounds.
    auto ghosted_cell_space = grid_block->ghostedIndexSpace( Cell() );
    for ( int d = 0; d < 3; ++d )
    {
        EXPECT_EQ( ghosted_cell_space.extent(d),
                   owned_cell_space.extent(d) + 2 * halo_width );
    }

    // Check the ghosted node bounds.
    auto ghosted_node_space = grid_block->ghostedIndexSpace( Node() );
    for ( int d = 0; d < 3; ++d )
    {
        EXPECT_EQ( ghosted_node_space.extent(d),
                   owned_node_space.extent(d) + 2 * halo_width + 1);
    }

    // Check the cells we own that we will share with our neighbors. Cover
    // enough of the neighbors that we know the bounds are correct in each
    // dimension. The three variations here cover all of the cases.
    auto owned_shared_cell_space =
        grid_block->sharedOwnedIndexSpace( Cell(), -1, 0, 1 );
    EXPECT_EQ( owned_shared_cell_space.min(Dim::I),
               owned_cell_space.min(Dim::I) );
    EXPECT_EQ( owned_shared_cell_space.max(Dim::I),
               owned_cell_space.min(Dim::I) + halo_width );
    EXPECT_EQ( owned_shared_cell_space.min(Dim::J),
               owned_cell_space.min(Dim::J) );
    EXPECT_EQ( owned_shared_cell_space.max(Dim::J),
               owned_cell_space.max(Dim::J) );
    EXPECT_EQ( owned_shared_cell_space.min(Dim::K),
               owned_cell_space.max(Dim::K) - halo_width );
    EXPECT_EQ( owned_shared_cell_space.max(Dim::K),
               owned_cell_space.max(Dim::K) );

    owned_shared_cell_space =
        grid_block->sharedOwnedIndexSpace( Cell(), 1, -1, 0 );
    EXPECT_EQ( owned_shared_cell_space.min(Dim::I),
               owned_cell_space.max(Dim::I) - halo_width );
    EXPECT_EQ( owned_shared_cell_space.max(Dim::I),
               owned_cell_space.max(Dim::I) );
    EXPECT_EQ( owned_shared_cell_space.min(Dim::J),
               owned_cell_space.min(Dim::J) );
    EXPECT_EQ( owned_shared_cell_space.max(Dim::J),
               owned_cell_space.min(Dim::J) + halo_width );
    EXPECT_EQ( owned_shared_cell_space.min(Dim::K),
               owned_cell_space.min(Dim::K) );
    EXPECT_EQ( owned_shared_cell_space.max(Dim::K),
               owned_cell_space.max(Dim::K) );

    owned_shared_cell_space =
        grid_block->sharedOwnedIndexSpace( Cell(), 0, 1, -1 );
    EXPECT_EQ( owned_shared_cell_space.min(Dim::I),
               owned_cell_space.min(Dim::I) );
    EXPECT_EQ( owned_shared_cell_space.max(Dim::I),
               owned_cell_space.max(Dim::I) );
    EXPECT_EQ( owned_shared_cell_space.min(Dim::J),
               owned_cell_space.max(Dim::J) - halo_width );
    EXPECT_EQ( owned_shared_cell_space.max(Dim::J),
               owned_cell_space.max(Dim::J) );
    EXPECT_EQ( owned_shared_cell_space.min(Dim::K),
               owned_cell_space.min(Dim::K) );
    EXPECT_EQ( owned_shared_cell_space.max(Dim::K),
               owned_cell_space.min(Dim::K) + halo_width );

    // Check the cells are ghosts that our neighbors own. Cover enough of the
    // neighbors that we know the bounds are correct in each dimension. The
    // three variations here cover all of the cases.
    auto ghosted_shared_cell_space =
        grid_block->sharedGhostedIndexSpace( Cell(), -1, 0, 1 );
    EXPECT_EQ( ghosted_shared_cell_space.min(Dim::I),
               0 );
    EXPECT_EQ( ghosted_shared_cell_space.max(Dim::I),
               halo_width );
    EXPECT_EQ( ghosted_shared_cell_space.min(Dim::J),
               owned_cell_space.min(Dim::J) );
    EXPECT_EQ( ghosted_shared_cell_space.max(Dim::J),
               owned_cell_space.max(Dim::J) );
    EXPECT_EQ( ghosted_shared_cell_space.min(Dim::K),
               owned_cell_space.max(Dim::K) );
    EXPECT_EQ( ghosted_shared_cell_space.max(Dim::K),
               owned_cell_space.max(Dim::K) + halo_width );

    ghosted_shared_cell_space =
        grid_block->sharedGhostedIndexSpace( Cell(), 1, -1, 0 );
    EXPECT_EQ( ghosted_shared_cell_space.min(Dim::I),
               owned_cell_space.max(Dim::I) );
    EXPECT_EQ( ghosted_shared_cell_space.max(Dim::I),
               owned_cell_space.max(Dim::I) + halo_width );
    EXPECT_EQ( ghosted_shared_cell_space.min(Dim::J),
               0 );
    EXPECT_EQ( ghosted_shared_cell_space.max(Dim::J),
               halo_width );
    EXPECT_EQ( ghosted_shared_cell_space.min(Dim::K),
               owned_cell_space.min(Dim::K) );
    EXPECT_EQ( ghosted_shared_cell_space.max(Dim::K),
               owned_cell_space.max(Dim::K) );

    ghosted_shared_cell_space =
        grid_block->sharedGhostedIndexSpace( Cell(), 0, 1, -1 );
    EXPECT_EQ( ghosted_shared_cell_space.min(Dim::I),
               owned_cell_space.min(Dim::I) );
    EXPECT_EQ( ghosted_shared_cell_space.max(Dim::I),
               owned_cell_space.max(Dim::I) );
    EXPECT_EQ( ghosted_shared_cell_space.min(Dim::J),
               owned_cell_space.max(Dim::J) );
    EXPECT_EQ( ghosted_shared_cell_space.max(Dim::J),
               owned_cell_space.max(Dim::J) + halo_width );
    EXPECT_EQ( ghosted_shared_cell_space.min(Dim::K),
               0 );
    EXPECT_EQ( ghosted_shared_cell_space.max(Dim::K),
               halo_width );

    // Check the nodes we own that we will share with our neighbors. Cover
    // enough of the neighbors that we know the bounds are correct in each
    // dimension. The three variations here cover all of the cases.
    auto owned_shared_node_space =
        grid_block->sharedOwnedIndexSpace( Node(), -1, 0, 1 );
    EXPECT_EQ( owned_shared_node_space.min(Dim::I),
               owned_node_space.min(Dim::I) );
    EXPECT_EQ( owned_shared_node_space.max(Dim::I),
               owned_node_space.min(Dim::I) + halo_width + 1);
    EXPECT_EQ( owned_shared_node_space.min(Dim::J),
               owned_node_space.min(Dim::J) );
    EXPECT_EQ( owned_shared_node_space.max(Dim::J),
               owned_node_space.max(Dim::J) );
    EXPECT_EQ( owned_shared_node_space.min(Dim::K),
               owned_node_space.max(Dim::K) - halo_width );
    EXPECT_EQ( owned_shared_node_space.max(Dim::K),
               owned_node_space.max(Dim::K) );

    owned_shared_node_space =
        grid_block->sharedOwnedIndexSpace( Node(), 1, -1, 0 );
    EXPECT_EQ( owned_shared_node_space.min(Dim::I),
               owned_node_space.max(Dim::I) - halo_width );
    EXPECT_EQ( owned_shared_node_space.max(Dim::I),
               owned_node_space.max(Dim::I) );
    EXPECT_EQ( owned_shared_node_space.min(Dim::J),
               owned_node_space.min(Dim::J) );
    EXPECT_EQ( owned_shared_node_space.max(Dim::J),
               owned_node_space.min(Dim::J) + halo_width + 1);
    EXPECT_EQ( owned_shared_node_space.min(Dim::K),
               owned_node_space.min(Dim::K) );
    EXPECT_EQ( owned_shared_node_space.max(Dim::K),
               owned_node_space.max(Dim::K) );

    owned_shared_node_space =
        grid_block->sharedOwnedIndexSpace( Node(), 0, 1, -1 );
    EXPECT_EQ( owned_shared_node_space.min(Dim::I),
               owned_node_space.min(Dim::I) );
    EXPECT_EQ( owned_shared_node_space.max(Dim::I),
               owned_node_space.max(Dim::I) );
    EXPECT_EQ( owned_shared_node_space.min(Dim::J),
               owned_node_space.max(Dim::J) - halo_width );
    EXPECT_EQ( owned_shared_node_space.max(Dim::J),
               owned_node_space.max(Dim::J) );
    EXPECT_EQ( owned_shared_node_space.min(Dim::K),
               owned_node_space.min(Dim::K) );
    EXPECT_EQ( owned_shared_node_space.max(Dim::K),
               owned_node_space.min(Dim::K) + halo_width + 1 );

    // Check the nodes are ghosts that our neighbors own. Cover enough of the
    // neighbors that we know the bounds are correct in each dimension. The
    // three variations here cover all of the cases.
    auto ghosted_shared_node_space =
        grid_block->sharedGhostedIndexSpace( Node(), -1, 0, 1 );
    EXPECT_EQ( ghosted_shared_node_space.min(Dim::I),
               0 );
    EXPECT_EQ( ghosted_shared_node_space.max(Dim::I),
               halo_width );
    EXPECT_EQ( ghosted_shared_node_space.min(Dim::J),
               owned_node_space.min(Dim::J) );
    EXPECT_EQ( ghosted_shared_node_space.max(Dim::J),
               owned_node_space.max(Dim::J) );
    EXPECT_EQ( ghosted_shared_node_space.min(Dim::K),
               owned_node_space.max(Dim::K) );
    EXPECT_EQ( ghosted_shared_node_space.max(Dim::K),
               owned_node_space.max(Dim::K) + halo_width + 1);

    ghosted_shared_node_space =
        grid_block->sharedGhostedIndexSpace( Node(), 1, -1, 0 );
    EXPECT_EQ( ghosted_shared_node_space.min(Dim::I),
               owned_node_space.max(Dim::I) );
    EXPECT_EQ( ghosted_shared_node_space.max(Dim::I),
               owned_node_space.max(Dim::I) + halo_width + 1 );
    EXPECT_EQ( ghosted_shared_node_space.min(Dim::J),
               0 );
    EXPECT_EQ( ghosted_shared_node_space.max(Dim::J),
               halo_width );
    EXPECT_EQ( ghosted_shared_node_space.min(Dim::K),
               owned_node_space.min(Dim::K) );
    EXPECT_EQ( ghosted_shared_node_space.max(Dim::K),
               owned_node_space.max(Dim::K) );

    ghosted_shared_node_space =
        grid_block->sharedGhostedIndexSpace( Node(), 0, 1, -1 );
    EXPECT_EQ( ghosted_shared_node_space.min(Dim::I),
               owned_node_space.min(Dim::I) );
    EXPECT_EQ( ghosted_shared_node_space.max(Dim::I),
               owned_node_space.max(Dim::I) );
    EXPECT_EQ( ghosted_shared_node_space.min(Dim::J),
               owned_node_space.max(Dim::J) );
    EXPECT_EQ( ghosted_shared_node_space.max(Dim::J),
               owned_node_space.max(Dim::J) + halo_width + 1 );
    EXPECT_EQ( ghosted_shared_node_space.min(Dim::K),
               0 );
    EXPECT_EQ( ghosted_shared_node_space.max(Dim::K),
               halo_width );

    // Get another block without a halo and check the local low corner. Do an
    // exclusive scan of sizes to get the local cell offset.
    auto grid_block_2 = createBlock( global_grid, 0 );
    int i_offset =
        std::accumulate( local_num_cell_i.begin(),
                         local_num_cell_i.begin() + cart_rank[Dim::I],
                         0 );
    int j_offset =
        std::accumulate( local_num_cell_j.begin(),
                         local_num_cell_j.begin() + cart_rank[Dim::J],
                         0 );
    int k_offset =
        std::accumulate( local_num_cell_k.begin(),
                         local_num_cell_k.begin() + cart_rank[Dim::K],
                         0 );

    EXPECT_EQ( grid_block_2->lowCorner(Dim::I),
               i_offset * cell_size + global_low_corner[Dim::I] );
    EXPECT_EQ( grid_block_2->lowCorner(Dim::J),
               j_offset * cell_size + global_low_corner[Dim::J] );
    EXPECT_EQ( grid_block_2->lowCorner(Dim::K),
               k_offset * cell_size + global_low_corner[Dim::K] );

    // Compare ghosted low corner to the low corner of the block without a
    // halo.
    for ( int d = 0; d < 3; ++d )
        EXPECT_EQ( grid_block_2->lowCorner(d) - cell_size * halo_width,
                   grid_block->lowCorner(d) );
}

//---------------------------------------------------------------------------//
void notPeriodicTest()
{
    // Create a different MPI communication on every rank, effectively making
    // it serial.
    int comm_rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &comm_rank );
    MPI_Comm serial_comm;
    MPI_Comm_split( MPI_COMM_WORLD, comm_rank, 0, &serial_comm );

    // Let MPI compute the partitioning for this test.
    UniformDimPartitioner partitioner;

    // Create the global grid.
    double cell_size = 0.23;
    std::vector<int> global_num_cell = { 101, 85, 99 };
    std::vector<bool> is_dim_periodic = {false,false,false};
    std::vector<double> global_low_corner = { 1.2, 3.3, -2.8 };
    std::vector<double> global_high_corner =
        { global_low_corner[0] + cell_size * global_num_cell[0],
          global_low_corner[1] + cell_size * global_num_cell[1],
          global_low_corner[2] + cell_size * global_num_cell[2] };
    auto global_grid = createGlobalGrid( serial_comm,
                                         partitioner,
                                         is_dim_periodic,
                                         global_low_corner,
                                         global_high_corner,
                                         cell_size );
    auto grid_comm = global_grid->comm();

    // Create a  grid block.
    int halo_width = 2;
    auto grid_block = createBlock( global_grid, halo_width );

    // Check sizes
    EXPECT_EQ( grid_block->haloWidth(), halo_width );

    // Get the owned number of cells.
    auto owned_cell_space = grid_block->ownedIndexSpace( Cell() );
    for ( int d = 0; d < 3; ++d )
        EXPECT_EQ( owned_cell_space.extent(d), global_num_cell[d] );

    // Get the ghosted number of cells.
    auto ghosted_cell_space = grid_block->ghostedIndexSpace( Cell() );
    for ( int d = 0; d < 3; ++d )
        EXPECT_EQ( ghosted_cell_space.extent(d), global_num_cell[d] );

    // Get the owned number of nodes.
    auto owned_node_space = grid_block->ownedIndexSpace( Node() );
    for ( int d = 0; d < 3; ++d )
        EXPECT_EQ( owned_node_space.extent(d), global_num_cell[d] + 1);

    // Get the ghosted number of nodes.
    auto ghosted_node_space = grid_block->ghostedIndexSpace( Node() );
    for ( int d = 0; d < 3; ++d )
        EXPECT_EQ( ghosted_node_space.extent(d), global_num_cell[d] + 1);

    // Check the low corner.
    for ( int d = 0; d < 3; ++d )
        EXPECT_EQ( grid_block->lowCorner(d), global_low_corner[d] );

    // Check neighbor ranks and shared spaces.
    for ( int i = -1; i < 2; ++i )
        for ( int j = -1; j < 2; ++j )
            for ( int k = -1; k < 2; ++k )
            {
                if ( i == 0 && j == 0 && k == 0 )
                {
                    std::vector<int> nr = {
                        global_grid->dimBlockId(Dim::I)+ i,
                        global_grid->dimBlockId(Dim::J) + j,
                        global_grid->dimBlockId(Dim::K) + k };
                    int nrank;
                    MPI_Cart_rank( grid_comm, nr.data(), &nrank );
                    EXPECT_EQ( grid_block->neighborRank(i,j,k), nrank );
                }
                else
                {
                    EXPECT_EQ( grid_block->neighborRank(i,j,k), -1 );

                    auto owned_shared_cell_space =
                        grid_block->sharedOwnedIndexSpace( Cell(), i, j, k );
                    EXPECT_EQ( owned_shared_cell_space.size(), 0 );

                    auto ghosted_shared_cell_space =
                        grid_block->sharedGhostedIndexSpace( Cell(), i, j, k );
                    EXPECT_EQ( ghosted_shared_cell_space.size(), 0 );

                    auto owned_shared_node_space =
                        grid_block->sharedOwnedIndexSpace( Node(), i, j, k );
                    EXPECT_EQ( owned_shared_node_space.size(), 0 );

                    auto ghosted_shared_node_space =
                        grid_block->sharedGhostedIndexSpace( Node(), i, j, k );
                    EXPECT_EQ( ghosted_shared_node_space.size(), 0 );
                }
            }

    // Free the serial communicator we made
    MPI_Comm_free( &serial_comm );
}

//---------------------------------------------------------------------------//
// RUN TESTS
//---------------------------------------------------------------------------//
TEST( grid_block, api_test )
{
    periodicTest();
    notPeriodicTest();
}

//---------------------------------------------------------------------------//

} // end namespace Test
