#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <string>
#include <algorithm>
#include <cassert>
#include <iomanip>
using namespace std;

class Matrix {
public:
    int n;
    vector<double> data;

    Matrix(int size) : n(size), data(size * size, 0.0) {}

    double& operator()(int i, int j) { return data[i * n + j]; }
    const double& operator()(int i, int j) const { return data[i * n + j]; }
};

// Naive
void naive_multiply(const Matrix& A, const Matrix& B, Matrix& C) {
    int n = A.n;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            for (int k = 0; k < n; k++)
                C(i, j) += A(i, k) * B(k, j);
}

// Tiled
void tiled_multiply(const Matrix& A, const Matrix& B, Matrix& C, int tile_size) {
    int n = A.n;

    for (int i = 0; i < n; i += tile_size) {
        int imax = min(i + tile_size, n);
        for (int j = 0; j < n; j += tile_size) {
            int jmax = min(j + tile_size, n);
            for (int k = 0; k < n; k += tile_size) {
                int kmax = min(k + tile_size, n);

                for (int it = i; it < imax; it++) {
                    for (int jt = j; jt < jmax; jt++) {
                        double sum = 0;
                        for (int kt = k; kt < kmax; kt++) {
                            sum += A(it, kt) * B(kt, jt);
                        }
                        C(it, jt) += sum;
                    }
                }
            }
        }
    }
}

int main() {
    cout << "MATRIX MULTIPLICATION: TILING SWEEP\n";
    cout << "====================================\n\n";

    vector<int> sizes = {64, 128, 512}; //test larger matrices for larger caches sizes
    vector<int> tile_sizes = {1, 4, 8, 16, 32, 64, 128, 256, 512};

    for (int n : sizes) {
        cout << "\n--- Matrix Size: " << n << "x" << n << " ---\n";
        cout << "Total multiply-add operations: " << (long long)n * n * n << " (SAME for both!)\n";

        Matrix A(n), B(n), C1(n), C2(n);

        // Initialize A and B arbitrary
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++) {
                A(i, j) = i + j;
                B(i, j) = i - j;
            }
        // Initialize C1 and C2 to zeros
        for (int i = 0; i < n * n; i++) C1.data[i] = 0.0;
        for (int i = 0; i < n * n; i++) C2.data[i] = 0.0;

        // Baseline: naive
        auto start = chrono::high_resolution_clock::now();
        naive_multiply(A, B, C1);
        auto end = chrono::high_resolution_clock::now();
        // auto naive_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        auto naive_time_us = chrono::duration_cast<chrono::microseconds>(end - start).count();
        double naive_time_ms = naive_time_us / 1000.0;
        // cout << "Naive: " << naive_time << " ms\n";
        if (naive_time_us < 1000) {
            cout << "Naive: " << naive_time_us << " µs\n";
        } else {
            cout << "Naive: " << fixed << setprecision(2) << naive_time_ms << " ms\n";
        }


        // Try different tile sizes
        for (int t : tile_sizes) {
            if (t > n) continue;

            // Reset C
            for (int i = 0; i < n * n; i++) C2.data[i] = 0.0;

            start = chrono::high_resolution_clock::now();
            tiled_multiply(A, B, C2, t);
            end = chrono::high_resolution_clock::now();
            // auto tiled_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
            auto tiled_time_us = chrono::duration_cast<chrono::microseconds>(end - start).count();
            double tiled_time_ms = tiled_time_us / 1000.0;
            for (int i_test = 0; i_test < n * n; i_test++){
                if (C1.data[i_test] != C2.data[i_test]) {
                    assert(false && "Error: Matrix multiplication results don't match!");
                    exit(1);
                }
            }
            // double speedup = (double)naive_time / tiled_time;
            double speedup = naive_time_us / (double)tiled_time_us;
            string faster = speedup >= 1.0 ? "FASTER" : "SLOWER";

            // cout << "  Tile " << t << ": " << tiled_time << " ms, "
            //      << speedup << "x " << faster << "\n";
            cout << "  Tile " << t << ": ";
            if (tiled_time_us < 1000) {
                cout << tiled_time_us << " µs, ";
            } else {
                cout << fixed << setprecision(2) << tiled_time_ms << " ms, ";
            }
            cout << fixed << setprecision(2) << speedup << "x " << faster << "\n";
        }
    }

    return 0;
}
