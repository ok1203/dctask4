#include <mpi.h>
#include <vector>
#include <cmath>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <queue>

using namespace std;

const int REQUEST_TAG = 100;
const int TASK_TAG = 101;
const int EXIT_TAG = 102;
const int RESULT_TAG = 103;
int resultg = 0;

void server(int rank, int size, int num_of_tasks, char** argv) {
    queue<int> tasks;

    // Init tasks
    for (int i = 1; i < num_of_tasks; i++) {
        tasks.push(stoi(argv[i]));
    }
    tasks.push(0);
    int result = 0;
    double start_time = MPI_Wtime();
    // Distribute tasks
    int remaining_workers = size - 1;
    while (remaining_workers > 0) {
        MPI_Status stat;
        // Wait for incoming message from a worker
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        const int source_rank = stat.MPI_SOURCE;
        const int tag = stat.MPI_TAG;
        if (tag == REQUEST_TAG) { // message is a task request
            // Receive task request
            MPI_Recv(nullptr, 0, MPI_INT, source_rank, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (!tasks.empty()) {
                // We have a task - send it
                MPI_Send(&tasks.front(), 1, MPI_INT, source_rank, TASK_TAG, MPI_COMM_WORLD);
                tasks.pop();
            }
            else {
                // There are no more tasks - tell worker to exit
                MPI_Send(nullptr, 0, MPI_INT, source_rank, EXIT_TAG, MPI_COMM_WORLD);
                remaining_workers--;
            }
        } 
	if (tag == RESULT_TAG) {
	    
            MPI_Recv(&result, 1, MPI_INT, source_rank, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	    resultg += result;
	}
    }
    MPI_Status stat;
        // Wait for incoming message from a worker
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        const int source_rank = stat.MPI_SOURCE;
        const int tag = stat.MPI_TAG;
    if (tag == RESULT_TAG) {
	    
            MPI_Recv(&result, 1, MPI_INT, source_rank, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	    resultg += result;
	}
    double end_time = MPI_Wtime();
    printf("result is: %d\n", resultg);
    printf("Server is done, time = %.5f\n", end_time - start_time);
}

bool isPrime(int n)
{
    // Corner case
    if (n <= 1)
        return false;

    // Check from 2 to n-1
    for (int i = 2; i < n; i++)
        if (n % i == 0)
            return false;

    return true;
}

int exec_task(int worker_rank, int task) {
    int c = 0;
    for (int i = 2; i <= task; i++)
        if (isPrime(i))
            c++;
    printf("rank: %d\n", worker_rank);
    return c;
}

void worker(int rank, int size, int server_rank) {
    bool working = true;
    int result = 0;
    while (working) {
        // Send a request to the server
        MPI_Send(nullptr, 0, MPI_INT, server_rank, REQUEST_TAG, MPI_COMM_WORLD);
        // Wait for a response
        MPI_Status stat;
        MPI_Probe(server_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        const int tag = stat.MPI_TAG;
        if (tag == TASK_TAG) { // response is a task
            int task;
            MPI_Recv(&task, 1, MPI_INT, server_rank, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // Executing the task
            resultg += exec_task(rank, task);
        }
        else if (tag == EXIT_TAG) { // response is a signal to exit
            MPI_Recv(nullptr, 0, MPI_INT, server_rank, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            working = false;
            printf("resultofthisworker: %d\n", resultg);
	}
        else {
	    
            printf("Unknown tag: %d\n", tag);
        }
    }
    MPI_Send(&resultg, 1, MPI_INT, server_rank, RESULT_TAG, MPI_COMM_WORLD);
    
}

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int num_of_tasks = (argc > 1 ? argc : 1000);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int SERVER_RANK = 0;

    if (rank == SERVER_RANK) {
        // This is a server process
        server(rank, size, num_of_tasks, argv);
    }
    else {
        // This is a worker process
        worker(rank, size, SERVER_RANK);
    }
    
    
    MPI_Finalize();
    return 0;
}