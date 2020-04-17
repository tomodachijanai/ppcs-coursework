/**********************************
 *	Yaroslav Fedoriachenko, IV-71
 *	lab4 MPI
 *	A = sort(Z) *MO + min(D)*R*(MZ*MR)
 **********************************/

#include <mpi.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <ctime>

const int N = 128;
const int P = 8;
const int H = N/P;
const int indexes[] = {2,5,8,10,12,15,18,20};
const int edges[] = {
	0,1,
	0,4,
	1,2,
	1,5,
	2,3,
	2,6,
	3,7,
	4,5,
	5,6,
	6,7
	};
const int reorder = 0;
MPI_Comm graph_comm;
int tid = 0;
int world_size = 1;
int *MZ, *MO;
int *A, *B, *Z, *S;
int *e, d;
int *AH;

int main(int argc, char* argv[]) {
	MZ = (int *) malloc(sizeof(int)*N*N);
	MO = (int *) malloc(sizeof(int)*N*N);
	Z = (int *) malloc(sizeof(int)*N);
	B = (int *) malloc(sizeof(int)*N);
	S = (int *) malloc(sizeof(int)*N);
	AH = (int *) malloc(sizeof(int)*H);
	d = 0;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Graph_create(MPI_COMM_WORLD, P, indexes, edges, reorder,  &graph_comm); 
	MPI_Comm_rank(graph_comm, &tid);
	clock_t start;
	if (tid == 0)
	{
		start = clock();
	}
	printf("Thread %d started\n", tid);
	fflush(stdout);
	// 1. Якщо tid = 0, ввести MZ, D, передати MZ та DH іншим процесам.
    // 2. Якщо tid = 4, ввести MR, R, передати MRH та R іншим процесам.
    // 3. Якщо tid = 7, ввести MO, Z, передати MOH та ZH іншим процесам.
	switch (tid)
	{
		case 0:
		{
			A = (int *) malloc(sizeof(int)*N);
			for (int i = 0; i < N; i++)
				*(Z + i) = 1;
			break;
		}

		case 5:
		{
			
			for (int i = 0; i < N; i++)
			{
				for (int j = 0; j < N; j++)
				{
					*(MZ + i*N + j) = 1;
				}
				*(B + i) = 1;
			}
			break;
		}

		case 7:
		{
			for (int i = 0; i < N; i++)
				for (int j = 0; j < N; j++)
					*(MO + i*N + j) = 1;
			d = 1;
			break;
		}
		default:
			break;
	}
    // 4. Якщо tid != 0, отримати ZH передані з процесу з tid = 0.
	MPI_Scatter(Z, H, MPI_INT, Z+tid*H, H, MPI_INT, 0, graph_comm);

    // 5. Якщо tid != 5, отримати B, MZH передані з процесу з tid = 5, скопіювати B у S.
	MPI_Scatter(MZ, H*N, MPI_INT, MZ+tid*H*N, H*N, MPI_INT, 5, graph_comm);
	MPI_Bcast(B, N, MPI_INT, 5, graph_comm);
	for (int i = 0; i < N; i++)
		*(S + i) = *(B + i);
	
	// 6. Якщо tid != 7, отримати MO, d передані з процесу з tid = 7.
	MPI_Bcast(MO, N*N, MPI_INT, 7, graph_comm);
	MPI_Bcast(&d, 1, MPI_INT, 7, graph_comm);
    
	
	// 7. Обчислення ei=min(ZH).
	e = std::min_element(Z+tid*H, Z+(tid+1)*H);
    // 8. Обчислення e=min(e, ei) (MPI_Allreduce).
	int ei;
	MPI_Allreduce(e, &ei, 1, MPI_INT, MPI_MIN, graph_comm);
    // 9. Обчислення SH=sort(SH).
	std::qsort(S + H*tid, H, sizeof(int), 
		[](const void* a, const void* b) {
		int arg1 = *static_cast<const int*>(a);
        int arg2 = *static_cast<const int*>(b);
 
        if(arg1 < arg2) return -1;
        if(arg1 > arg2) return 1;
        return 0;
	});
    // 10. Якщо tid mod 2 = 1, передача SH процесу з target_tid = tid - 1.
    // 11. Якщо tid mod 2 = 0, отримання SH з процесу з source_tid = tid + 1, обчислення S2H=merge(SH, SH).
	if (tid%2) 
	{
		MPI_Send(S + tid*H, H, MPI_INT, tid-1, tid, graph_comm);
	}
	else
	{
		MPI_Recv(S + (tid+1)*H, H, MPI_INT, tid+1, tid+1, graph_comm, MPI_STATUS_IGNORE);
		std::inplace_merge(	S + tid*H, 	 S + (tid+1)*H, 
							S + (tid+2)*H);
	}
    // 12. Якщо tid mod 4 = 2, передача S2H процесу з target_tid = tid - 2.
    // 13. Якщо tid mod 4 = 0, отримання S2H з процесу з source_tid = tid + 2, обчислення S4H=merge(S2H, S2H).
	if (tid%4 == 2) 
	{
		MPI_Send(S + tid*H, 2*H, MPI_INT, tid-2, tid, graph_comm);
	}
	else if (tid%4 == 0) 
	{
		MPI_Recv(S + (tid+2)*H, 2*H, MPI_INT, tid+2, tid+2, graph_comm, MPI_STATUS_IGNORE);
		std::inplace_merge(	S + tid*H, 	 S + (tid+2)*H, 
							S + (tid+4)*H);
	}
    // 14. Якщо tid = 4, передача S4H процесу з target_tid = 0.
    // 15. Якщо tid = 0, отримання S4H з процесу з source_tid = 4, обчислення S=merge(S4H, S4H), передача S іншим процесам.
	if (tid == 4) 
	{
		MPI_Send(S + 4*H, 4*H, MPI_INT, 0, 4, graph_comm);
	}
	else if (tid == 0) 
	{
		MPI_Recv(S + 4*H, 4*H, MPI_INT, 4, 4, graph_comm, MPI_STATUS_IGNORE);
		std::inplace_merge(	S + tid*H, 	 S + (tid+4)*H, 
							S + (tid+8)*H);
	}
    // 16. Якщо tid != 0, отримання S з процесу з source_tid = 0.
	MPI_Scatter(S, H, MPI_INT, S+tid*H, H, MPI_INT, 0, graph_comm);
	
    // 17. Обчислення AH = e * (B *(MO * MZH)) + SH * d.
	for (int sumB_MO_MZ, k = tid*H; k < (tid+1)*H; k++) {
		sumB_MO_MZ = 0;
		for (int sumMO_MZ, i = 0; i < N; i++) {
			sumMO_MZ = 0;
			for (int j = 0; j < N; j++)
				sumMO_MZ += MO[j*N + i] * MZ[k*N + j];
			sumB_MO_MZ += sumMO_MZ * B[i];
		}
		AH[k%H] = ei*sumB_MO_MZ + S[k] * d;
	}
    // 18. Якщо tid = 0, зібрати AH з усіх процесів, вивести А.
	MPI_Gather(AH, H, MPI_INT, A, H, MPI_INT, 0, graph_comm);
	
	if (tid == 0)
	{
		if (N < 17)
		{
			std::string res = "A: ";
			for (int i = 0; i < N; i++)
				res += " " + std::to_string(A[i]);
			std::cout << res << std::endl;
			fflush(stdout);
			free(A);
		}
		else
		{
			printf("Output supressed\n");
			fflush(stdout);
		}
	}
	printf("Thread %d finished\n", tid);
	fflush(stdout);
	MPI_Finalize();
	if (tid == 0)
	{
		std::cout << clock() - start << std::endl;
	}
	free(B);
	free(S);
	free(Z);
	free(MO);
	free(MZ);
	free(AH);
	return 0;
}