using System;
using System.Threading;
using System.Threading.Tasks;
using System.Diagnostics;

namespace course_work
{
    class Program
    {
        public const int N = 8;
        public const int P = 8;
        public const int H = N / P;
        public static int d;
        public static int e = int.MaxValue;
        public static int[] S = new int[N];
        public static int[] A = new int[N];
        public static int[] Z, B;
        public static int[,] MO, MZ;
        public static object CS_e, CS_B, CS_d, CS_MO;
        static Barrier barrier = new Barrier(participantCount: P);
        static void Main(string[] args)
        {
            Console.WriteLine("Program started");
            var stopwatch = new Stopwatch();
            CS_d = new object();
            CS_e = new object();
            CS_B = new object();
            CS_MO = new object();
            Task[] tasks = new Task[P];
            stopwatch.Start();
            for (int i = 0; i < P; i++)
            {
                int index = i;
                tasks[index]= Task.Factory.StartNew(() => Tx(index));
            }

            Task.WaitAll(tasks);
            stopwatch.Stop();
            Console.WriteLine("Program finished");
            Console.WriteLine("Elapsed: " + stopwatch.ElapsedMilliseconds);
            Console.ReadKey();
        }

        static void Tx(int index)
        {
            int tid = index + 1;
            Console.WriteLine("Task " + tid + " started");
            // 1. Якщо tid = 1, ввести Z.
            // 2. Якщо tid = 6, ввести B, MZ, скопіювати B у S.
            // 3. Якщо tid = 8, ввести MO, d.
            switch (tid)
            {
                case 1:
                    Z = new int[N];
                    for (int i = 0; i < N; i++)
                    {
                        Z[i] = 1;
                    }
                    break;
                case 6:
                    B = new int[N];
                    for (int i = 0; i < N; i++)
                    {
                        B[i] = 1;
                    }
                    B.CopyTo(S,0);
                    MZ = new int[N, N];
                    for (int i = 0; i < N; i++)
                        for (int j = 0; j < N; j++)
                            MZ[i, j] = 1;
                    break;
                case 8:
                    MO = new int[N, N];
                    for (int i = 0; i < N; i++)
                        for (int j = 0; j < N; j++)
                            MO[i, j] = 1;
                    d = 1;
                    break;
                default:
                    break;
            }
            // 4. Сигнал іншим задачам про завершення вводу.
            // 5. Чекати на завершення вводу інших задач.
            barrier.SignalAndWait();
            // 6. Обчислення ei=min(ZH).
            int ei = int.MaxValue;
            for (int i = H * (tid - 1); i < H * tid; i++)
            {
                if (Z[i] < ei)
                {
                    ei = Z[i];
                }
            }
            // 7. Обчислення e=min(e, ei).
            lock (CS_e)
            {
                if (ei < e)
                {
                    e = ei;
                }
            }
            // 8. Обчислення SH=sort(SH).
            mergeSort(S, H * (tid - 1), H * tid - 1);
            // 9. Сигнал іншим задачам про завершення сортування.
            // 10. Чекати на завершення сортування інших задач.
            barrier.SignalAndWait();
            // 11. Якщо tid mod 2 = 1, обчислення S2H = merge(SH, SH).
            if (tid % 2 == 1)
            {
                merge(S, H * (tid - 1), H * tid - 1, H * (tid + 1) - 1);
            }
            // 12. Сигнал іншим задачам про завершення злиття
            // 13. Чекати на завершення злиття інших задач.
            barrier.SignalAndWait();
            // 14. Якщо tid mod 4 = 1, обчислення S4H = merge(S2H, S2H).
            if (tid % 4 == 1)
            {
                merge(S, H * (tid - 1), H * (tid + 1) - 1, H * (tid + 3) - 1);
            }
            // 15. Сигнал іншим задачам про завершення злиття
            // 16. Чекати на завершення злиття інших задач.
            barrier.SignalAndWait();
            // 17. Якщо tid = 1, обчислення S = merge(S4H, S4H).
            if (tid == 1)
            {
                merge(S, 0, H * 4 - 1, H * 8 - 1);
            }
            // 18. Сигнал іншим задачам про завершення злиття
            // 19. Чекати на завершення злиття інших задач.
            barrier.SignalAndWait();
            // 20. Копіювання e.
            lock (CS_e)
            {
                ei = e;
            }
            // 21. Копіювання B.
            int[] Bi = new int[N];
            lock (CS_B) 
            {
                B.CopyTo(Bi, 0);
            }
            // 22. Копіювання MO.
            int[,] MOi = new int[N, N];
            lock (CS_B) 
            {
                MOi = MO;
            }
            // 23. Копіювання d.
            int di;
            lock (CS_d) 
            {
                di = d;
            }
            // 24. Обчислення AH = ei * (Bi * (MOi * MZH)) + SH * di.
            for (int sum2, k = H * (tid - 1); k < H * tid; k++) {
                sum2 = 0;
                for (int sum1, i = 0; i < N; i++) {
                    sum1 = 0;
                    for (int j = 0; j < N; j++)
                        sum1 += MO[i, j] * MZ[j, k];
                    sum2 += sum1 * B[k];
                }
                A[k] = ei * sum2 + S[k] * di;
            }
            // 25. Сигнал іншим задачам про завершення обчислень.
            // 26. Чекати на завершення обчислень інших задач.
            barrier.SignalAndWait();
            // 27. Якщо tid = 1, вивести А.
            if (tid == 1)
            {
                if (N < 20)
                {
                    string result = "";
                    foreach (var i in A)
                    {
                        result = result+i+" ";
                    }
                    Console.WriteLine(result);
                }
                else
                {
                    Console.WriteLine("Output suppressed");
                }
            }
            Console.WriteLine("Task " + tid + " finished");
        }
        static void merge(int []arr, int start, int mid, int end) 
        { 
            int start2 = mid + 1; 
        
            // If the direct merge is already sorted 
            if (arr[mid] <= arr[start2]) 
            { 
                return; 
            } 
        
            // Two pointers to maintain start 
            // of both arrays to merge 
            while (start <= mid && start2 <= end) 
            { 
        
                // If element 1 is in right place 
                if (arr[start] <= arr[start2])  
                { 
                    start++; 
                } 
                else 
                { 
                    int value = arr[start2]; 
                    int index = start2; 
        
                    // Shift all the elements between element 1 
                    // element 2, right by 1. 
                    while (index != start)  
                    { 
                        arr[index] = arr[index - 1]; 
                        index--; 
                    } 
                    arr[start] = value; 
        
                    // Update all the pointers 
                    start++; 
                    mid++; 
                    start2++; 
                } 
            } 
        }    
  
        static void mergeSort(int []arr, int l, int r) 
        { 
            if (l < r) 
            { 
        
                // Same as (l + r) / 2, but avoids overflow 
                // for large l and r 
                int m = l + (r - l) / 2; 
        
                // Sort first and second halves 
                mergeSort(arr, l, m); 
                mergeSort(arr, m + 1, r); 
        
                merge(arr, l, m, r); 
            } 
        }     
    }
    
}
