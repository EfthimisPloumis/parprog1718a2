// ... ο κώδικάς σας για την υλοποίηση του quicksort
// με pthreads και thread pool...

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define QUEUE_SIZE 1000000
#define THREADS 4
#define N 1000000
#define CUTOFF 1000
#define THREADLIMIT 5000

struct message{
	 int n;
    double *a;
	int complete;
	int shutdown;
};

struct message queue[QUEUE_SIZE];
int global_availmsg = 0;
int queue_in=0;
int queue_out=0;

pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t recv_mutex = PTHREAD_MUTEX_INITIALIZER;

void inssort(double *a, int n) {
    int i, j;
    double t;

    for(i = 1; i < n; i++) {
        j = i;
        while((j> 0) && (a[j - 1] > a[j])) {
            t = a[j - 1];
            a[j - 1] = a[j];
            a[j] = t;
            j--;
        }
    }
}

int partition(double *a, int n) {
    int i, j;
    double p, t;
    int first = 0;
    int mid = n/2;
    int last = n-1;

    if(a[first] > a[mid]) {
        t = a[first];
        a[first] = a[mid];
        a[mid] = t;
    }

    if(a[mid] > a[last]) {
        t = a[mid];
        a[mid] = a[last];
        a[last] = t;
    }

    if(a[first] > a[mid]) {
        t = a[first];
        a[first] = a[mid];
        a[mid] = t;
    }

    p = a[mid];
    for(i = 1, j = n - 2; /**/ ; i++, j--) {
        while(a[i] < p) {
            i++;
        }

        while(a[j] > p) {
            j--;
        }

        if(i >= j) {
            break;
        }

        t = a[i];
        a[i] = a[j];
        a[j] = t;
    }

    return i;
}

void quicksort(double *a, int n) {
    int i;

    if(n <= CUTOFF) {
        inssort(a, n);
        return;
    }

    i = partition(a, n);

    quicksort(a, i);
    quicksort(a + i, n - i);
}

int send(double *a, int n, int shutdown, int complete){
	pthread_mutex_lock(&send_mutex);
    if(shutdown){
		queue[queue_in].shutdown=1;
	}
	if(complete){
		queue[queue_in].complete=1;
	}
	queue[queue_in].a=a;
	queue[queue_in].n=n;
    queue_in+=1;
	if(queue_in>=QUEUE_SIZE){
		queue_in=0;
	}
	global_availmsg += 1;

    pthread_cond_signal(&msg_in);
	pthread_mutex_unlock(&send_mutex);
	return 0;
}

int recv(double **a, int *n){
	pthread_mutex_lock(&recv_mutex);
	*a=queue[queue_out].a;
	*n=queue[queue_out].n;

    queue_out+=1;
	if(queue_out>=QUEUE_SIZE){
		queue_out=0;
	}
	global_availmsg -= 1;
	pthread_mutex_unlock(&recv_mutex);
	return 0;
}

// thread function
void *worker_thread(void *args) {
	double **a = NULL;
	int *n = NULL;
	int pivot;

	for( ; ; ){
		pthread_mutex_lock(&mutex);

  		while (global_availmsg<1) {
			pthread_cond_wait(&msg_in,&mutex);
  		}
  		if(queue[queue_out].shutdown){
	  		break;
  		}
  		else if(!queue[queue_out].complete){
			a=&(queue[queue_out].a);
	  		n=&(queue[queue_out].n);
	  		recv(a, n);

  			if(*n <= THREADLIMIT) {
			  	quicksort(*a, *n);
				send(*a, *n, 0, 1);
			}
			else {
				pivot = partition(*a, *n);
				send(*a, *n, 0,0);
				send(*a+pivot, *n-pivot,0,0);
			}
		}
		pthread_mutex_unlock(&mutex);
	}

	pthread_mutex_unlock(&mutex);
	pthread_exit(NULL);
}

int main() {
  double *a;
  pthread_t worker;
  int i,n cn=0 ,count=0;
  int *np;
  double **dummy;

  np = &n;

  if((a = (double *)malloc(N * sizeof(double))) == NULL) {
  	printf("Error at malloc a\n");
  	exit(1);
  }

  srand(time(NULL));
  for(i = 0; i < N; i++) {
  	a[i] = (double)rand() / RAND_MAX;
  }

  for(i=0;i<QUEUE_SIZE;i++){
	  queue[i].n=0;
	  queue[i].a=NULL;
	  queue[i].complete=0;
	  queue[i].shutdown=0;
  }

  // create threads
  for(i=0;i<THREADS;i++){
  	pthread_create(&worker,NULL,worker_thread,NULL);
  }

	send(a, N,0,0);

	dummy = &a;
  	while(count<QUEUE_SIZE){
		for(i=0;i<QUEUE_SIZE;i++){
			pthread_mutex_lock(&mutex);
		  	if(queue[i].complete){
				recv(dummy,np);
			  	count += *np;
		  	}
		  	pthread_mutex_unlock(&mutex);
	  	}
  	}

  	send(a, 0,1,0);

  for(i=0;i<THREADS;i++){
  	pthread_join(worker,NULL);
  }
  // Check
  for(i = 0; i < (N -1); i++) {
  	if(a[i] > a[i + 1]) {
  		printf("Error at element %d\n", i);
  		break;
  	}
  }
  free(a);
  pthread_mutex_destroy(&mutex);
  pthread_mutex_destroy(&send_mutex);
  pthread_mutex_destroy(&recv_mutex);
  pthread_cond_destroy(&msg_in);

  return 0;
}

