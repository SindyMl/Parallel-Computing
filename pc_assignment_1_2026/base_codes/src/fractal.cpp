#include <iostream>
#include <fstream>
#include <cstdlib>
#include <omp.h>
using namespace std;

#define DIM 768

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

int julia(int x, int y) {
    const float scale = 1.5;
    float jx = scale * (float)(DIM / 2 - x) / (DIM / 2);
    float jy = scale * (float)(DIM / 2 - y) / (DIM / 2);

    cuComplex c(-0.7269, 0.1889);
    cuComplex a(jx, jy);

    for (int i = 0; i < 300; i++) {
        a = a * a + c;
        if (a.magnitude2() > 1000)
            return 0;
    }
    return 1;
}

/* Students should parallelize this */
void kernel_omp(unsigned char* ptr) {
    for (int y = 0; y < DIM; y++) {
        for (int x = 0; x < DIM; x++) {
            int offset = x + y * DIM;

            int juliaValue = julia(x, y);
            ptr[offset * 3 + 0] = 255 * juliaValue; // R
            ptr[offset * 3 + 1] = 0;               // G
            ptr[offset * 3 + 2] = 0;               // B
        }
    }
}

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

/* Save image as PPM */
void save_ppm(const char* filename, unsigned char* data, int width, int height) {
    ofstream file(filename, ios::binary);
    file << "P6\n" << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<char*>(data), width * height * 3);
    file.close();
}

int main(void) {
    unsigned char* image_s = new unsigned char[DIM * DIM * 3];
    unsigned char* image_p = new unsigned char[DIM * DIM * 3];

    double start, finish_s, finish_p;

    /* Serial run */
    start = omp_get_wtime();
    kernel_serial(image_s);
    finish_s = omp_get_wtime() - start;

    /* Parallel run */
    start = omp_get_wtime();
    kernel_omp(image_p);
    finish_p = omp_get_wtime() - start;

    cout << "Elapsed time:\n";
    cout << "Serial time: " << finish_s << endl;
    cout << "Parallel time: " << finish_p << endl;
    cout << "Speedup: " << finish_s / finish_p << endl;

    /* Save result */
    save_ppm("output/fractal_serial.ppm", image_s, DIM, DIM);
    save_ppm("output/fractal_par.ppm", image_p, DIM, DIM);    

    delete[] image_s;
    delete[] image_p;
    return 0;
}