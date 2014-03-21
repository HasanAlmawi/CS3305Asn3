#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

/*Function executed by threads*/
void *enteroffice(void * arg);
/*Function executed by TA*/
void *answerQ(void * arg);

/*Mutex to protect access to printing*/
pthread_mutex_t print_mutex;
/*Mutex to protect access for question-answer*/
pthread_mutex_t qa_mutex;
/*Binary semaphore to notify question asked*/
sem_t q_asked;
/*Binary semaphore to notify answer received*/
sem_t answered;
/*Semaphore for the office capacity*/
sem_t office_max;


struct Params{
	int studentNumber;
	int questionNumber;
};

int main(int argc, char **argv){
	//Initializing the array to read in arguments
	int args[3] = { 0 };
	//Checking the number of arguments
	if(argc != 4){
		perror("Number of arguments is incorrect");
		exit(0);
	}
	//Reading in arguments
	int i;
	for(i=0; i<argc-1; ++i){
		args[i] = atoi(argv[i + 1]);
	}
	//Number of students
	int numStudents = args[0];
	//Office size
	int officeCap = args[1];
	//Number of questions
	int questionNum = args[2];


	//Checking if any of the arguments are less than 0
	if(numStudents<1||officeCap<1||questionNum<1){
		printf("All arguments must be greater than zero\n");
		exit(0);
	}

	//Initializing the mutexes
	pthread_mutex_init(&print_mutex, NULL);
	pthread_mutex_init(&qa_mutex, NULL);

	//Initializing the semaphores
	int start = sem_init(&office_max, 0, officeCap);
	if (start < 0){
		perror("Semaphore initialization failed");
		exit(0);
	}

	//Initializing the binary sempahores q_asked and asnwered
	int qstart = sem_init(&q_asked, 0, 0);
	if(qstart<0){
		perror("Sempahore initialization failed");
		exit(0);
	}
	int anstart = sem_init(&answered,0,0);
	if(anstart<0){
		perror("Semaphore initialization failed");
		exit(0);
	}
	struct Params *parameter;
	//Initializing the pthreads
	pthread_t students[numStudents];

	for(i =0; i<numStudents;i++){
		parameter = (struct Params *)calloc(1,sizeof(struct Params));
		parameter->studentNumber = i+1;
		parameter->questionNumber = questionNum;
		pthread_create(&students[i], NULL, (void *)enteroffice, (void *)parameter);
	}

	//TA Params for pthread
	struct Params *TAparam;
	TAparam = (struct Params *)calloc(1,sizeof(struct Params));
	TAparam->studentNumber = numStudents;
	TAparam->questionNumber = questionNum;
	//Initialize TA pthread
	pthread_t TA;
	//Create pthread for TA
	pthread_create(&TA, NULL, (void *)answerQ, (void *)TAparam);

	//Pthread joins for students
	for(i=0;i<numStudents;i++){
		pthread_join(students[i], NULL);
	}
	//Pthread join for TA
	pthread_join(TA,NULL);

	//Destroying mutexes and semaphores after completion
	pthread_mutex_destroy(&print_mutex);
	pthread_mutex_destroy(&qa_mutex);
	sem_destroy(&office_max);
	sem_destroy(&q_asked);
	sem_destroy(&answered);

	printf("All questions answered by TA.\n");
	return 0;
}

//Doing the thing with the thing
void * enteroffice(void * arg){
	struct Params *answer;
	answer = (struct Params *) arg;
	int sN = answer->studentNumber;
	if(sem_trywait(&office_max)){
		printf("I am student %d and I'm waiting outside the TA's door\n", sN);
		//Students wait till the office permits for students to enter
		//sem_wait(&office_max);
	}
	//Student enters the room
	printf("I am student %d and I am entering the room\n", sN);
	//Student begins asking question
	int qNum = answer->questionNumber;
	int q;
	for(q = 0; q<qNum;q++){
		//Put student into mutex
		pthread_mutex_lock(&qa_mutex);
		//Print the question
		pthread_mutex_lock(&print_mutex);
		int printnum = q +1;
   		printf("I am student %d asking question %d\n", sN, printnum);
    	pthread_mutex_unlock(&qa_mutex);
		sem_post(&answered);
		//Student waits on a question to be answered
    	//Post the Q asked semaphore for other students to ask questions
    	sem_wait(&q_asked);
    	//Get out of mutex
    	pthread_mutex_unlock(&print_mutex);	
	}
	//Print that the student is leaving
	printf("I am student %d and I am leaving the room\n", sN);
	//Post semaphore for capacity
	sem_post(&office_max);
}

//Doing the other thing with the other thing
void * answerQ(void * arg){
	struct Params *answer;
	answer = (struct Params *) arg;
	int questions = answer->questionNumber * answer->studentNumber;
	while(questions>0){
		sem_wait(&answered);
		printf("I am TA and this will be discussed in class\n");
		sem_post(&q_asked);
		questions--;
	}
}