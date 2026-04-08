#include <iostream>
#include <fstream>
#include <cstdlib>
#include <omp.h>
using namespace std;

#define DIM 768

// ─────────────────────────────────────────────
// Complex number helper struct
// ─────────────────────────────────────────────
struct cuComplex {
    float r;
    float i;
    cuComplex(float a, float b) : r(a), i(b) {}
    float magnitude2(void) { return r * r + i * i; }
    cuComplex operator*(const cuComplex& a) {
        return cuComplex(r * a.r - i * a.i, i * a.r + r * a.i);
    }
    cuComplex operator+(const cuComplex& a) {
        return cuComplex(r + a.r, i + a.i);
    }
};

// ─────────────────────────────────────────────
// Julia set evaluation for a single pixel
// Returns 1 if (x,y) is in the set, 0 otherwise
// ─────────────────────────────────────────────
int julia(int x, int y) {
    const float scale = 1.5;
    float jx = scale * (float)(DIM / 2 - x) / (DIM / 2);
    float jy = scale * (float)(DIM / 2 - y) / (DIM / 2);

    cuComplex c(-0.7269, 0.1889);   // constant c from assignment spec
    cuComplex a(jx, jy);

    for (int i = 0; i < 300; i++) {
        a = a * a + c;
        if (a.magnitude2() > 1000)
            return 0;
    }
    return 1;
}

// ─────────────────────────────────────────────
// Serial baseline kernel
// ─────────────────────────────────────────────
void kernel_serial(unsigned char* ptr) {
    for (int y = 0; y < DIM; y++) {
        for (int x = 0; x < DIM; x++) {
            int offset = x + y * DIM;
            int juliaValue = julia(x, y);
            ptr[offset * 3 + 0] = 255 * juliaValue;
            ptr[offset * 3 + 1] = 0;
            ptr[offset * 3 + 2] = 0;
        }
    }
}

// ─────────────────────────────────────────────
// (a) 1D Rowwise Parallel  [INTERLEAVED]
//
// With T threads, thread id handles rows:
//   id, id+T, id+2T, ...
// e.g. T=4: Thread 0 -> rows 0,4,8,...
//           Thread 1 -> rows 1,5,9,...
//           Thread 2 -> rows 2,6,10,...
//           Thread 3 -> rows 3,7,11,...
// ─────────────────────────────────────────────
void kernel_1d_rowwise(unsigned char* ptr) {
    #pragma omp parallel
    {
        int id = omp_get_thread_num();
        int T  = omp_get_num_threads();
        for (int y = id; y < DIM; y += T) {
            for (int x = 0; x < DIM; x++) {
                int offset = x + y * DIM;
                int juliaValue = julia(x, y);
                ptr[offset * 3 + 0] = 255 * juliaValue;
                ptr[offset * 3 + 1] = 0;
                ptr[offset * 3 + 2] = 0;
            }
        }
    }
}

// ─────────────────────────────────────────────
// (b) 1D Columnwise Parallel  [INTERLEAVED]
//
// With T threads, thread id handles columns:
//   id, id+T, id+2T, ...
// e.g. T=4: Thread 0 -> cols 0,4,8,...
//           Thread 1 -> cols 1,5,9,...
// ─────────────────────────────────────────────
void kernel_1d_colwise(unsigned char* ptr) {
    #pragma omp parallel
    {
        int id = omp_get_thread_num();
        int T  = omp_get_num_threads();
        for (int x = id; x < DIM; x += T) {
            for (int y = 0; y < DIM; y++) {
                int offset = x + y * DIM;
                int juliaValue = julia(x, y);
                ptr[offset * 3 + 0] = 255 * juliaValue;
                ptr[offset * 3 + 1] = 0;
                ptr[offset * 3 + 2] = 0;
            }
        }
    }
}

// ─────────────────────────────────────────────
// (c) 2D Row-Block Parallel  [CONTIGUOUS BLOCK]
//
// DIM rows split into T contiguous blocks.
// Thread id gets rows [ystart, yend).
// Last thread takes any leftover rows.
// ─────────────────────────────────────────────
void kernel_2d_rowblock(unsigned char* ptr) {
    #pragma omp parallel
    {
        int id             = omp_get_thread_num();
        int T              = omp_get_num_threads();
        int rows_per_thread = DIM / T;
        int ystart         = id * rows_per_thread;
        int yend           = (id == T - 1) ? DIM : ystart + rows_per_thread;

        for (int y = ystart; y < yend; y++) {
            for (int x = 0; x < DIM; x++) {
                int offset = x + y * DIM;
                int juliaValue = julia(x, y);
                ptr[offset * 3 + 0] = 255 * juliaValue;
                ptr[offset * 3 + 1] = 0;
                ptr[offset * 3 + 2] = 0;
            }
        }
    }
}

// ─────────────────────────────────────────────
// (d) 2D Column-Block Parallel  [CONTIGUOUS BLOCK]
//
// DIM columns split into T contiguous blocks.
// Thread id gets columns [xstart, xend).
// Last thread takes any leftover columns.
// ─────────────────────────────────────────────
void kernel_2d_colblock(unsigned char* ptr) {
    #pragma omp parallel
    {
        int id             = omp_get_thread_num();
        int T              = omp_get_num_threads();
        int cols_per_thread = DIM / T;
        int xstart         = id * cols_per_thread;
        int xend           = (id == T - 1) ? DIM : xstart + cols_per_thread;

        for (int y = 0; y < DIM; y++) {
            for (int x = xstart; x < xend; x++) {
                int offset = x + y * DIM;
                int juliaValue = julia(x, y);
                ptr[offset * 3 + 0] = 255 * juliaValue;
                ptr[offset * 3 + 1] = 0;
                ptr[offset * 3 + 2] = 0;
            }
        }
    }
}

// ─────────────────────────────────────────────
// (e) OpenMP for construct
//
// Let OpenMP handle decomposition automatically
// using static scheduling on the outer (row) loop.
// ─────────────────────────────────────────────
void kernel_omp_for(unsigned char* ptr) {
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < DIM; y++) {
        for (int x = 0; x < DIM; x++) {
            int offset = x + y * DIM;
            int juliaValue = julia(x, y);
            ptr[offset * 3 + 0] = 255 * juliaValue;
            ptr[offset * 3 + 1] = 0;
            ptr[offset * 3 + 2] = 0;
        }
    }
}

// ─────────────────────────────────────────────
// Save buffer as PPM image file
// ─────────────────────────────────────────────
void save_ppm(const char* filename, unsigned char* data, int width, int height) {
    ofstream file(filename, ios::binary);
    file << "P6\n" << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<char*>(data), width * height * 3);
    file.close();
}

// ─────────────────────────────────────────────
// Main: benchmark all methods across thread counts
// ─────────────────────────────────────────────
int main(void) {
    int thread_counts[] = {1, 2, 4, 6, 8, 10, 12, 14, 16};
    int num_tests = 9;

    unsigned char* img_serial = new unsigned char[DIM * DIM * 3];
    unsigned char* img_par    = new unsigned char[DIM * DIM * 3];

    // ── Serial baseline ──────────────────────
    double t_start  = omp_get_wtime();
    kernel_serial(img_serial);
    double t_serial = omp_get_wtime() - t_start;
    save_ppm("output/fractal_serial.ppm", img_serial, DIM, DIM);

    cout << "=== Julia Set Fractal Benchmark (DIM=" << DIM << ") ===" << endl;
    cout << "Serial time: " << t_serial << " s" << endl << endl;

    // ── Table header ─────────────────────────
    cout << "Threads\t1D_Row_t\t1D_Col_t\t2D_RowBlk_t\t2D_ColBlk_t\tOMP_For_t\t"
         << "1D_Row_S\t1D_Col_S\t2D_RowBlk_S\t2D_ColBlk_S\tOMP_For_S" << endl;

    for (int ti = 0; ti < num_tests; ti++) {
        int T = thread_counts[ti];
        omp_set_num_threads(T);

        double times[5];

        t_start = omp_get_wtime(); kernel_1d_rowwise(img_par);  times[0] = omp_get_wtime() - t_start;
        t_start = omp_get_wtime(); kernel_1d_colwise(img_par);  times[1] = omp_get_wtime() - t_start;
        t_start = omp_get_wtime(); kernel_2d_rowblock(img_par); times[2] = omp_get_wtime() - t_start;
        t_start = omp_get_wtime(); kernel_2d_colblock(img_par); times[3] = omp_get_wtime() - t_start;
        t_start = omp_get_wtime(); kernel_omp_for(img_par);     times[4] = omp_get_wtime() - t_start;

        cout << T;
        for (int m = 0; m < 5; m++) cout << "\t" << times[m];
        for (int m = 0; m < 5; m++) cout << "\t" << t_serial / times[m];
        cout << endl;
    }

    // Save parallel output for visual correctness check
    omp_set_num_threads(4);
    kernel_omp_for(img_par);
    save_ppm("output/fractal_par.ppm", img_par, DIM, DIM);

    cout << endl << "Images saved to output/" << endl;

    delete[] img_serial;
    delete[] img_par;
    return 0;
}
