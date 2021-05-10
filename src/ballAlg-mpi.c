#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <string.h>
#include <mpi.h>
#include "gen_points.h"
#include "point_operations.h"
#include "ball_tree.h"
#include "macros.h"

//#define DEBUG
//remove the above comment to enable debug messages

int n_dims;                             /* number of dimensions of each point                                               */

double **pts;                           /* list of points of the current iteration of the algorithm                         */
double **ortho_array;                   /* list of ortogonal projections of the points in pts                               */
double **ortho_array_srt;               /* list of ortogonal projections of the point in pts to be sorted.                  */
double **pts_aux;                       /* list of points of the next iteration of the algorithm                            */

double *first_point;                    /* first point in the set i.e. with lower index relative to the initial point set   */

long n_points_local;                    /* number of points in the dataset present at this process                          */
long n_points_global;                   /* number of points in the dataset present at all processes                         */

double  *basub;                         /* point containing b-a for the orthogonal projections                              */
double *ortho_tmp;                      /* temporary pointer used for calculation the orthogonal projection                 */

node_ptr node_list;                     /* list of nodes of the ball tree                                                   */
double** node_centers;                  /* list of centers of the ball tree nodes                                           */

long n_nodes;                           /* number of nodes of the ball tree                                                 */
long node_id;                           /* id of the current node of the algorithm                                          */
long node_counter;                      /* number of nodes generated by the program                                         */

int rank;                               /* rank of the current process                                                      */
int n_procs;                            /* total number of processes                                                        */

long *processes_n_points;               /* array of the number of points owned by each process currently                    */

double **furthest_away_point_buffer;    /* buffer storing the local furthest away point at each process                      */

double *a;                              /* furthest away point from the first point in the globalset                         */
double *b;                              /* furthest away point from a in the global set                                      */
double *furthest_from_center;           /* furthest away point from center in the global set                                 */

double *median_left_point;              /* rightmost point in the global point set that is left of the median                */
double *median_right_point;             /* leftmost point in the global point set that is right of the median                */

/*
Returns the point in the global point set that is furthest away from point p
*/
void mpi_get_furthest_away_point(double *p, double *out) {
    double local_max_distance = 0.0;
    double *local_furthest_point = p;

    /*compute local furthest point from p*/
    for(long i = 0; i < n_points_local; i++){
        double curr_distance = distance(p, pts[i]);
        if(curr_distance > local_max_distance){
            local_max_distance = curr_distance;
            local_furthest_point = pts[i];
        }
    }

    /*get local furthest point from p of all processes*/
    MPI_Allgather(
                local_furthest_point,             /* send local furthest point to all other processes */
                n_dims,                           /* local furthest point has n_dims elements  */
                MPI_DOUBLE,                       /* each element is of type double */
                *furthest_away_point_buffer,      /* store each local furthest_away_point in the buffer */
                n_dims,                           /* each local_furthest_point has n_dims elements  */
                MPI_DOUBLE,                       /* each element is of type double */
                MPI_COMM_WORLD                    /* broadcast is all to all */
    );

    double global_max_distance = 0.0;
    double *global_furthest_point = p;

    /*of those, compute the furthest away from p*/
    for(int i = 0; i < n_procs; i++) {
        double curr_distance = distance(p, furthest_away_point_buffer[i]);
        if(curr_distance > global_max_distance){
            global_max_distance = curr_distance;
            global_furthest_point = furthest_away_point_buffer[i];
        }
    }
    copy_point(global_furthest_point, out);
}

/*
Returns the radius of the ball tree node defined by point center
*/
double mpi_get_radius(double* center) {
    mpi_get_furthest_away_point(center, furthest_from_center);
    return sqrt(distance(furthest_from_center, center));
}

// TODO Clara get the nth point in the global pts
// The owner broadcasts, the rest receives
void mpi_get_point(double **pts, int n, double* out) {

}

/*
Dummy for now
*/
double **distributed_projections_sort() {
    /*TODO parallel distributed sort*/
    return NULL;
}

/*
Returns the median projection of the dataset
by sorting the projections based on their x coordinate
*/
double* mpi_get_center() {
    double **sorted_points = distributed_projections_sort();

    if(n_points_local % 2) { // is odd
        long middle = (n_points_local - 1) / 2;
        mpi_get_point(sorted_points, middle, node_centers[node_counter]);
    }
    else { // is even
        long first_middle = (n_points_local / 2) - 1;
        long second_middle = (n_points_local / 2);
        mpi_get_point(sorted_points, first_middle, median_left_point);
        mpi_get_point(sorted_points, second_middle, median_right_point);
        middle_point(median_left_point, median_right_point, node_centers[node_counter]);
    }
    return node_centers[node_counter];
}

/*
Computes the orthogonal projections of points in pts onto line defined by b-a
*/
void calc_orthogonal_projections(double* a, double* b) {
    sub_points(b, a, basub);
    for(long i = 0; i < n_points_local; i++){
        orthogonal_projection(basub, a, pts[i], ortho_array[i], ortho_tmp);
    }
}

/*
Places each point in pts in partition left or right by comparing the x coordinate
of its orthogonal projection with the x coordinate of the center point
*/
// TODO Bras rewrite, now we do not know how many points will be in left and right
void mpi_fill_partitions(double** left, double** right, double* center) {
    long l = 0;
    long r = 0;
    for(long i = 0; i < n_points_local; i++) {
        if(ortho_array[i][0] < center[0]) {
            left[l] = pts[i];
            l++;
        }
        else {
            right[r] = pts[i];
            r++;
        }
    }
}


/*
Get the number of points currently held by each process
*/
void mpi_get_processes_n_points() {
    long my_points = n_points_local;

    /*Broadcast all-to-all the number of points held locally*/
    MPI_Allgather(
                &my_points,             /*the address of the data the current process is sending*/
                1,                      /*number of data elements sent*/
                MPI_LONG,               /*type of data element sent*/
                processes_n_points,     /*the address where the current process stores the data received*/
                1,                      /*number of data elements sent by each process*/
                MPI_LONG,               /*type of data element received*/
                MPI_COMM_WORLD          /*sending and receiving to all processes*/
    );

#ifdef DEBUG
    for(int i = 0; i < n_procs; i++) {
        printf("%d processes_n_points[%d]=%ld\n", rank, i, processes_n_points[i]);
    }
#endif
}

/*
Returns the first point in the set
i.e. lower index relative to the initial set
*/
double *mpi_get_first_point() {
    int root = 0;
    /*first process that owns at least one point */
    for(int i = 0; i < n_procs; i++) {
        if(processes_n_points[i] != 0) {
            root = i;
            break;
        }
    }

    if(rank==root) {
        /*send*/
        MPI_Bcast(
                pts[0],             /*the address of the data the current process is sending*/
                n_dims,             /*the number of data elements sent*/
                MPI_DOUBLE,         /*type of data elements sent*/
                root,               /*rank of the process sending the data*/
                MPI_COMM_WORLD      /*broadcast to processes*/
        );
        return pts[0];
    }
    else {
        /*receive*/
        MPI_Bcast(
                first_point,        /*the address of the buffer the data received will be stored into*/
                n_dims,             /*the number of data elements received*/
                MPI_DOUBLE,         /*type of data elements received*/
                root,               /*rank of the process sending the data*/
                MPI_COMM_WORLD      /*broadcast to processes*/
        );
        return first_point;
    }
}

void mpi_build_node() {

    mpi_get_processes_n_points();

    double* first_point = mpi_get_first_point();

#ifdef DEBUG
    printf("%d first_point=", rank);
    print_point(first_point);
#endif

    mpi_get_furthest_away_point(first_point, a);

    mpi_get_furthest_away_point(a, b);
#ifdef DEBUG
    printf("%d a=", rank);
    print_point(a);
    printf("%d b=", rank);
    print_point(b);
#endif

    calc_orthogonal_projections(a, b);

    //sort projections

    //calc left and right points


}

void alloc_memory() {
    long n_points_ceil = (long) (ceil((double) (n_points_global) / (double) (n_procs)));

    n_points_local = BLOCK_SIZE(rank, n_procs, n_points_global);
    n_nodes = (n_points_global * 2) - 1;

    pts_aux = (double**) malloc(sizeof(double*) * n_points_ceil);

    ortho_array = create_array_pts(n_dims, n_points_ceil);
    ortho_array_srt = (double**) malloc(sizeof(double*) * n_points_ceil);

    basub = (double*) malloc(sizeof(double) * n_dims);
    ortho_tmp = (double*) malloc(sizeof(double) * n_dims);

    node_list = (node_ptr) malloc(sizeof(node_t) * n_nodes);
    node_centers = create_array_pts(n_dims, n_nodes);
    processes_n_points = (long*) malloc(sizeof(long) * n_procs);

    first_point = (double*) malloc(sizeof(double) * n_dims);

    furthest_away_point_buffer = create_array_pts(n_dims, n_procs);
    a = (double*) malloc(sizeof(double) * n_dims);
    b = (double*) malloc(sizeof(double) * n_dims);
    furthest_from_center = (double*) malloc(sizeof(double) * n_dims);
    median_left_point = (double*) malloc(sizeof(double) * n_dims);
    median_right_point = (double*) malloc(sizeof(double) * n_dims);
}

int main(int argc, char** argv) {
    double exec_time;
    exec_time = -omp_get_wtime();

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &n_procs);

    pts = get_points(argc, argv, &n_dims, &n_points_global);
    alloc_memory();

    mpi_build_node();
    /*
    build_tree();
    */

    exec_time += omp_get_wtime();
    /*
    if (!rank) {
        fprintf(stderr, "%.1lf\n", exec_time);
        printf("%d %ld\n", n_dims, n_nodes);
    }
    dump_tree();
    */
    MPI_Finalize();
}
