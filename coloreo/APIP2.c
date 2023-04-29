#include "APIP2.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __ERROR_CONDICIONAL(cond, pl, s, vfr, vfr2) \
  if (!(cond)) {                                    \
    if (vfr != NULL) free(vfr), vfr = NULL;         \
    if (vfr2 != NULL) free(vfr2), vfr2 = NULL;      \
    fprintf(stderr, "Error en %s: %s\n", pl, s);    \
    return ERROR;                                   \
  }

// ---------------------------------- GREEDY ----------------------------------

static bool EsColoreoPropio(Grafo G, u32* Orden, u32* Color) {
  const u32 n = NumeroDeVertices(G);

  for (u32 indice = 0; indice < n; indice++) {
    const u32 nodo = Orden[indice], grado = Grado(nodo, G),
              colorNodo = Color[nodo];
    for (u32 indiceVec = 0; indiceVec < grado; indiceVec++) {
      const u32 vec = IndiceVecino(indiceVec, nodo, G), colorVec = Color[vec];
      if (colorNodo == colorVec) return 0;
    }
  }
  return 1;
}

// Se asume SZ(Orden) = SZ(Color) = NumeroDeVertices(G) = n
// Por enunciado, NO se verifica
u32 Greedy(Grafo G, u32* Orden, u32* Color) {
  const u32 ERROR = (1LL << 32) - 1, n = NumeroDeVertices(G), d = Delta(G);
  bool *color_usado = calloc(d + 1, sizeof(bool)),
       *vis = calloc(n, sizeof(bool));
  __ERROR_CONDICIONAL((color_usado != NULL), "Greedy", "Error interno",
                      color_usado, vis);
  __ERROR_CONDICIONAL((vis != NULL), "Greedy", "Error interno", color_usado,
                      vis);

  u32 cnt_colores = 1;
  Color[Orden[0]] = 0, vis[Orden[0]] = 1;


  for (u32 indice = 1; indice < n; indice++) {
    const u32 nodo = Orden[indice], grado = Grado(nodo, G);
    vis[nodo] = 1;

    u32 cnt_color_usado = 0;
    memset(color_usado, false, cnt_colores * sizeof(bool));
    
    for (u32 indiceVec = 0; indiceVec < grado; indiceVec++) {
      const u32 vec = IndiceVecino(indiceVec, nodo, G);
      if (!vis[vec]) continue;

      if (!color_usado[Color[vec]])
        cnt_color_usado++, color_usado[Color[vec]] = 1;
    }

    if (cnt_color_usado == cnt_colores)
      Color[nodo] = cnt_colores++;
    else {
      for (u32 color = 0; color < cnt_colores; color++)
        if (!color_usado[color]) {
          Color[nodo] = color;
          break;
        }
    }
  }

  free(color_usado), free(vis);
  color_usado = vis = NULL;
  __ERROR_CONDICIONAL((cnt_colores <= d + 1), "Greedy", "Error interno",
                      color_usado, vis);
  __ERROR_CONDICIONAL((EsColoreoPropio(G, Orden, Color)), "Greedy",
                      "El coloreo NO es propio", color_usado, vis);
  return cnt_colores;
}

// ----------------------------- ORDEN DE COLOREO -----------------------------

bool OrdenJediCMP(const u32 a, const u32 b, const u32* F) {
  if (F[a] == F[b]) return a <= b;
  return F[a] > F[b];
}

void MergeSortJedi(u32* Orden, u32* Color, u32* F, const u32 izq,
                   const u32 der) {
  if (izq < der) {
    u32 med = (izq + der) / 2;
    MergeSortJedi(Orden, Color, F, izq, med);
    MergeSortJedi(Orden, Color, F, med + 1, der);

    u32* aux = calloc(der - izq + 1, sizeof(u32));
    u32 i = izq, j = med + 1, nw_aux = 0;
    while (i <= med && j <= der)
      aux[nw_aux++] = OrdenJediCMP(Color[Orden[i]], Color[Orden[j]], F)
                          ? Orden[i++]
                          : Orden[j++];
    while (i <= med) aux[nw_aux++] = Orden[i++];
    while (j <= der) aux[nw_aux++] = Orden[j++];

    for (u32 w = 0; w < nw_aux; w++) Orden[w + izq] = aux[w];

    free(aux);
    aux = NULL;
  }
}

/**
 * DETALLE:
 * Dado un grafo G no dirigido, un array Orden (permutacion de los indices) y un
 * array Color (colores de los nodos de modo que es coloreo propio de G):
 *    Ordenar Orden de forma no decreciente respecto a la siguiente comparacion:
 *      sean a, b dos elementos de Orden, a < b si y solo si:
 *          1) F[Color[a]] > F[Color[b]]
 *          o
 *          2) F[Color[a]] == F[Color[b]] && Color[a]<Color[b]
 *      donde F es una funcion de colores -> entero dada por:
 *          F[x] = x * (sumatoria_{i tal que Color[i]==x} Grado(i, G))
 *
 * Se asume que Color y Orden apuntan a una region de memoria de no menos de n
 * elementos
 * Si todo anduvo bien, devuelve char 0. Sino, 1
 */
char OrdenJedi(Grafo G, u32* Orden, u32* Color) {
  const char ERROR = '1';
  u32 n = NumeroDeVertices(G), r = 0;

  u32* F = calloc(n, sizeof(u32));
  __ERROR_CONDICIONAL((F != NULL), "Orden Jedi", "Error interno", F, F);
  for (u32 indice = 0; indice < n; indice++) {
    __ERROR_CONDICIONAL((Color[indice] < n), "Orden Jedi",
                        "Error con argumento Color", F, F);
    F[Color[indice]] += Grado(Orden[indice], G);
    if (r < Color[indice]) r = Color[indice];
  }
  for (u32 color = 0; color < r; color++) F[color] *= color;

  MergeSortJedi(Orden, Color, F, 0, n - 1);

  free(F);
  F = NULL;

  return '0';
}

static int cmp_asc(const void *a, const void *b) {
    return (*(u32 *)a - *(u32 *)b);
}

static u32* EliminaIguales(u32* arr, u32* tam) {
  u32 j = 0;

  for (u32 i = 1; i < *tam; i++){
    if (arr[i] != arr[j]) {
      arr[++j] = arr[i];
    }
  }

  *tam = j + 1;
  arr = realloc(arr, (*tam) * sizeof(u32));
  
  return arr;
}

char OrdenImparPar(u32 n, u32* Orden, u32* Color) {
  const char ERROR = '1';

  // Tamaño de los arreglos de pares e impares
  u32 tamPares = 0;
  u32 tamImpares = 0;

  for (u32 i = 0; i < n; i++) {
    if (Color[i] % 2 == 0) {
      tamPares++;
    } else {
      tamImpares++;
    }
  }

  // Creo los arreglos de pares e impares
  u32* pares = calloc(tamPares, sizeof(u32));
  __ERROR_CONDICIONAL((pares != NULL), "OrdenImparPar", "Error Interno", pares, pares);
  u32* impares = calloc(tamImpares, sizeof(u32));
  __ERROR_CONDICIONAL((impares != NULL), "OrdenImparPar", "Error Interno", impares, impares);

  // Lleno los arreglos de pares e impares
  u32 j = 0; u32 k = 0;
  for (u32 i = 0; i < n; i++) {
    if (Color[i] % 2 == 0) {
      pares[j] = Color[i];
      j++;
    } else {
      impares[k] = Color[i];
      k++;
    }
  }

  // Ordeno los arreglos
  qsort(pares, tamPares, sizeof(unsigned int), cmp_asc);
  qsort(impares, tamImpares, sizeof(unsigned int), cmp_asc);

  // Elimino los elementos repetidos de ambos arreglos
  pares = EliminaIguales(pares, &tamPares);
  __ERROR_CONDICIONAL((pares != NULL), "EliminaIguales", "Error Interno", pares, pares);
  impares = EliminaIguales(impares, &tamImpares);
  __ERROR_CONDICIONAL((pares != NULL), "OrdenImparPar", "Error Interno", pares, pares);

  // Creo un arreglo para guardar los colores finales ordenados
  k = 0;

  // Quiero ordenar los índices en el array Orden en base a su color
  for (int i = tamImpares-1; i >= 0; i--){
    for (u32 j = 0; j < n; j++) {
      if (Color[j] == impares[i]) {
        Orden[k] = j;
        k++;
      }
    }

  }

  for (int i = tamPares-1; i >= 0; i--){
    for (u32 j = 0; j < n; j++) {
      if (Color[j] == pares[i]) {
        Orden[k] = j;
        k++;
      }
    }
  }

  free(pares);
  free(impares);

  return 0;
}


int main(void){
  int coloreo1[500], coloreo2[500];

  Grafo G = NULL;
  G = ConstruirGrafo();
  u32 n = NumeroDeVertices(G);
  u32 *Orden = calloc(n, sizeof(u32)), *Color = calloc(n, sizeof(u32));

  // Corremos Greedy en orden natural.
  for (u32 i = 0; i < n; i++) Orden[i] = i;
  u32 cant_ord_natural = Greedy(G, Orden, Color);
  printf("Cantidad de colores con Orden Natural: %d\n", cant_ord_natural);

  int counter = 1;

  u32 cant_colores1, cant_colores2;
  for (int i = 0; i < 500; i++) {
    if (counter<=16){
      // Correr OrdenImparPar y luego Greedy con ese orden
      OrdenImparPar(n, Orden, Color);
      // Guardar el resultado en el array coloreo1
      cant_colores1 = Greedy(G, Orden, Color);
      coloreo1[i] = cant_colores1;

      // Correr OrdenJedi y luego Greedy con ese orden
      OrdenJedi(G, Orden, Color);
      // Guardar el resultado en el array coloreo2
      cant_colores2 = Greedy(G, Orden, Color);
      coloreo2[i] = cant_colores2;
      counter++;
    } else {
      // Correr OrdenImparPar y luego Greedy con ese orden
      OrdenJedi(G, Orden, Color);
      // Guardar el resultado en el array coloreo1
      cant_colores1 = Greedy(G, Orden, Color);
      coloreo1[i] = cant_colores1;
      // Correr OrdenJedi y luego Greedy con ese orden
      OrdenImparPar(n, Orden, Color);
      // Guardar el resultado en el array coloreo2
      cant_colores2 = Greedy(G, Orden, Color);
      coloreo2[i] = cant_colores2;
      counter++;
      if (counter == 33) counter=1;
    }
  }

  // Finalmente, puedes imprimir los resultados obtenidos
  printf("Resultados de los coloreos:\n");
  printf("COLOREO 1\n");
  for (int i = 0; i < 500; i++) {
    printf("%u ", coloreo1[i]);
  }
  printf("\n");
  printf("COLOREO 2\n");
  for (int i = 0; i < 500; i++) {
    printf("%u ", coloreo2[i]);
  }
  printf("\n");

  free(Orden); free(Color); DestruirGrafo(G);

  return 0;
}