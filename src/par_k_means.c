#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#define NUM_POINTS 10000000 // Número total de pontos
#define K 50                // Número de clusters (centróides)
#define MAX_ITER 100        // Número máximo de iterações permitidas

// Estrutura que representa um ponto 2D e o cluster ao qual pertence
typedef struct {
    double x, y;
    int cluster;
} Point;

// Estrutura que representa um centróide em 2D
typedef struct {
    double x, y;
} Centroid;

// Função para calcular a distância euclidiana ao quadrado
double distance_sq(Point p, Centroid c) {
    double dx = p.x - c.x;
    double dy = p.y - c.y;
    return dx * dx + dy * dy;
}

int main(int argc, char *argv[]) {
    // Define número de threads via OpenMP
    int num_threads = (argc > 1) ? atoi(argv[1]) : omp_get_max_threads();
    omp_set_num_threads(num_threads);

    // Aloca o vetor de pontos
    Point *points = malloc(NUM_POINTS * sizeof(Point));
    if (points == NULL) {
        fprintf(stderr, "Erro ao alocar memória para os pontos.\n");
        return 1;
    }

    Centroid centroids[K];

    // Inicializa semente base para rand_r
    unsigned int seed_base = (unsigned int)time(NULL);

    // Cada thread terá sua própria semente, derivada da semente base
    #pragma omp parallel
    {
        unsigned int seed = seed_base + omp_get_thread_num();

        // Paraleliza a geração aleatória dos pontos no intervalo [0, 100] para x e y
        // Considera a seed local de cada thread para isso
        #pragma omp for
        for (int i = 0; i < NUM_POINTS; i++) {
            points[i].x = rand_r(&seed) / (double)RAND_MAX * 100.0;
            points[i].y = rand_r(&seed) / (double)RAND_MAX * 100.0;
            points[i].cluster = -1;
        }

        // Inicializa os centróides escolhendo aleatoriamente pontos gerados
        // Garante que apenas uma thread execute essa parte
        // Isso *parece* ser válido, já que o número de centróides é pequeno e ocorre apenas uma vez
        #pragma omp single
        {
            unsigned int cent_seed = seed_base; // Semente base para esse laço
            for (int i = 0; i < K; i++) {
                int index = rand_r(&cent_seed) % NUM_POINTS;
                centroids[i].x = points[index].x;
                centroids[i].y = points[index].y;
            }
        }
    }
    
    int iterations    = 0;
    int changed       = 1;
    double start_time = omp_get_wtime();
    
    // Loop principal do algoritmo k-means
    while (changed && iterations < MAX_ITER) {
        changed = 0;
        
        // Atribuição de pontos ao centróide mais próximo
        for (int i = 0; i < NUM_POINTS; i++) {
            double minDist = distance_sq(points[i], centroids[0]);
            int bestCluster = 0;

            // Sinaliza o uso de SIMD para essa parte, otimizando o loop no compilador
            #pragma omp simd
            for (int j = 1; j < K; j++) {
                double d2 = distance_sq(points[i], centroids[j]);
                if (d2 < minDist) {
                    minDist = d2;
                    bestCluster = j;
                }
            }

            if (points[i].cluster != bestCluster) {
                points[i].cluster = bestCluster;
                changed = 1;
            }
        }
        
        // Atualização dos centróides
        double sumX[K] = {0};
        double sumY[K] = {0};
        int count[K]   = {0};

        // Paraleliza a soma dos pontos por centróide
        // Utiliza "atomic" para evitar condições de corrida
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < NUM_POINTS; i++) {
            int cl = points[i].cluster;
            #pragma omp atomic
            sumX[cl] += points[i].x;
            #pragma omp atomic
            sumY[cl] += points[i].y;
            #pragma omp atomic
            count[cl]++;
        }

        // Paraleliza a atualização dos centróides
        // Pouco ganho de desempenho (são poucos centróides e operações simples)
        #pragma omp parallel for
        for (int j = 0; j < K; j++) {
            if (count[j] != 0) {
                centroids[j].x = sumX[j] / count[j];
                centroids[j].y = sumY[j] / count[j];
            }
        }
        
        iterations++;
    }
    
    double end_time = omp_get_wtime();

    // Exibe resultados
    printf("K-means convergiu em %d iterações com %d threads.\n", iterations, num_threads);
    for (int j = 0; j < K; j++) {
        printf("Centróide %2d: (%.4f, %.4f)\n", j, centroids[j].x, centroids[j].y);
    }
    printf("Tempo total: %.4f seg\n", end_time - start_time);
    
    free(points);
    return 0;
}
