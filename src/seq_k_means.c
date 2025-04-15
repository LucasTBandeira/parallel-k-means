#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#define NUM_POINTS 1000000 // Número total de pontos
#define K 25               // Número de clusters (centróides)
#define MAX_ITER 100       // Número máximo de iterações permitidas

// Estrutura que representa um ponto 2D e o cluster ao qual pertence.
typedef struct {
    double x;
    double y;
    int cluster;
} Point;

// Estrutura que representa um centróide em 2D.
typedef struct {
    double x;
    double y;
} Centroid;

// Função para calcular a distância euclidiana entre um ponto e um centróide.
double distance(Point p, Centroid c) {
    double dx = p.x - c.x;
    double dy = p.y - c.y;
    return sqrt(dx * dx + dy * dy);
}

int main(int argc, char **argv) {
    // Aloca o vetor de pontos
    Point *points = malloc(NUM_POINTS * sizeof(Point));
    if (points == NULL) {
        printf("Erro ao alocar memória para os pontos.\n");
        return 1;
    }

    
    Centroid centroids[K];

    // Inicializa a semente para números aleatórios
    srand(time(NULL));
    double itime, ftime, exec_time;

    // Utiliza o valor passado por linha de comando para setar o número de threads
    int num_threads = atoi(argv[1]);
    // Configura o número de threads para OpenMP
    omp_set_num_threads(num_threads);

    // Geração aleatória dos pontos (intervalo [0, 100] para x e y)
    for (int i = 0; i < NUM_POINTS; i++) {
        points[i].x = (double)rand() / RAND_MAX * 100.0;
        points[i].y = (double)rand() / RAND_MAX * 100.0;
        points[i].cluster = -1;  // Inicialmente, o ponto não pertence a nenhum cluster
    }
    
    // Inicializa os centróides escolhendo aleatoriamente pontos gerados
    for (int i = 0; i < K; i++) {
        int index = rand() % NUM_POINTS;
        centroids[i].x = points[index].x;
        centroids[i].y = points[index].y;
    }
    
    int iterations = 0;
    int changed = 1;  // Flag que indica se houve alteração nas atribuições dos pontos
    
    // Abre o arquivo de log
    FILE *logFile = fopen("../logs/seq-execution.log", "a");
    if (logFile == NULL) {
        printf("Erro ao abrir o arquivo de log.\n");
        free(points);
        return 1;
    }
    
    // Loga um cabeçalho com o timestamp da execução
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    fprintf(logFile, "===========================\n");
    fprintf(logFile, "Início da execução: %s", timestamp);
    
    // Medição do tempo total de execução
    itime = omp_get_wtime();
    // Loop principal do algoritmo k-means
    while (changed && iterations < MAX_ITER) {
        // Registra o início da iteração
        clock_t iter_start = clock();
        
        changed = 0;
        
        // Atribuição dos pontos para o centróide mais próximo
        #pragma omp parallel for
        for (int i = 0; i < NUM_POINTS; i++) {
            double minDist = distance(points[i], centroids[0]);
            int bestCluster = 0;
            for (int j = 1; j < K; j++) {
                double d = distance(points[i], centroids[j]);
                if (d < minDist) {
                    minDist = d;
                    bestCluster = j;
                }
            }
            if (points[i].cluster != bestCluster) {
                points[i].cluster = bestCluster;
                #pragma omp atomic write
                    changed = 1;
            }
        }
        
        // Atualização dos centróides
        double sumX[K] = {0};
        double sumY[K] = {0};
        int count[K] = {0};  // Contador de pontos para cada cluster nesta iteração
        #pragma omp parallel for
        for (int i = 0; i < NUM_POINTS; i++) {
            int cl = points[i].cluster;
            sumX[cl] += points[i].x;
            sumY[cl] += points[i].y;
            count[cl]++;
        }

        for (int j = 0; j < K; j++) {
            if (count[j] != 0) {
                centroids[j].x = sumX[j] / count[j];
                centroids[j].y = sumY[j] / count[j];
            }
        }
        
        // Registra o fim da iteração e o tempo de duração
        iterations++;
    }
    
    // Registra o fim do tempo total de execução
    ftime = omp_get_wtime();
    exec_time = ftime - itime;

    
    // Exibe os resultados finais no console
    printf("K-means convergiu em %d iterações.\n", iterations);
    for (int j = 0; j < K; j++) {
        // Recalcula a contagem final para cada cluster
        int final_count = 0;
        for (int i = 0; i < NUM_POINTS; i++) {
            if (points[i].cluster == j) final_count++;
        }
        printf("Centróide %d: (%.4f, %.4f) com %d pontos.\n", j, centroids[j].x, centroids[j].y, final_count);
    }
    printf("Tempo total de execução: %.4f segundos\n", exec_time);
    
    // Loga o resumo final da execução
    fprintf(logFile, "Resumo final:\n");
    fprintf(logFile, "K-means convergiu em %d iterações.\n", iterations);
    for (int j = 0; j < K; j++) {
        int final_count = 0;
        for (int i = 0; i < NUM_POINTS; i++) {
            if (points[i].cluster == j) final_count++;
        }
        fprintf(logFile, "   Cluster %d: Centróide (%.4f, %.4f), Pontos: %d\n",
                j, centroids[j].x, centroids[j].y, final_count);
    }
    fprintf(logFile, "\n");
    
    fclose(logFile);
        
    free(points);
    return 0;
}
