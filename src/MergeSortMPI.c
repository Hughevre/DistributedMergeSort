#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "mpi.h"
#include "ErrorCodes.h"

#define ASSERT(x)\
if(!(x))\
{\
    printf("[ERROR] Assertion failed\n");\
    printf("on line %d\n", __LINE__);\
    printf("in file %s\n", __FILE__);\
    return;\
}

/**
 * @param x - A number that is meant to be checked
 * 
 * @brief This function checks if passed number is
 *        a power of 2. If it is, function returnes
 *        true. Otherwise, returnes false.
*/
bool is_power_of_2(int x) {
    return (x > 0 && !(x & (x - 1)));
}

/**
 * @param p1 - A pointer to the first element under comparison
 * @param p2 - A pointer to the second element under comparison
 * 
 * @brief This function checks if element p1 is bigger than
 *        element p2. If it so, function returnes non-zero 
 *        positive value. If elements p1 and p2 are equal, the
 *        function returnes 0. Finally, if element p2 is bigger
 *        than element p1, function returnes negative values.
*/
int ascending_comparator(const void* p1, const void* p2) {
    return (*(int*)p1 - *(int*)p2);
}

/**
 * @param numbers_array      - An empty array of integers that is meant to
 *                             be filled with random values
 * @param numbers_array_size - Size of the numbers_array
 * @param max_value          - Maximum value that array should store
 * @param min_value          - Minimum value that arary should store
 * 
 * @brief This function fills an array with random values drawned within a 
 *        range of min_value to max_value using STD rand() function.
*/
void fill_numbers_array(int* numbers_array, size_t numbers_array_size, int max_value, int min_value) {
    srand(time(0));
    for (size_t i = 0; i < numbers_array_size; ++i) {
        numbers_array[i] = rand() % (max_value + 1 - min_value) + min_value;
    }
}

/**
 * @param first_half_array  - First of the two arrays that are meant to be merged
 * @param second_half_array - Second of the two arrays that are meant to be merged
 * @param merged_array      - An array that stores merged results
 * @param size              - Size of each sub-arrays. Each array has to be of equal size
 * 
 * @brief This function merges to arrays of integers into one signle array by copying 
 *        values from one array to another.
*/
void merge(int first_half_array[], int second_half_array[], int merged_array[], int size){
    int ai, bi, ci;
    ai = bi = ci = 0;

    // Integers remain in both arrays
    while ((ai < size) && (bi < size)){
        if (first_half_array[ai] <= second_half_array[bi]){
            merged_array[ci] = first_half_array[ai];
            ++ai;
        } else {
            merged_array[ci] = second_half_array[bi];
            ++bi;
        }
            ++ci;
    }

    // Integers only remain in second_half_array
    if (ai >= size){
        while (bi < size) {
            merged_array[ci] = second_half_array[bi];
            ++bi; 
            ++ci;
        }
    }

    // Integers only remain in first_half_array
    if (bi >= size){
        while (ai < size) {
            merged_array[ci] = first_half_array[ai];
            ++ai; 
            ++ci;
        }
    }
}

/**
 * @param numbers_array      - An array of integers that is meant to printed
 * @param numbers_array_size - Size of the numbers_array
 * 
 * @brief This function prints an array of inetgers to the screen separating each
 *        number by single ' ' sign.
*/
void print_numbers_array(int* numbers_array, size_t numbers_array_size) {
    for (size_t i = 0; i < numbers_array_size; ++i) {
        printf("%d ", numbers_array[i]);
    }

    printf("\n");
}

int* merge_sort(unsigned int total_merge_tree_height,
                int          process_id,
                int          local_numbers_array[],
                size_t       local_numbers_array_size,
                MPI_Comm     comm,
                int          global_numbers_array[]) {
    unsigned int local_tree_height;
    int          parent_id;
    int          right_child_id;
    int*         left_leaf;
    int*         right_leaf;
    int*         merged_results_array;

    local_tree_height = 0;
    qsort(local_numbers_array, local_numbers_array_size, sizeof(int), ascending_comparator);
    left_leaf = local_numbers_array;

    while (local_tree_height < total_merge_tree_height) {
        parent_id = (process_id & (~(1 << local_tree_height)));

        if (parent_id == process_id) {
            right_child_id = (process_id | (1 << local_tree_height));

            // Reserve some space for array from the (child) right leaf
            right_leaf = malloc(local_numbers_array_size * sizeof(int));
            MPI_Recv(right_leaf, local_numbers_array_size, MPI_INT, right_child_id, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            merged_results_array = malloc(local_numbers_array_size * 2 * sizeof(int));
            merge(left_leaf, right_leaf, merged_results_array, local_numbers_array_size);

            left_leaf = merged_results_array;
            local_numbers_array_size *= 2;

            free(right_leaf);
            merged_results_array = NULL;

            ++local_tree_height;
        } else {
            // Send local numbers array to parent (left leaf)
            MPI_Send(left_leaf, local_numbers_array_size, MPI_INT, parent_id, 0, MPI_COMM_WORLD);
            if(local_tree_height != 0) {
                free(left_leaf);
            }
            local_tree_height = total_merge_tree_height;
        }
    }

    if (process_id == 0) {
        global_numbers_array = left_leaf;
    }

    return global_numbers_array;
}

/**
 * @param arr_1    - Poitner to the first array used in the comparison
 * @param arr_2    - Pointer to the second array used in the comparison
 * @param arr_size - Size of the two arrays
 * 
 * @brief This functions calls ASSERT for each element of the arr_1.
*/
void compare_numbers_array(int* arr_1, int* arr_2, size_t arr_size) {
    for (size_t i = 0; i < arr_size; ++i) {
        ASSERT(arr_1[i] == arr_2[i])
    }
    printf("---[SUCCESS]---\nCheck ended successfully\n");
}

int main(int argc, char** argv) {
    int       comm_group_size;
    int       process_id;
    char      host_name[MPI_MAX_PROCESSOR_NAME];
    int       host_name_length;
    const int array_max_value = 100000;
    const int array_min_value = 0;

    MPI_Init(&argc, &argv);

    // Determines the size of the group associated with a communicator
    MPI_Comm_size(MPI_COMM_WORLD, &comm_group_size);

    // Determines the rank of the calling process in the communicator
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);

    // Gets the name of the processor
    host_name_length = -1;
    MPI_Get_processor_name(host_name, &host_name_length);

    // Checks for odd number of processes
    if (is_power_of_2(comm_group_size) == false) {
        printf("[ERROR, PID = %d] - Communication group size is odd. Should be even\n", process_id);
        MPI_Finalize();
        return COMM_GROUP_ODD_SIZE;
    }

    if (argc != 2) {
        printf("[ERROR, PID = %d] - Incorrect number of program arguments. Only one argument is accepted\n", process_id);
        return ARGC_INCORRECT_VALUE;
    }

    // Allocates some space for global array of numbers to be sorted, shared among all processes
    size_t global_numbers_array_size;
    int    *global_numbers_array;
    int    *global_numbers_array_copy;
    size_t local_numbers_array_size;
    int    *local_numbers_array;

    if (process_id == 0) {
        global_numbers_array_size = atoi(argv[1]);
        global_numbers_array      = calloc(global_numbers_array_size, sizeof(int));
        global_numbers_array_copy = calloc(global_numbers_array_size, sizeof(int));
        fill_numbers_array(global_numbers_array, global_numbers_array_size, array_max_value, array_min_value);
        memcpy(global_numbers_array_copy, global_numbers_array, sizeof(int)*global_numbers_array_size);
#ifdef DEBUG
        print_numbers_array(global_numbers_array, global_numbers_array_size);
#endif
    }

    MPI_Bcast(&global_numbers_array_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Allocates some space for local array, private for each process
    local_numbers_array_size = global_numbers_array_size / comm_group_size;
    local_numbers_array      = calloc(local_numbers_array_size, sizeof(int));

    MPI_Scatter(global_numbers_array,
                local_numbers_array_size,
                MPI_INT,
                local_numbers_array,
                local_numbers_array_size,
                MPI_INT,
                0,
                MPI_COMM_WORLD);

    // Calculate total height of tree
    double merge_tree_height;
    merge_tree_height = log2(comm_group_size);

    // Starts measuring a time of execution
    double start_time;
    double end_time;

    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    // Main thread of the programm
    if (process_id == 0) {
        global_numbers_array = merge_sort(merge_tree_height, process_id, local_numbers_array, local_numbers_array_size, MPI_COMM_WORLD, global_numbers_array);
    } else {
        merge_sort(merge_tree_height, process_id, local_numbers_array, local_numbers_array_size, MPI_COMM_WORLD, NULL);
    }

    // Stops measuring the time
    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();

    // Makes few final tocuhes
    if (process_id == 0) {
        printf("Sorting the array of size %ld took %f in seconds\n", global_numbers_array_size, end_time - start_time);

        // Checks if algorithm works correctly
        qsort(global_numbers_array_copy, global_numbers_array_size, sizeof(int), ascending_comparator);
        compare_numbers_array(global_numbers_array, global_numbers_array_copy, global_numbers_array_size);
#ifdef DEBUG
        print_numbers_array(global_numbers_array_copy, global_numbers_array_size);
#endif
        free(global_numbers_array_copy);
        free(global_numbers_array);
    }

    free(local_numbers_array);
    MPI_Finalize();

    return 0;
}