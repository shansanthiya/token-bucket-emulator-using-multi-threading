#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <math.h>
#include "my402list.h"
#include "cs402.h"

typedef struct input_values
{
	int pack_no;
	double lambda;
	double mu;
	double r;
	int b;
	int p;
	int n;
	char fname[20];
	struct timeval q1_en, q1_ex, q2_en, q2_ex, p_en, p_ex, ser_en, ser_ex;
}input;

int token_count;
My402List *q1;
My402List *q2;

int token_p;
int pack_ser_count;
int server_status;
int pack_app;
int pack_drop;
int pack_rmv;
int tok_drop;
int t_flag;
int sig_flag;

char *elem[3];
char buff[1026];
FILE *fp;

struct timeval start_time;
struct timeval end_time;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t q2_bc = PTHREAD_COND_INITIALIZER;

double intr_arr;
double pack_serv;
double pack_q1;
double pack_q2;
double pack_s1;
double pack_s2;
double emu_time;
double pack_sys;
double pack_arr;
double pack_depart;
double sys_sq;
double sys_time;

void init( input * );
void init_default(input *);
void cinit(int ,char *[],input *);
void print( input *);
void * packet_func(void *);
void * token_func(void *);
void * server1_func(void *);
void * server2_func(void *);
void * term_func( void *);

double get_time(struct timeval);
void stats();

pthread_t packet;
pthread_t token;
pthread_t server1;
pthread_t server2;
pthread_t terminate;
sigset_t sig_mask;


//create and join threads
int main( int argc, char * argv[])
{
	sigemptyset(&sig_mask);
  	sigaddset(&sig_mask, SIGINT);
  	sigprocmask(SIG_BLOCK,&sig_mask,0);

	q1 = (My402List *) malloc(sizeof(My402List));
	q2 = (My402List *) malloc(sizeof(My402List));
	
	input *in = (input *)malloc(sizeof(input));
	init(in);
	
	init_default(in);
	
	cinit(argc, argv, in);
	
	if(t_flag == 1)
	{


		fp = fopen(in->fname, "r");
		if ( fp == NULL)
		{
			fprintf(stderr, "\n**FILE: %s IS NOT VALLID**: ",in->fname );
			perror("");
			fprintf(stderr,"**COMMAND LINE INPUT SHOULD BE OF THE FORM './warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile] : invalid input**\n\n");
			
			exit(1);
		}
		fgets(buff, sizeof(buff),fp);

		if(atoi(buff) == 0)
		{
			fprintf(stderr,"\n**Line 1 of file is not a number : invalid input**");
			fprintf(stderr,"\n**COMMAND LINE INPUT SHOULD BE OF THE FORM './warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile] : invalid input**\n\n");
			exit(1);
		}
		in->n = atoi(buff);

	}

	print(in);
	pack_ser_count = in -> n;
	
	gettimeofday(&start_time, 0);
	printf("\n\n\n%012.3fms: emulation begins", get_time(start_time));
	pthread_create(&packet,0, packet_func, in);
	
	//argu = 2;
	pthread_create(&token,0, token_func,in);
	
	//argu = 3;
	pthread_create(&server1,0, server1_func,in);
	
	//argu = 4;
	pthread_create(&server2,0, server2_func,in);

	pthread_create(&terminate,0,term_func,0);

	pthread_join(packet,0);
	pthread_join(token,0);
	pthread_join(server1,0);
	pthread_join(server2,0);
	if(sig_flag == 1)
	{
		pthread_join(terminate,0);
	}

	gettimeofday(&end_time, 0);
	printf("\n%012.3fms: emulation ends\n", get_time(end_time));

	if (t_flag == 1)
	{
		fclose(fp);
	}

	stats();
	return 0;	
}

//initialize all
void init( input *temp)
{
	temp->lambda = 0.0;
	temp->mu = 0.0;
	temp->r = 0.0;
	temp->b = 0;
	temp->p = 0;
	temp->n = 0;
	//temp->fname = NULL;
}

//initialize default values
void init_default( input *temp)
{
	temp->lambda = 1.0;
	temp->mu = 0.35;
	temp->r = 1.5;
	temp->b = 10;
	temp->p = 3;
	temp->n = 20;
}

//print parameters
void print( input *temp)
{
	printf("\n Emulation Parameters:");

	printf("\n\t number to arrive = %d",temp->n);
	if( t_flag == 0 )
	{
		printf("\n\t lambda = %.6g",temp->lambda);
		printf("\n\t mu = %.6g",temp->mu);
	}

	printf("\n\t r = %.6g",temp->r);
	printf("\n\t B = %d",temp->b);
	if( t_flag == 0 )
	{
		printf("\n\t P = %d",temp->p);
	}

	if( t_flag == 1)
	{
		//printf("\ntest:::::%s",temp->fname);
		printf("\n\t tsfile = %s",temp->fname);
		//printf("\n\t tsfile = %s",temp->fname);
	}
	//printf("**.\n");
	return;
}

//take command line inputs
void cinit(int argnum, char * arglist[], input *temp)
{
	//printf("$$.");
	int i;
	//printf("%d\n",argnum);
	//char *temp = (char *)malloc(sizeof(int));
	//memset(temp,	'\0',sizeof(int));
	if( argnum % 2 == 0)
	{
		fprintf(stderr,"\n**COMMAND LINE INPUT SHOULD BE OF THE FORM './warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile] : invalid input**\n\n");
   		exit(1);
	}
	for(i = 1; i < argnum; i=i+2)
	{
		if(strncmp(arglist[i],"-lambda",strlen("-lambda")) == 0)
		{
			temp -> lambda = atof(arglist[i+1]);
		}
		if(strncmp(arglist[i],"-mu",strlen("-mu")) == 0)
		{
			temp -> mu = atof(arglist[i+1]);
		}
		if(strncmp(arglist[i],"-r",strlen("-r")) == 0)
		{
			temp -> r = atof(arglist[i+1]);
		}
		if(strncmp(arglist[i],"-B",strlen("-B")) == 0)
		{
			temp -> b = atoi(arglist[i+1]);
		}
		if(strncmp(arglist[i],"-P",strlen("-P")) == 0)
		{
			temp -> p = atoi(arglist[i+1]);
		}
		if(strncmp(arglist[i],"-n",strlen("-n")) == 0)
		{
			temp -> n = atoi(arglist[i+1]);
		}
		if(strncmp(arglist[i],"-t",strlen("-t")) == 0)
		{	
			t_flag = 1;
			strncpy(temp -> fname,arglist[i+1],strlen(arglist[i+1]));
		}

		if((strncmp(arglist[i],"-t",strlen("-t")) != 0) && (strncmp(arglist[i],"-n",strlen("-n")) != 0) &&(strncmp(arglist[i],"-P",strlen("-P")) != 0) &&
			(strncmp(arglist[i],"-B",strlen("-B")) != 0) &&(strncmp(arglist[i],"-r",strlen("-r")) != 0) && (strncmp(arglist[i],"-mu",strlen("-mu")) != 0) &&
			(strncmp(arglist[i],"-lambda",strlen("-lambda")) != 0))
		{	
			fprintf(stderr,"\n**COMMAND LINE INPUT SHOULD BE OF THE FORM './warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile] ': invalid input**\n\n");
   			exit(1);
		}
	}
	
}

//packet thread
void * packet_func(void *arg)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);

	struct timeval p_arr,p_drop;
	struct timeval temp = start_time;
	struct timeval time1,time2;
	double time_temp = 0.0;
	char *line;
	//struct timeval p_temp;
	//printf("\npacket thread\n");
	My402ListInit(q1);
	int pack_count = 0;

	input * in_pack1 = (input *) arg;

	for(pack_count = 1; pack_count <= in_pack1->n; pack_count++)
	{
		//int k = 1;
		input *in_pack = (input *)malloc(sizeof(input));
		
		in_pack->pack_no = pack_count;
		in_pack->lambda = in_pack1->lambda;
		in_pack->mu = in_pack1->mu;
		in_pack->r = in_pack1->r;
		in_pack->b = in_pack1->b;
		in_pack->p = in_pack1->p;
		in_pack->n = in_pack1->n;
		//printf("$$%d in_pack->n ",in_pack->n);
		if (t_flag == 1)
		{
			//while(!feof(fp))
			//{
				if (fgets(buff, sizeof(buff),fp) != NULL)
		   		{
		   			int length =  strlen(buff);
		   			if ( length > 1024)
		   			{
		   				fprintf(stderr,"\n**THE LINE EXCEEDS 1024 CHARACTERS: invalid **\n");
		   				exit(1);
		   			}
		   			line = strtok (buff, " \t");

		   			int i = 0;
		   			while(line != NULL)
		   			{
		   				
		   				elem[i] = line;
		   				line = strtok (NULL, " \t");
		   				//printf ( "\n #### %s #### \n",elem[i]);
		   				i++;
		   			}


				}

				in_pack->lambda = atof(elem[0]);
				in_pack->p = atoi(elem[1]);
				in_pack->mu = atof(elem[2]);
			//}
		}

		//if ( t_flag == 0)
		//{
			double pack;
			//printf("@%d", in_pack -> n);
			pack = in_pack -> lambda;

			if(t_flag == 0)
			{
				pack = 1 / pack;
				if( pack > 10)
				{
					pack = 10;
				}
				if ( pack == 0.0001)
				{
					pack = (pack * 1000);
				}
				else
				{
					pack = round(pack * 1000);
				}
			}

		if ( (pack - time_temp)>0)
		{	//printf("\npack_time_sleep %f\n", pack - time_temp );
			usleep(((pack - time_temp) * 1000));
		}
		gettimeofday(&time1,0);
		strncpy(in_pack->fname,in_pack1->fname,strlen(in_pack1->fname));
		gettimeofday(&p_arr,0);
		pack_arr = get_time(p_arr);
		in_pack->p_en = p_arr;
		pthread_mutex_lock(&m);
		if(in_pack->p > in_pack-> b)
		{
			pack_drop++;

			gettimeofday(&p_drop,0);
			printf("\n%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3fms, dropped", get_time(p_arr), in_pack->pack_no, in_pack->p,get_time(p_arr) - get_time(temp));
			intr_arr += get_time(p_arr) - get_time(temp);
			//pthread_mutex_unlock(&m);	
			//break;
		}
		else
		{
			
				printf("\n%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3fms", get_time(p_arr), in_pack->pack_no, in_pack->p,get_time(p_arr) - get_time(temp));
				intr_arr += get_time(p_arr) - get_time(temp);
				//printf("\n&&&&&&&&%f\n",get_time(p_arr) - get_time(temp));
				My402ListAppend (q1, in_pack);
				gettimeofday(&(in_pack->q1_en),0);
				printf("\n%012.3fms: p%d enters Q1", get_time(in_pack->q1_en),in_pack->pack_no);
				pack_app++;	
				//printf("\n q1 packet: %d\n",in_pack->pack_no);

			
				if( token_count >= (in_pack->p )&& (My402ListLength (q1) == 1))
				{
					//printf("\nppc%dppp%d",token_count,in_pack->p);
					My402ListElem *first = My402ListFirst(q1);
					My402ListUnlink( q1 , first );
					token_count = token_count - in_pack->p;
					gettimeofday(&(in_pack->q1_ex),0);
					printf("\n%012.3fms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token", get_time(in_pack->q1_ex), in_pack->pack_no,get_time(in_pack -> q1_ex) - get_time(in_pack -> q1_en), token_count);
					pack_q1 += get_time(in_pack->q1_ex)-get_time(in_pack->q1_en);
					My402ListAppend( q2, in_pack);
					gettimeofday(&(in_pack->q2_en),0);
					printf("\n%012.3fms: p%d enters Q2", get_time(in_pack->q2_en),in_pack->pack_no);
					
					if(My402ListLength(q2) == 1 && server_status == 0)
					{
						pthread_cond_broadcast(&q2_bc);
					}

				}
				
			}
			temp = p_arr;
		//}
		gettimeofday(&time2,0);
		pthread_mutex_unlock(&m);
		time_temp = get_time(time2) -get_time(time1);
		//printf("\n print_func_check \n");
	}
	//printf("\n packet dead \n");
	return 0;
}

void * token_func(void *arg)
{
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);

	struct timeval t_arr;
	struct timeval t_drop;
	struct timeval time1,time2;
	double temp_time;
	//printf("\ntoken thread\n");
	token_count = 0; 
	//int token_p = 0;
	int tok_num;
	input * in_tok1 = (input *) arg;
	//printf("\n######%d",in_tok1->n);

	double tok;
	//printf("@%d", in_tok1 -> b);
	tok_num = in_tok1 -> b;
	tok = in_tok1 -> r;

	tok = 1 / tok;
	if ( tok > 10)
	{
		tok = 10;
	}
	tok = tok * 1000;
	
	while(1)
	//for(tok_count = 1; tok_count <= in_tok -> b; tok_count++)
	{
		//printf("\n token");
		if (tok-temp_time > 0)
		{   //printf("\ntoken_time_sleep %f\n", tok - temp_time );
			usleep(((tok - temp_time) * 1000));
		}
		gettimeofday(&time1,0);
		pthread_mutex_lock(&m);
		
		//printf("##pack drop%d, ##pack app %d## in_tok1->n%d### %d\n",pack_drop,pack_app ,in_tok1->n,My402ListLength(q1));
		if(((pack_drop + pack_app) == in_tok1->n) && My402ListEmpty(q1))
		{
			//printf("\n token break check\n");
			pthread_mutex_unlock(&m);
			break;
		}
		token_count++;
		gettimeofday(&t_arr,0);
		token_p++;

		//if(token_count > 0)
		//{
			//printf("\n token: %d\n",token_count);
		//}
		
		
		if(token_count <= tok_num)
		{
			
				if( token_count == 1)
				{
					printf("\n%012.3fms: token t%d arrives, token bucket now has %d token", get_time(t_arr), token_p, token_count);
				}
				else
				{
					printf("\n%012.3fms: token t%d arrives, token bucket now has %d tokens", get_time(t_arr), token_p, token_count);
				}

				if (My402ListEmpty(q1) == 0)
				{
					My402ListElem *first = My402ListFirst(q1);
					input *in_tok = first->obj;
				
					if((token_count >= (in_tok -> p) ))
					{
					
						My402ListUnlink( q1 , first );
						token_count = token_count - in_tok->p;
						gettimeofday(&(in_tok->q1_ex),0);
						printf("\n%012.3fms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token", get_time(in_tok->q1_ex), in_tok->pack_no, get_time(in_tok -> q1_ex) - get_time(in_tok -> q1_en), token_count);
						pack_q1 += get_time(in_tok->q1_ex)-get_time(in_tok->q1_en);
						My402ListAppend( q2, in_tok);
						gettimeofday(&(in_tok->q2_en),0);
						printf("\n%012.3fms: p%d enters Q2", get_time(in_tok->q2_en),in_tok->pack_no);
					
						if(My402ListLength(q2) == 1 && server_status == 0)
						{
							pthread_cond_broadcast(&q2_bc);
						}	
					
					}
				}

			//}
			

		}
		//pthread_mutex_unlock(&m);
		
		else 
		{
			//printf("\n ##TOKEN DROPPED %d## \n", token_count--);
			gettimeofday(&t_drop,0);
			//printf("\n%012.3fms: token t%d arrives, token dropped\n", get_time(t_drop),token_p);
			
			tok_drop++;
			token_count--;
			
		}
		gettimeofday(&time2,0);
		pthread_mutex_unlock(&m);
		temp_time = get_time(time2)-get_time(time1);
		
		//My402ListAppend (q1, arg);	
		
		
	}
	pthread_mutex_lock(&m);
	pthread_cond_broadcast(&q2_bc);
	pthread_mutex_unlock(&m);
	//printf("\n token dead \n");
	
	return 0;
}

void * server1_func(void *arg)
{
	//printf("\nserver1 thread\n");

	input * in_ser11 = (input *) arg;
	double time;

	 

	while(1)
	{
		//printf("\n pack_ser_count1: %d",pack_ser_count);
		pthread_mutex_lock(&m);
		while(My402ListEmpty ( q2 ))
		{
			
			//printf("\n pack_ser_count1: %d",pack_ser_count);

			if( sig_flag == 1)
			{
				//printf("\n pack_ser_count11: %d",pack_ser_count);
				pthread_cond_broadcast(&q2_bc);
				pthread_mutex_unlock(&m);
				//pthread_cond_broadcast(&q2_bc);
				return 0;
			}
			if(((pack_drop + pack_app) == in_ser11->n) && My402ListEmpty(q1))
			{
				
				pthread_cancel(terminate);
				pthread_cond_broadcast(&q2_bc);
				pthread_mutex_unlock(&m);
				//pthread_cond_broadcast(&q2_bc);
				return 0;
			}
			
			//printf("\n s1 wait\n");
			server_status = 0;
			pthread_cond_wait(&q2_bc, &m);

		}
		//printf("\n s1 out\n");
		server_status = 1;
		
		My402ListElem *first = My402ListFirst(q2);

			input *in_ser1 = first->obj;

		
		
		double mu = 0.0;
		mu = in_ser1 -> mu;

		if(t_flag == 0)
		{
			mu = 1 / mu;
			//printf("\n1/mu : %f", mu);
			if ( mu > 10)
			{
				mu = 10;
			}
			mu = round(mu * 1000);
		}

		My402ListUnlink( q2 , first );
		gettimeofday(&(in_ser1->q2_ex),0);
		printf("\n%012.3fms: p%d leaves Q2, time in Q2 = %.3fms", get_time(in_ser1->q2_ex), in_ser1->pack_no, get_time(in_ser1 -> q2_ex) - get_time(in_ser1 -> q2_en));
		pack_q2 += get_time(in_ser1->q2_ex)-get_time(in_ser1->q2_en);		
		//printf("pack_q2:%f",pack_q2);
		
		pack_ser_count--;
		
		gettimeofday(&(in_ser1->ser_en),0);
		printf("\n%012.3fms: p%d begins service at S1, requesting %.0fms of service", get_time(in_ser1->ser_en), in_ser1->pack_no, mu);
		//printf("\n$$$$%f\n",mu);
		pthread_mutex_unlock(&m);
		//printf("\n##%f\n",mu * 1000);
		if( mu > 0)
		{
			usleep((mu * 1000));
		}
		//printf("\n$$$$%f\n",mu);
		pthread_mutex_lock(&m);

		gettimeofday(&(in_ser1->ser_ex),0);
		
		printf("\n%012.3fms: p%d departs from S1, service time = %.3fms, time in system = %.3fms", get_time(in_ser1->ser_ex), in_ser1->pack_no, get_time(in_ser1->ser_ex)-get_time(in_ser1->ser_en), get_time(in_ser1->ser_ex)-get_time(in_ser1->p_en));
		pack_serv += get_time(in_ser1->ser_ex) - get_time(in_ser1->ser_en);
		pack_s1 += get_time(in_ser1->ser_ex) - get_time(in_ser1->ser_en);
		pack_depart = get_time(in_ser1->ser_ex);
		//printf("\n...depart..%f",pack_depart);
		pack_sys += (pack_depart - get_time(in_ser1->p_en));
		time = (pack_depart - get_time(in_ser1->p_en));
		sys_sq = pow(time,2);
		sys_time += sys_sq;
		
		if(((pack_drop + pack_app) == in_ser1->n) && My402ListEmpty(q1) && My402ListEmpty(q2))
		{
			//printf("\n pack_ser_count1: %d",pack_ser_count);
			pthread_cond_broadcast(&q2_bc);
			pthread_cancel(terminate);
			//pthread_cancel(server2);
			pthread_mutex_unlock(&m);
			//pthread_cond_broadcast(&q2_bc);
			break;
		}
		if( sig_flag == 1)
		{
			pthread_cond_broadcast(&q2_bc);
			
			pthread_mutex_unlock(&m);
			//pthread_cond_broadcast(&q2_bc);
			break;
		}
		pthread_mutex_unlock(&m);
	}
	//printf("\n server 1 dead\n");
	return 0;
}

void * server2_func(void *arg)
{
	
	//printf("\nserver2 thread\n");
	input * in_ser21 = (input *) arg;
	double time;

	 

	while(1)
	{
		pthread_mutex_lock(&m);
		
		while(My402ListEmpty ( q2 ))
		{
			
			if( sig_flag == 1)
			{
				//printf("\n pack_ser_count21: %d",pack_ser_count);
				pthread_cond_broadcast(&q2_bc);
				//pthread_cancel(terminate);
				pthread_mutex_unlock(&m);
				//pthread_cond_broadcast(&q2_bc);
				return 0;
			}
			if(((pack_drop + pack_app) == in_ser21->n) && My402ListEmpty(q1) )
			{
				
				pthread_cancel(terminate);

				pthread_cond_broadcast(&q2_bc);
				pthread_mutex_unlock(&m);
				//pthread_cond_broadcast(&q2_bc);
				return 0;
			}


			//printf("\n s2 wait\n");
			server_status = 0;
			pthread_cond_wait(&q2_bc, &m);
			
		}
		//printf("\n s2 out\n");
		server_status = 1;
		
		My402ListElem *first = My402ListFirst(q2);

		    input *in_ser2 = first->obj;
		
		
		double mu = 0.0;
		mu = in_ser2 -> mu;

		if(t_flag == 0)
		{
			mu = 1 / mu;
			//printf("\n1/mu : %f", mu);
			mu = round(mu * 1000);
		}

		My402ListUnlink( q2 , first );
		gettimeofday(&(in_ser2->q2_ex),0);
		printf("\n%012.3fms: p%d leaves Q2, time in Q2 = %.3fms", get_time(in_ser2->q2_ex), in_ser2->pack_no, get_time(in_ser2 -> q2_ex) - get_time(in_ser2 -> q2_en));
		pack_q2 += get_time(in_ser2->q2_ex)-get_time(in_ser2->q2_en);	
		//printf("pack_q2:%f",pack_q2);
		pack_ser_count--;
		
		
		gettimeofday(&(in_ser2->ser_en),0);
		printf("\n%012.3fms: p%d begins service at S2, requesting %.0fms of  service", get_time(in_ser2->ser_en), in_ser2->pack_no, mu);
		pthread_mutex_unlock(&m);
		if (mu > 0)
		{
			usleep((mu * 1000));
		}
		pthread_mutex_lock(&m);
		gettimeofday(&(in_ser2->ser_ex),0);
		printf("\n%012.3fms: p%d departs from S2, service time = %.3fms, time in system = %.3fms", get_time(in_ser2->ser_ex), in_ser2->pack_no, get_time(in_ser2->ser_ex)-get_time(in_ser2->ser_en), get_time(in_ser2->ser_ex)-get_time(in_ser2->p_en));
		pack_serv += get_time(in_ser2->ser_ex) - get_time(in_ser2->ser_en);
		pack_s2 += get_time(in_ser2->ser_ex) - get_time(in_ser2->ser_en);
		pack_depart = get_time(in_ser2->ser_ex);
		//printf("\n...depart..%f",pack_depart);
		pack_sys += (pack_depart - get_time(in_ser2->p_en));
		time = (pack_depart - get_time(in_ser2->p_en));
		sys_sq = pow(time,2);
		sys_time += sys_sq;
		
		if(((pack_drop + pack_app) == in_ser2->n) && My402ListEmpty(q1) && My402ListEmpty(q2))
		{
			//printf("\n pack_ser_count2: %d",pack_ser_count);
			pthread_cond_broadcast(&q2_bc);
			pthread_cancel(terminate);
			//pthread_cancel(server1);
			pthread_mutex_unlock(&m);
			//pthread_cond_broadcast(&q2_bc);
			break;
		}
		if( sig_flag == 1)
		{
			pthread_cond_broadcast(&q2_bc);
			pthread_mutex_unlock(&m);
			//pthread_cond_broadcast(&q2_bc);
			break;
		}
		pthread_mutex_unlock(&m);
	}
	//printf("\n server 2 dead\n");
	return 0;
}

double get_time( struct timeval temp)
{

	double time = 1000 * (temp.tv_sec - start_time.tv_sec) + ((double)(temp.tv_usec - start_time.tv_usec)) / 1000;
	return time;

}

void stats()
{
	printf("\nStatistics:\n\n");
	//printf("stats total timeeee %f\n",pack_sys );
	int pack = abs(pack_app - pack_rmv);
	//printf("\n#####pack_app %d ####pack_rmv %d####",pack_app,pack_rmv);
	emu_time = get_time(end_time) - get_time(start_time);


	
	//printf("\n####%d...####%d....%f",pack_app,pack_drop,intr_arr);
	intr_arr = intr_arr /(double) (pack_app + pack_drop);
	if (pack_app + pack_drop <= 0)
	{
		printf("\n\taverage packet inter-arrival time = N/A No packets arrived");
	}
	else
	{
		printf("\n\taverage packet inter-arrival time = %.6g", intr_arr/1000);
	}


	pack_serv = pack_serv / pack;
	if( pack <= 0)
	{
		printf("\n\taverage packet service time = N/A No packets serviced\n");
	}
	else
	{
		printf("\n\taverage packet service time = %.6g\n", pack_serv/1000);
	}


	pack_q1 = pack_q1 / emu_time;
	printf("\n\taverage number of packets in Q1 = %.6g", pack_q1);

	pack_q2 = pack_q2 / emu_time;
	printf("\n\taverage number of packets in Q2 = %.6g", pack_q2);

	
		pack_s1 = pack_s1 / emu_time;
		printf("\n\taverage number of packets in S1 = %.6g", pack_s1);
	
		pack_s2 = pack_s2 / emu_time;
		printf("\n\taverage number of packets in S2 = %.6g\n", pack_s2);
	//}


	pack_sys = pack_sys / pack;
	//printf("\n...%f...%d", pack_sys, pack);
	if ( pack <= 0)
	{
		printf("\n\taverage time a packet spent in system = N/A No packets serviced\n");
		printf("\tstandard deviation of time spent in system = N/A No packets serviced\n");
	}
	else
	{
		printf("\n\taverage time a packet spent in system = %.6g\n", pack_sys/1000);

		double sd = ((sys_time/(pack*1000000)) - pow((pack_sys/(1000)),2));
		
		if(sd < 0)
		{
			sd = -(sd);
		}
		sd = pow((sd),0.5);
		printf("\tstandard deviation of time spent in system = %.6g\n", sd);
	}

	if(token_p == 0)
	{
		printf("\n\ttoken drop probability = NA No tokens arrived");
	}
	else
	{
		double tok_drop_prob = ((double)tok_drop /(double) token_p);	
		//printf("\ncurr token: %d present token: %d dropped token: %d...\n",token_count, token_p, tok_drop);
		printf("\n\ttoken drop probability = %.6g",tok_drop_prob);
	}

	if ((pack_app + pack_drop) == 0)
	{
		printf("\n\tpacket drop probability = NA No packets generated\n");
	}

	else
	{
		double pack_drop_prob = ((double)pack_drop /(double) (pack_app + pack_drop));
		printf("\n\tpacket drop probability = %.6g\n\n",pack_drop_prob);
	}

	return;
}

void * term_func( void * arg)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);

	//int sig; 
	struct timeval unlink;
	struct timeval sig_catch;
	
	sigwait(&sig_mask);

	gettimeofday(&sig_catch, 0);
	printf("\n%012.3fms: SIGINT caught, no new packets or tokens will be allowed", get_time(sig_catch));

	//printf("\nSIGINT caught, no new packets or tokens will be allowed");
	sig_flag = 1;
	pthread_cancel(packet);
	pthread_cancel(token);

	pthread_mutex_lock(&m);
	pthread_cond_broadcast(&q2_bc);
	if(My402ListEmpty(q1) == 0)
	{
		int length = My402ListLength(q1);
		int i = 1;
		My402ListElem *temp;
		for(i = 1; i <= length; i++)
		{
			temp = My402ListFirst(q1);
			input * in_temp = temp->obj;
			My402ListUnlink(q1,temp);
			pack_rmv++;
			gettimeofday(&unlink, 0);
			printf("\n%012.3fms: p%d removed from Q1", get_time(unlink),in_temp->pack_no);

			

		}
	}
	//printf("\n unlink:\n");
	if(My402ListEmpty(q2) == 0)
	{
		int length = My402ListLength(q2);
		int i = 1;
		My402ListElem *temp;
		for(i = 1; i <= length; i++)
		{
			temp = My402ListFirst(q2);
			input * in_temp = temp->obj;
			My402ListUnlink(q2,temp);
			pack_rmv++;
			gettimeofday(&unlink, 0);
			printf("\n%012.3fms: p%d removed from Q2", get_time(unlink),in_temp->pack_no);
		}
	}
	pthread_mutex_unlock(&m);
	//printf("\n term_func dead\n");
	return 0;
}

