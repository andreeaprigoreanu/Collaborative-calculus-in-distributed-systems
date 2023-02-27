#include "mpi.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_COORDINATORS 4
#define TOPOLOGY_TEXT_LEN 300
#define RESULT_LEN 500000
#define SIZE_TAG 1
#define DATA_TAG 2
#define SEND_COORD_RANK_TAG 3
#define SEND_N_TAG 4
#define SEND_VECTOR_TAG 5
#define SEND_INT_TAG 6

// function used by a coordinator to read processes from their cluster
void read_file(int rank, int* clusters_sizes, int** topology) {
    // get filename
    char file_name[15];
    sprintf(file_name, "cluster%d.txt", rank);

    // open file
    FILE *fin = fopen(file_name, "r");

    // read number of workers
    fscanf(fin, "%d", &clusters_sizes[rank]);

    // read workers' ranks
    topology[rank] = calloc(clusters_sizes[rank], sizeof(int));
    for (int i = 0; i < clusters_sizes[rank]; i++) {
        fscanf(fin, "%d", &topology[rank][i]);
    }

    // close file
    fclose(fin);
}

// function used by process with given rank to print topology
void print_topology(int rank, int* clusters_sizes, int** topology) {
    char topology_text[TOPOLOGY_TEXT_LEN];
    sprintf(topology_text, "%d -> ", rank);

    for (int i = 0; i < NUM_COORDINATORS; i++) {
        sprintf(topology_text + strlen(topology_text), "%d:", i);
        for (int j = 0; j < clusters_sizes[i]; j++) {
            if (j == clusters_sizes[i] - 1) {
                if (i == NUM_COORDINATORS - 1) {
                    sprintf(topology_text + strlen(topology_text), "%d", topology[i][j]);
                } else {
                    sprintf(topology_text + strlen(topology_text), "%d ", topology[i][j]);
                }
            } else {
                sprintf(topology_text + strlen(topology_text), "%d,", topology[i][j]);
            }
        }
    }

    printf("%s\n", topology_text);
}

// function that prints log message
void print_log_message(int src, int dst) {
    printf("M(%d,%d)\n", src, dst);
}

// function used by a coordinator (with rank src) to send info about a cluster with given rank
// to a fellow coordinator (with rank dst)
void send_data_to_coordinator(int rank, int src, int dst, int* clusters_sizes, int** topology) {
    // send workers number
    MPI_Send(&clusters_sizes[rank], 1, MPI_INT, dst, SIZE_TAG, MPI_COMM_WORLD);
    print_log_message(src, dst);

    // send workers
    MPI_Send(topology[rank], clusters_sizes[rank], MPI_INT, dst, DATA_TAG, MPI_COMM_WORLD);
    print_log_message(src, dst);
}

// function used by a coordinator (with rank src) to receive info about a cluster with given rank
// to a fellow coordinator (with rank dst)
void recv_data_from_coordinator(int rank, int src, int dst, int* clusters_sizes, int** topology) {
    // receive workers number
    MPI_Recv(&clusters_sizes[rank], 1, MPI_INT, src, SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // receive workers
    topology[rank] = calloc(clusters_sizes[rank], sizeof(int));
    MPI_Recv(topology[rank], clusters_sizes[rank], MPI_INT, src, DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

// function used by coordinators to populate their topology matrix when there's no error
void send_topology_to_coordinators_no_error(int rank, int* clusters_sizes, int** topology) {
    if (rank == 0) {
        send_data_to_coordinator(0, 0, 1, clusters_sizes, topology);
        send_data_to_coordinator(0, 0, 3, clusters_sizes, topology);

        recv_data_from_coordinator(1, 1, 0, clusters_sizes, topology);

        recv_data_from_coordinator(3, 3, 0, clusters_sizes, topology);

        send_data_to_coordinator(3, 0, 1, clusters_sizes, topology);

        recv_data_from_coordinator(2, 3, 0, clusters_sizes, topology);
    }

    if (rank == 1) {
        send_data_to_coordinator(1,1, 0, clusters_sizes, topology);
        send_data_to_coordinator(1, 1, 2, clusters_sizes, topology);

        recv_data_from_coordinator(0, 0, 1, clusters_sizes, topology);

        recv_data_from_coordinator(2, 2, 1, clusters_sizes, topology);

        send_data_to_coordinator(0, 1, 2, clusters_sizes, topology);

        recv_data_from_coordinator(3, 0, 1, clusters_sizes, topology);
    }

    if (rank == 2) {
        send_data_to_coordinator(2, 2, 1, clusters_sizes, topology);
        send_data_to_coordinator(2, 2, 3, clusters_sizes, topology);

        recv_data_from_coordinator(1, 1, 2, clusters_sizes, topology);

        recv_data_from_coordinator(3, 3, 2, clusters_sizes, topology);

        send_data_to_coordinator(1, 2, 3, clusters_sizes, topology);

        recv_data_from_coordinator(0, 1, 2, clusters_sizes, topology);
    }

    if (rank == 3) {
        send_data_to_coordinator(3, 3, 0, clusters_sizes, topology);
        send_data_to_coordinator(3, 3, 2, clusters_sizes, topology);

        recv_data_from_coordinator(0, 0, 3, clusters_sizes, topology);

        recv_data_from_coordinator(2, 2, 3, clusters_sizes, topology);

        send_data_to_coordinator(2, 3, 0, clusters_sizes, topology);

        recv_data_from_coordinator(1, 2, 3, clusters_sizes, topology);
    }
}

// function used by coordinators to populate their topology matrix when
// there's no communication between 0 and 1
void send_topology_to_coordinators_error(int rank, int* clusters_sizes, int** topology) {
    if (rank == 0) {
        send_data_to_coordinator(0, 0, 3, clusters_sizes, topology);

        recv_data_from_coordinator(3, 3, 0, clusters_sizes, topology);
        recv_data_from_coordinator(2, 3, 0, clusters_sizes, topology);
        recv_data_from_coordinator(1, 3, 0, clusters_sizes, topology);
    }

    if (rank == 1) {
        send_data_to_coordinator(1, 1, 2, clusters_sizes, topology);

        recv_data_from_coordinator(2, 2, 1, clusters_sizes, topology);
        recv_data_from_coordinator(3, 2, 1, clusters_sizes, topology);
        recv_data_from_coordinator(0, 2, 1, clusters_sizes, topology);
    }

    if (rank == 2) {
        send_data_to_coordinator(2, 2, 1, clusters_sizes, topology);
        send_data_to_coordinator(2, 2, 3, clusters_sizes, topology);

        recv_data_from_coordinator(1, 1, 2, clusters_sizes, topology);
        recv_data_from_coordinator(3, 3, 2, clusters_sizes, topology);

        send_data_to_coordinator(1, 2, 3, clusters_sizes, topology);

        recv_data_from_coordinator(0, 3, 2, clusters_sizes, topology);

        send_data_to_coordinator(3, 2, 1, clusters_sizes, topology);
        send_data_to_coordinator(0, 2, 1, clusters_sizes, topology);
    }

    if (rank == 3) {
        send_data_to_coordinator(3, 3, 0, clusters_sizes, topology);
        send_data_to_coordinator(3, 3, 2, clusters_sizes, topology);

        recv_data_from_coordinator(0, 0, 3, clusters_sizes, topology);
        recv_data_from_coordinator(2, 2, 3, clusters_sizes, topology);

        send_data_to_coordinator(0, 3, 2, clusters_sizes, topology);

        recv_data_from_coordinator(1, 2, 3, clusters_sizes, topology);

        send_data_to_coordinator(2, 3, 0, clusters_sizes, topology);
        send_data_to_coordinator(1, 3, 0, clusters_sizes, topology);
    }
}

// function used by a coordinator to send topology to their workers
void send_topology_to_workers(int rank, int* clusters_sizes, int** topology) {
    for (int i = 0; i < clusters_sizes[rank]; i++) {
        // send coordinator rank
        MPI_Send(&rank, 1, MPI_INT, topology[rank][i], SEND_COORD_RANK_TAG, MPI_COMM_WORLD);
        print_log_message(rank, topology[rank][i]);

        // send topology
        for (int j = 0; j < NUM_COORDINATORS; j++) {
            MPI_Send(&clusters_sizes[j], 1, MPI_INT, topology[rank][i], SIZE_TAG, MPI_COMM_WORLD);
            print_log_message(rank, topology[rank][i]);

            MPI_Send(topology[j], clusters_sizes[j], MPI_INT, topology[rank][i], DATA_TAG, MPI_COMM_WORLD);
            print_log_message(rank, topology[rank][i]);
        }
    }
}

// function used by a worker to receive topology to their coordinator
void recv_topology(int* coord_rank, int* clusters_sizes, int** topology) {
    // receive coordinator rank
    MPI_Recv(coord_rank, 1, MPI_INT, MPI_ANY_SOURCE, SEND_COORD_RANK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // receive topology
    for (int i = 0; i < NUM_COORDINATORS; i++) {
        MPI_Recv(&clusters_sizes[i], 1, MPI_INT, *coord_rank, SIZE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        topology[i] = calloc(clusters_sizes[i], sizeof(int));
        MPI_Recv(topology[i], clusters_sizes[i], MPI_INT, *(coord_rank), DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

// function used by coordinator 0 to initialise the vector
int* initialize_vector(int n) {
    int *v = calloc(n, sizeof(int));
    for (int i = 0; i < n; i++) {
        v[i] = n - i - 1;
    }
    return v;
}

// function used by coordinator 0 to print processed vector
void print_vector(int n, int *v) {
    char message[RESULT_LEN];
    sprintf(message, "Rezultat: ");

    for (int i = 0; i < n - 1; i++) {
        sprintf(message + strlen(message), "%d ", v[i]);
    }
    sprintf(message + strlen(message), "%d\n", v[n - 1]);

    printf("%s", message);
}

// function that computes the total number of workers in topology
int get_total_workers (int *clusters_sizes) {
    int total_workers = 0;
    for (int i = 0; i < NUM_COORDINATORS; i++) {
        total_workers += clusters_sizes[i];
    }
    return total_workers;
}

// function that computes the average number of elements a workers
// has to process
int get_work(int n, int total_workers) {
    int work = n / total_workers;
    return work;
}

// function used by a coordinator to send vector to next coordinator
void send_vector(int *n, int *v, int src, int dst) {
    // send number of elements (n)
    MPI_Send(n, 1, MPI_INT, dst, SEND_N_TAG, MPI_COMM_WORLD);
    print_log_message(src, dst);

    // send vector
    MPI_Send(v, *n, MPI_INT, dst, SEND_VECTOR_TAG, MPI_COMM_WORLD);
    print_log_message(src, dst);
}

// function used by a coordinator to receive vector from previous coordinator
int *recv_vector(int *n, int src, int dst) {
    // receive n
    MPI_Recv(n, 1, MPI_INT, src, SEND_N_TAG, MPI_COMM_WORLD, NULL);

    // receive vector
    int *v = calloc(*n, sizeof(int));
    MPI_Recv(v, *n, MPI_INT, src, SEND_VECTOR_TAG, MPI_COMM_WORLD, NULL);

    return v;
}

// function used by a process to send an int value
void send_int_value(int *value, int src, int dst) {
    MPI_Send(value, 1, MPI_INT, dst, SEND_INT_TAG, MPI_COMM_WORLD);
    print_log_message(src, dst);
}

// function used by a process to send an int value
void recv_int_value(int *value, int src, int dst) {
    MPI_Recv(value, 1, MPI_INT, src, SEND_INT_TAG, MPI_COMM_WORLD, NULL);
}

// function used by coordinators to pass processed vector to each other
// until it reaches process 0
void send_processed_vector(int n, int *v, int src, int dst) {
    MPI_Send(v, n, MPI_INT, dst, SEND_VECTOR_TAG, MPI_COMM_WORLD);
    print_log_message(src, dst);
}

// function used by coordinators to receive processed vector to neighbour
// until it reaches process 0
void recv_processed_vector(int n, int *v, int src, int dst) {
    MPI_Recv(v, n, MPI_INT, src, SEND_VECTOR_TAG, MPI_COMM_WORLD, NULL);
}

// function used by a coordinator to send to a worker the fraction of the vector
// it has to process
void send_vector_to_worker(int start_pos, int end_pos, int *v, int src, int dst) {
    // send number of elements
    int n = end_pos - start_pos + 1;
    MPI_Send(&n, 1, MPI_INT, dst, SEND_N_TAG, MPI_COMM_WORLD);
    print_log_message(src, dst);

    // send vector
    MPI_Send(v + start_pos, n, MPI_INT, dst, SEND_VECTOR_TAG, MPI_COMM_WORLD);
    print_log_message(src, dst);
}

// function used by a worker to receive from the coordinator the
// fraction of the vector it has to process
int *recv_vector_from_coord(int *task_dim, int coord_rank) {
    // receive number of elements
    MPI_Recv(task_dim, 1, MPI_INT, coord_rank, SEND_N_TAG, MPI_COMM_WORLD, NULL);

    // receive fraction of vector
    int *v = calloc(*task_dim, sizeof(int));
    MPI_Recv(v, *task_dim, MPI_INT, coord_rank, SEND_VECTOR_TAG, MPI_COMM_WORLD, NULL);

    return v;
}

// function used by a worker to send to the coordinator the
// fraction of the vector it processed
void send_vector_to_coord(int task_dim, int *v, int src, int coord_rank) {
    MPI_Send(v, task_dim, MPI_INT, coord_rank, SEND_VECTOR_TAG, MPI_COMM_WORLD);
    print_log_message(src, coord_rank);
}

// function used by a coordinator to receive from a worker the fraction of the vector
// it processed
void recv_vector_from_worker(int start_pos, int end_pos, int *v, int src, int dst) {
    MPI_Recv(v + start_pos, end_pos - start_pos + 1, MPI_INT, src, SEND_VECTOR_TAG, MPI_COMM_WORLD, NULL);
}

// function used by a worker to process the vector
void do_work(int task_dim, int *v) {
    for (int i = 0; i < task_dim; i++) {
        v[i] *= 5;
    }
}

// function used by coordinator to send tasks to workers
void send_tasks_to_workers(int rank_coord, int first_pos, int work, int n, int *v,
                           int* clusters_sizes, int** topology) {
    for (int i = 0; i < clusters_sizes[rank_coord]; i++) {
        // get start_pos
        int start_pos = first_pos + i * work;
        // get end_pos
        int end_pos = start_pos + work - 1;
        if (rank_coord == 1 && i == clusters_sizes[rank_coord] - 1) {
            end_pos = n - 1;
        }

        // send portion to worker
        send_vector_to_worker(start_pos, end_pos, v, rank_coord, topology[rank_coord][i]);
    }
}

// function used by coordinator to receive the processed fractions of the vector
// from workers
void recv_tasks_from_workers(int rank_coord, int first_pos, int work, int n, int *v,
                           int* clusters_sizes, int** topology) {
    for (int i = 0; i < clusters_sizes[rank_coord]; i++) {
        // get start_pos
        int start_pos = first_pos + i * work;
        // get end_pos
        int end_pos = start_pos + work - 1;
        if (rank_coord == 1 && i == clusters_sizes[rank_coord] - 1) {
            end_pos = n - 1;
        }

        // receive processed vector
        recv_vector_from_worker(start_pos, end_pos, v, topology[rank_coord][i], rank_coord);
    }
}

int main (int argc, char *argv[])
{
    int rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // check number of arguments
    if (argc < 3) {
        fprintf(stderr, "Please use: mpirun -np <numar_procese> ./tema3 <dimensiune_vector> <eroare_comunicatie>\n");
        MPI_Finalize();
        return 0;
    }

    // check if topology has error
    int no_error = atoi(argv[1]);

    // allocate memory for topology
    int coord_rank;
    int* clusters_sizes = calloc(NUM_COORDINATORS, sizeof(int));
    int** topology = calloc(NUM_COORDINATORS, sizeof(int *));
    int n;
    int *v;

    // part 1: populate topology matrix
    // part 3: changed exchange of messages in case 0 and 1 cannot communicate
    if (rank < NUM_COORDINATORS) {
        coord_rank = rank;
        read_file(rank, clusters_sizes, topology);
        if (no_error == 0) {
            send_topology_to_coordinators_no_error(rank, clusters_sizes, topology);
        } else {
            send_topology_to_coordinators_error(rank, clusters_sizes, topology);
        }
        send_topology_to_workers(rank, clusters_sizes, topology);

        print_topology(rank, clusters_sizes, topology);
    } else {
        recv_topology(&coord_rank, clusters_sizes, topology);
        print_topology(rank, clusters_sizes, topology);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // part 2: compute final vector
    int first_pos = 0;
    // start computing vector result
    if (rank == 0) {
        // get n
        n = atoi(argv[1]);
        // initialise vector
        v = initialize_vector(n);

        // send work to workers
        int total_workers = get_total_workers(clusters_sizes);
        int work = get_work(n, total_workers);
        send_tasks_to_workers(rank, first_pos, work, n, v, clusters_sizes, topology);
        recv_tasks_from_workers(rank, first_pos, work, n, v, clusters_sizes, topology);

        // send coordinator 3 the position from which it has to start
        // processing the vector
        first_pos += clusters_sizes[rank] * work;
        send_int_value(&first_pos, 0, 3);
        // send vector to coordinator 3
        send_vector(&n, v, 0, 3);

        // receive processed vector from coordinator 3
        recv_processed_vector(n, v, 3, 0);
        print_vector(n, v);
    } else {
        if (rank == 3) {
            // receive vector from coordinator 0
            v = recv_vector(&n, 0, 3);

            // send work to workers
            recv_int_value(&first_pos, 0, 3);
            int total_workers = get_total_workers(clusters_sizes);
            int work = get_work(n, total_workers);
            send_tasks_to_workers(rank, first_pos, work, n, v, clusters_sizes, topology);
            recv_tasks_from_workers(rank, first_pos, work, n, v, clusters_sizes, topology);

            // send coordinator 2 the position from which it has to start
            // processing the vector
            first_pos += clusters_sizes[rank] * work;
            send_int_value(&first_pos, 3, 2);
            // send vector to coordinator 2
            send_vector(&n, v, 3, 2);

            // receive processed vector from coordinator 2
            recv_processed_vector(n, v, 2, 3);
            // send processed vector to coordinator 0
            send_processed_vector(n, v, 3, 0);
        } else {
            if (rank == 2) {
                // receive vector from coordinator 3
                v = recv_vector(&n, 3, 2);

                // send work to workers
                recv_int_value(&first_pos, 3, 2);
                int total_workers = get_total_workers(clusters_sizes);
                int work = get_work(n, total_workers);
                send_tasks_to_workers(rank, first_pos, work, n, v, clusters_sizes, topology);
                recv_tasks_from_workers(rank, first_pos, work, n, v, clusters_sizes, topology);

                // send coordinator 1 the position from which it has to start
                // processing the vector
                first_pos += clusters_sizes[rank] * work;
                send_int_value(&first_pos, 2, 1);
                send_vector(&n, v, 2, 1);

                // receive processed vector from coordinator 1
                recv_processed_vector(n, v, 1, 0);
                // send processed vector to coordinator 3
                send_processed_vector(n, v, 2, 3);
            } else {
                if (rank == 1) {
                    // receive vector from coordinator 2
                    v = recv_vector(&n, 2, 1);

                    // send work to workers
                    recv_int_value(&first_pos, 2, 1);
                    int total_workers = get_total_workers(clusters_sizes);
                    int work = get_work(n, total_workers);
                    send_tasks_to_workers(rank, first_pos, work, n, v, clusters_sizes, topology);
                    recv_tasks_from_workers(rank, first_pos, work, n, v, clusters_sizes, topology);

                    // send processed vector to coordinator 2
                    send_processed_vector(n, v, 1, 2);
                } else {
                    // worker process
                    // receive tasks from coordinator
                    int task_dim;
                    v = recv_vector_from_coord(&task_dim, coord_rank);

                    do_work(task_dim, v);

                    // send processed vector to coordinator
                    send_vector_to_coord(task_dim, v, rank, coord_rank);
                }
            }
        }
    }

    MPI_Finalize();
    return 0;
}
