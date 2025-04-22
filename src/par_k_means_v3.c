#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#define DEFAULT_NUM_POINTS 10000000 // Número total de pontos
#define K 50                        // Número de centróides
#define MAX_ITER 150                // Número máximo de iterações

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

static double run(int num_points, int num_threads) {
    // Define número de threads via OpenMP
    omp_set_num_threads(num_threads);

    // Aloca o vetor de pontos
    Point *points = malloc(num_points * sizeof(Point));
    if (points == NULL) {
        fprintf(stderr, "Erro ao alocar memória para os pontos.\n");
        return 1;
    }

    // Cria o vetor de centróides
    Centroid centroids[K];

    // Inicializa semente base para rand_r
    unsigned int seed_base = (unsigned int)time(NULL);

    // Inicializa variáveis de controle
    int iterations    = 0;
    int changed       = 1;
    double start_time, end_time;

    // Cada thread terá sua própria semente, derivada da semente base
    #pragma omp parallel
    {
        unsigned int seed = seed_base + omp_get_thread_num();

        // Paraleliza a geração aleatória dos pontos no intervalo [0, 100] para x e y
        // Considera a seed local de cada thread para isso
        #pragma omp for
        for (int i = 0; i < num_points; i++) {

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
                int index = rand_r(&cent_seed) % num_points;
                centroids[i].x = points[index].x;
                centroids[i].y = points[index].y;
            }
        }
    }

    // Busca o tempo de início da execução
    start_time = omp_get_wtime();

    // Loop principal do algoritmo k-means
    while (changed && iterations < MAX_ITER) {
        changed = 0;
        
        // Paraleliza a atribuição de pontos ao centróide mais próximo
        #pragma omp parallel for schedule(static) reduction(max:changed)
        for (int i = 0; i < num_points; i++) {
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
        // Cada thread calcula a soma localmente e depois atualiza a soma global
        #pragma omp parallel
        {
          double localSumX[K]  = {0};
          double localSumY[K]  = {0};
          int    localCount[K] = {0};
        
          #pragma omp for
          for (int i = 0; i < num_points; i++) {
            int cl = points[i].cluster;
            localSumX[cl] += points[i].x;
            localSumY[cl] += points[i].y;
            localCount[cl]++;
          }
        
          #pragma omp critical
          for (int j = 0; j < K; j++) {
            sumX[j]  += localSumX[j];
            sumY[j]  += localSumY[j];
            count[j] += localCount[j];
          }
        }
        
        // Pode-se paralelizar a atualização dos centróides
        // Porém, pouco ganho de desempenho (são poucos centróides e operações simples)
        // O custo de sincronização pode ser maior que o ganho
        // #pragma omp parallel for
        for (int j = 0; j < K; j++) {
            if (count[j] != 0) {
                centroids[j].x = sumX[j] / count[j];
                centroids[j].y = sumY[j] / count[j];
            }
        }
        
        iterations++;
    }

    end_time = omp_get_wtime();
    free(points);
    return end_time - start_time;
}

// Teste de escalabilidade forte: problema fixo, varia threads
static void test_strong(int base_points) {
    printf("\n--- Teste de Escalabilidade Forte (N=%d) ---\n", base_points);

    // Loop para aumentar o número de threads
    int max_threads = omp_get_max_threads();
    for (int t = 1; t <= max_threads; t *= 2) {
        double t_exec = run(base_points, t);
        printf("Threads: %2d, Tempo: %.4f seg\n", t, t_exec);
    }
}

// Teste de escalabilidade fraca: aumenta N proporcional a threads
static void test_weak(int base_points) {
    printf("\n--- Teste de Escalabilidade Fraca (inicial N=%d) ---\n", base_points);

    // Loop para aumentar o número de pontos proporcionalmente ao número de threads
    int max_threads = omp_get_max_threads();
    for (int t = 1; t <= max_threads; t *= 2) {
        int n_pts = base_points * t;
        double t_exec = run(n_pts, t);
        printf("Threads: %2d, N=%d, Tempo: %.4f seg\n", t, n_pts, t_exec);
    }
}

int main(int argc, char *argv[]) {
    int num_threads = (argc > 1) ? atoi(argv[1]) : omp_get_max_threads();
    omp_set_num_threads(num_threads);
    int mode = (argc > 2) ? atoi(argv[2]) : 0; // 0=normal, 1=forte, 2=fraca

    // Realiza os testes considerando o tipo de escalabilidade informado
    if (mode == 1) {
        test_strong(DEFAULT_NUM_POINTS);
    } else if (mode == 2) {
        test_weak(DEFAULT_NUM_POINTS);
    } else {
        double t = run(DEFAULT_NUM_POINTS, num_threads);
        printf("\nExecução normal: threads=%d, Tempo=%.4f seg\n", num_threads, t);
    }
    return 0;
}
