#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <thread.h>
#include <unistd.h>
#include <time.h>

#define P 10

typedef struct MSG
{
	unsigned int sender;
	unsigned int TTL;
	unsigned int msg;
	struct MSG *next;
 } node, *node_ptr;
node_ptr Q_head[P+1], Q_tail[P+1];

struct timespec Tim={0, 10000};
thread_t tid,thr[P+1];

void sendMSG(int s,int r,int m,int t);
void enterQ(node_ptr curr_node, int qID);
node_ptr grabMSG(int x);

int rNum, NWL[P+2][P+2]={{0}},num_Empty =8;
char filename[]="stage7_log.txt";
FILE *fp=NULL;
mutex_t lock[P+1];

void *peer(void *arg)
{
	node_ptr y=malloc(sizeof(node));
	int myNBR[P+1],a[10],idx=0,j,sum=0;
	int i,s,t,m,r_frm,num_rounds_no_msg=0;
	int myID = thr_self();
	float avg=0.0;
	for(i=0;i<=rNum+1;i++)
	{
		myNBR[i] = NWL[myID][i];
	}
//	printf("Thread ID %d\n",myID);
//	for(i=1;i<=rNum+1;i++)
//		printf("Cell value: %d\n",myNBR[i]);
	usleep(100);

	while(num_rounds_no_msg<num_Empty)
	{
		y=grabMSG(myID);
		if(y==NULL)num_rounds_no_msg ++;
		else
		{
			while (y!= NULL)
			{
				s=y->sender;
				t=y->TTL;
				m=y->msg;
				if(t>0)
				{
					t--;
					r_frm=s;
					for(i=2;i<=rNum+1;i++)
					{
						if(myNBR[i]==1 && s!=i)
						{
							sendMSG(myID,i,m,t);
							fprintf(fp,"%d, %d, %d, %d, %d\n",s,myID,i,m,t);
							if (num_rounds_no_msg >0)
							{
								a[idx]=num_rounds_no_msg;
								num_rounds_no_msg=0;
								idx++;
							}
						}

					}
				}
				y=grabMSG(myID);
			}
		 }
		free(y);
		usleep(100);
	}

	for (j=0;j<idx;j++)
		sum+=a[j];
	if (idx ==0) avg=0;
	else avg=(float)sum/idx;
	//printf("The minimum number of rounds for thread %d before finding a msg are..%d\n",myID,sum);
	printf("The average number of rounds for thread %d are..%f\n",myID,avg);
	printf("For Thread Id: %d ,Smallest value of Num_EMPTY- %d \n",myID, num_Empty);
	return NULL;
}

void sendMSG(int s,int qID,int m,int t)
{
	node_ptr curr_node;
	curr_node = malloc(sizeof(node));
	if (curr_node==NULL) printf("Node creation failed/n");
	else
	{
		curr_node->sender=s;
		curr_node->TTL=t;
		curr_node->msg=m;
		enterQ(curr_node, qID);
	}
}
void enterQ(node_ptr curr_node, int qID)
{
	mutex_lock(&lock[qID]);
	if (Q_head[qID]== NULL && Q_tail[qID]==NULL)
	{
		Q_head[qID]=curr_node;
	}
	else
	{
		Q_tail[qID]->next=curr_node;
	}
	Q_tail[qID]=curr_node;
	Q_tail[qID]->next=NULL;
	mutex_unlock(&lock[qID]);
}
node_ptr grabMSG(int x)
{
	node_ptr y=malloc(sizeof(node)), tmp=malloc(sizeof(node));
	mutex_lock(&lock[x]);
	if (Q_head[x]==NULL)
		y=NULL;
	else if (Q_head[x]->next != NULL)
	{
		y=Q_head[x];
		tmp=Q_head[x];
		Q_head[x]=Q_head[x]->next;
		free(tmp);
	}
	else {
		y=Q_head[x];
		Q_head[x]=NULL;
		Q_tail[x]=NULL;
		free(Q_head[x]);
	}

	mutex_unlock(&lock[x]);
	return y;
}

int main (int argc, char *argv[])
{
	int x,k,cnt,i, status,ms,tl;
	char line[30],*d,*n;
	FILE *file;
	for (i=1; i<=P+1; i++)
		mutex_init(&lock[i], USYNC_THREAD, NULL);

	if(argc>0)
		file=fopen(argv[1],"r");

	if (file!=NULL && !feof(file))
	{
		fgets(line,20,file);
		rNum=atoi(line);
		for (k=1;k<=rNum;k++)
		{
			fgets(line,20,file);
			d=strtok(line," ");
			cnt=atoi(d);
			while(cnt>0)
			{
				n=strtok(NULL," ");
				x=atoi(n);
				NWL[k][x]=1;
				NWL[x][k]=1;
				cnt--;
			}
		}
//		printf("matrix values:\n");
//		for(k=1;k<=rNum+1;k++)
//		{
//			for(x=1;x<=rNum+1;x++)
//				printf("%d ",NWL[k][x]);
//			printf("\n");
//		}

	}
	else
	{
		printf("Cannot read the file");
		exit(1);
	}

	fp=fopen(filename,"w");
	if (fp==NULL)
		printf("Failed to open file for writing\n");

	for (i=2; i<=rNum+1; i++)
	{
	  thr_create(NULL, 0, peer, (void*)i, THR_BOUND, &thr[i]);
	}

	while(!feof(file))
		{
			fgets(line,20,file);
			sscanf(line,"%d%d",&ms,&tl);
			sendMSG(1, 2, ms, tl); //sender = 1, receiver=2, msg, TTL
			nanosleep(&Tim, NULL);
		}

	while (thr_join(0, &tid, (void**)&status)==0)
	  printf("main - status=%d, first_id=%d\n", status, tid);
	for(i=1;i<=rNum+1;i++)
		if(Q_head[i] != NULL)
			printf("Queue %d is not null\n",i);
	for (i=1; i<=P+1; i++)
		mutex_destroy(&lock[i]);
	fclose(file);
	fclose(fp);
	return 0;
}
