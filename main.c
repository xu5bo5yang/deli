/*
 * main.cc

 *
 *  Created on: Feb 9, 2015
 *      Author: boyangxu
 */
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <queue>

#include "dthreads.h"

using namespace std;

char **mainargv;
int cashierTotal;

class Board{
private:
	int size;
	int **b;
public:
	void init(int _size){
		int i=0;
		size = _size;
		b = (int**)malloc(size*sizeof(int*));
		for (i = 0; i < size; i++){
			b[i] = (int*)malloc(2*sizeof(int));
			b[i][0] = -9999;
			b[i][1] = -9999;
		}
	}

	void post(int new_s, int cashier){
		int i=0;
		for (i = 0; i < size; i++){
			if (b[i][0]==-9999){
				b[i][0] = new_s;
				b[i][1] = cashier;
				break;
			}
		}
	}

	int find(int cashier){
		int i=0;
		for (i = 0; i < size; i++){
			if (b[i][1]==cashier){
				return 1;
			}
		}
		return 0;
	}

	int* remove(int target, int cashier){
		int i=0;
		int min_dis = 65535;
		int* answer = (int*)malloc(2*sizeof(int));
		int position=-1;
		for (i = 0; i < size; i++){
			if (abs(b[i][0]-target) < min_dis){
				min_dis = abs(b[i][0]-target);
				position = i;
			}
		}
		answer[0]=b[position][0];
		answer[1]=b[position][1];
		b[position][0]=-9999;
		b[position][1]=-9999;
		return answer;
	}

	int getsize(){
		int count = 0;
		int i=0;
		for (i = 0; i < size; i++){
			if (b[i][0] != -9999){
				count++;
			}
		}
		return count;
	}

	int empty(){
		int count = 0;
		int i=0;
		for (i = 0; i < size; i++){
			if (b[i][0] != -9999)
				count++;
		}
		if (count == 0)
			return 1;
		else
			return 0;
	}

	int full(){
		int count = 0;
		int i=0;
		for (i = 0; i < size; i++){
			if (b[i][0] != -9999){
				count++;
			}
		}
		if (count == size)
			return 1;
		else
			return 0;
	}

	void print(){
		int i=0;
		for (i = 0; i < size; i++){
			cout<<"b["<<i<<"][0]=:"<<b[i][0]<<" b["<<i<<"][1]=:"<< b[i][1]<<endl;
		}
	}
};

Board B;

void cashier(void *arg){
	int cashierNumber = atoi((char*)arg);
	int sandwich=-1;
	FILE* inputFile=NULL;
	inputFile=fopen(mainargv[cashierNumber],"r");
	if(inputFile==NULL){
		//perror("File cannot open\n");
		exit(1);
	}
	int flag;
	std::queue<int> sandwiches;
	while(1){
		fscanf(inputFile,"%d",&sandwich);
		if(feof(inputFile)) break;
		sandwiches.push(sandwich);
	}
	fclose(inputFile);
	sandwich=-1;
	while(!sandwiches.empty()){
		if(dthreads_lock(100)==0){
			while(B.find(cashierNumber)||B.full()){
				while(dthreads_wait(100,1000)!=0);
			}
			sandwich=sandwiches.front();
			sandwiches.pop();
			B.post(sandwich, cashierNumber);
			cout<<"POSTED: cashier "<<cashierNumber<<" sandwich "<<sandwich<<endl;
			sandwich=-1;
			dthreads_broadcast(100,1002);
			while(dthreads_unlock(100)!=0){}
		}
	}
	while(dthreads_lock(100)!=0);
	while(B.find(cashierNumber)){
		dthreads_wait(100,1000);
	}
	cashierTotal--;
	dthreads_broadcast(100,1002);
	while(dthreads_unlock(100)!=0);
};

void maker(void *argc){
	int *lastMade=(int*)malloc(2*sizeof(int));
	lastMade[0]=-1;
	lastMade[1]=-1;
	while(cashierTotal>0||B.getsize()){
		if(dthreads_lock(100)==0){
			if(cashierTotal==0 && B.getsize()==0){
				dthreads_unlock(0);
				break;
			}
			while((B.getsize()<cashierTotal)&&!B.full()){
				dthreads_wait(100,1002);
			}
			if(!B.getsize()&&!cashierTotal){
				while(dthreads_unlock(100));
				break;
			}
			lastMade=B.remove(lastMade[0],lastMade[1]);
			cout<<"READY: cashier "<<lastMade[1]<<" sandwich "<<lastMade[0]<<endl;
			//cout<<B.getsize()<<""<<cashierTotal<<endl;
			dthreads_broadcast(100,1000);
			while(dthreads_unlock(100)!=0);
		}
	}
	//cout<<"maker"<<endl;
	//B.print();
};

void deli(void* argc){
	int number=*(int*)argc;
	int i=0;
	char* cashierNumber;
	while(dthreads_start((dthreads_func_t)maker, (void*)&number)){
				//cout<<"maker failed to start"<<endl;
	}
	for(i=0;i<number-2;i++){
		cashierNumber=(char*)malloc(10*sizeof(char));
		sprintf(cashierNumber,"%d",i);
		while(dthreads_start((dthreads_func_t)cashier, (void*)cashierNumber)){
			cashierTotal--;
			//cout<<"cashier "<<mainargv[i]<<" failed to start"<<endl;
		}
	}
	//cout<<"finish"<<endl;
};

int main (int argc, char* argv[]){
	if(argc<3){
		//cout<<"Not enough arguments!"<<endl;
		return 0;
	}
	int boardCapacity=0;
	boardCapacity=atoi(argv[1]);
	if(boardCapacity<=0){
		//cout<<"board capacity is incorrect!"<<endl;
		return 0;
	}
	cashierTotal=argc-2;
	mainargv=(char**)malloc((argc-2)*sizeof(char*));
	int i=0;
	for(i=0;i<argc-2;i++){
		mainargv[i]=(char*)malloc(100*sizeof(char));
	}
	for(i=2;i<argc;i++){
		mainargv[i-2]=argv[i];
	}
	B.init(boardCapacity);
	if(dthreads_init((dthreads_func_t)deli,(void*)&argc)){
		//cout<<"dthreads_init failed\n";
		exit(1);
	}
	return 0;
}
