#include <string.h>
#include "stratum.h"
#include "satoshi/uint256.h"
#include "RandomX/src/randomx.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

//    printf("%s\n", __func__);
    static bool is_init; // = false;
//    static char randomx_seed[64]; //={0}; // this should be 64 and not 32
    static uint256 randomx_seed2;
    static randomx_flags flags;
    static randomx_vm *rx_vm; // = nullptr;
//    static randomx_cache *cache; // = nullptr;
static randomx_cache *randomx_cpu_cache;
static randomx_dataset *randomx_cpu_dataset;
static struct Dataset_threads {
	pthread_t thr;
	int id;
} *dataset_threads;

//static pthread_mutex_t g_randomx_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_randomx_cond1 = PTHREAD_COND_INITIALIZER;
static pthread_barrier_t mybarrier;
static bool init_ready = true;
static int ready_or_not = 0;
static time_t TimeInit;
static time_t TimeControl;
static int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
void randomx_init_barrier(const int num_threads) {
	pthread_barrier_init(&mybarrier, NULL, num_threads);
}

static void* dataset_init_cpu_thr(void *arg) {

	int i = *(int*)arg;
    int n = num_cpus;
    if (num_cpus!=6) {
        stratumlog("dataset calculated with %d threads \n",num_cpus);
         n = 6;
    }
	randomx_init_dataset(randomx_cpu_dataset, randomx_cpu_cache, (i * randomx_dataset_item_count()) / n, ((i + 1) * randomx_dataset_item_count()) / n - (i * randomx_dataset_item_count()) / n);
	return NULL;
}

 void init_dataset(uint256 theseed)
{

       	CommonLock(&g_randomx_mutex);
    stratumlog("func %s pthread pthread_self %ld\n",__func__,pthread_self()/*syscall(__NR_gettid)*/);
           g_randomx_cond1 = PTHREAD_COND_INITIALIZER;           
           init_ready=false; 
      	CommonUnlock(&g_randomx_mutex);
        CommonLock(&g_randomx_mutex);
        stratumlog("before sleep ready_or_not %d\n",ready_or_not);
//        sleep(1);
        CommonUnlock(&g_randomx_mutex);
        CommonLock(&g_randomx_mutex);     
        int time = 0;   
        while (ready_or_not>0) {
        sleep(1);
        time++;    
        if (time>10) 
            ready_or_not = 0;
        stratumlog("after sleep ready_or_not %d\n",ready_or_not);
        }
        CommonUnlock(&g_randomx_mutex);
        CommonLock(&g_randomx_mutex);    
           if (randomx_cpu_dataset)
           		  randomx_release_dataset(randomx_cpu_dataset);
           stratumlog("init started\n");
//	if (thr_id == 0)
//	{
    stratumlog("dataset calculated with %d threads \n",num_cpus);
    if (num_cpus!=6) num_cpus=6;
		flags = randomx_get_flags();


		flags |= RANDOMX_FLAG_LARGE_PAGES;
//		flags |= RANDOMX_FLAG_ARGON2_AVX2;
//		flags |= RANDOMX_FLAG_ARGON2_SSSE3;
//		flags |= RANDOMX_FLAG_ARGON2;
		flags |= RANDOMX_FLAG_FULL_MEM;
//		flags |= RANDOMX_FLAG_JIT;
//		flags |= RANDOMX_FLAG_SECURE;


		randomx_cpu_cache = randomx_alloc_cache(flags);
		if (randomx_cpu_cache == nullptr) {
			stratumlog( "Cache allocation failed\n"); 
		}
		randomx_init_cache(randomx_cpu_cache, theseed.GetHex().c_str(), theseed.GetHex().size());
		randomx_cpu_dataset = randomx_alloc_dataset(flags);
		if (dataset_threads == NULL) {
			dataset_threads = (Dataset_threads*)malloc(num_cpus * sizeof(Dataset_threads));
		}
//// make sure all the threads are ran on different cpu thread
		cpu_set_t cpuset;
		pthread_attr_t attr;	
		pthread_attr_init(&attr);            

		for (int i = 0; i < num_cpus; ++i) {
		    CPU_ZERO(&cpuset);
		    CPU_SET(i,&cpuset);            
            int s = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
			dataset_threads[i].id = i;
			pthread_create(&dataset_threads[i].thr, &attr, dataset_init_cpu_thr, (void*)&dataset_threads[i].id);
		}
		for (int i = 0; i < num_cpus; ++i) {
			pthread_join(dataset_threads[i].thr, NULL);
		}

		randomx_release_cache(randomx_cpu_cache);
                   stratumlog("init ended\n");


        init_ready=true;
        ready_or_not=0;
        pthread_cond_broadcast(&g_randomx_cond1);


        CommonUnlock(&g_randomx_mutex); 

}


void rx_slow_hash(const char* data, char* hash, int length, uint256 seedhash)
{



    CommonLock(&g_randomx_mutex);
    while (!init_ready)
        pthread_cond_wait(&g_randomx_cond1,&g_randomx_mutex);  

    CommonUnlock(&g_randomx_mutex);


    randomx_vm *rx_vm2 = nullptr; 
    ready_or_not += 1;
        rx_vm2 = randomx_create_vm(flags, nullptr, randomx_cpu_dataset);
    if (rx_vm2==nullptr)
        rx_vm2 = randomx_create_vm(flags, nullptr, randomx_cpu_dataset);

//    stratumlog("before calcultae hash %s\n", __func__);
    randomx_calculate_hash(rx_vm2, data, length, hash);
        randomx_destroy_vm(rx_vm2);
    ready_or_not -= 1;
  
    CommonLock(&g_randomx_mutex);
    if (ready_or_not!=0 && !init_ready) {
          printf(" %s ready or not  %d pthread_self %ld\n", __func__,ready_or_not,pthread_self()/*syscall(__NR_gettid)*/);        
    }   
    while (!init_ready)
        pthread_cond_wait(&g_randomx_cond1,&g_randomx_mutex); 


    CommonUnlock(&g_randomx_mutex);

   	
//       	CommonUnlock(&g_randomx_mutex);
}
