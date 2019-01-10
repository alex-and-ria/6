#include<iostream>
#include <stdio.h>
#include<stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define shft 1
#define BUFFSZ 128

using namespace std;
typedef unsigned short int usi;

unsigned char* cypher_str(unsigned char* str){
	usi str_len=strlen((char*)(str));
	for(usi i=0;i<str_len;i++){
		str[i]=(((usi)(str[i])+shft)%256);
	}
	return str;
}

unsigned char* decypher_str(unsigned char* str, unsigned int str_len){
	for(usi i=0;i<str_len;i++){
		str[i]=((usi)((usi)str[i]+(256-shft))%256);
	}
	return str;
}

int main(){
	int sock,lsnr; struct sockaddr_in addr; int reusefl=1; srand(time(NULL));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(7701);
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	unsigned char recv_str[BUFFSZ], send_str[BUFFSZ],dc_str[BUFFSZ];
	if((lsnr=socket(AF_INET,SOCK_STREAM,0))<0){
		perror("\nslsnr<0: "); return 1;
	}
	if(setsockopt(lsnr,SOL_SOCKET, SO_REUSEADDR,&(reusefl),sizeof(int))<0){
		perror("\nsetsockopt!:"); return 1;
	}	
	if(bind(lsnr, (struct sockaddr*)(&addr),sizeof(addr))<0){
		perror("\nbind!: "); return 1;
	}
	listen(lsnr,1);
	while(1){
		if((sock=accept(lsnr,NULL,NULL))<0){
			perror("\naccept!: "); return 1;
		}
		recv(sock,recv_str,sizeof(recv_str),0);//IdA,EKA(r1, IdB) -> EKA(f(r1), Ks, IdB, EKB(Ks, IdA))
		strcpy((char*)dc_str,(char*)decypher_str(recv_str+3,strlen((char*)recv_str)-3));//skip IDa as we know that thic is a lsc.cpp ("l2c");
		unsigned char r1=dc_str[0]; r1++;//f(r1)=r1++;
		unsigned char Ks=rand()%10+'0';
		unsigned char IDa[4],IDb[4],tmpstr[BUFFSZ];
		strcpy((char*)IDa,(char*)recv_str); strcpy((char*)IDb,(char*)(dc_str+1)); IDa[3]='\0';
		sprintf((char*)tmpstr,"%c%s",Ks,IDa);
		sprintf((char*)send_str,"%c%c%s%s",r1,Ks,IDb,cypher_str(tmpstr));
		cout<<"\nIDa="<<IDa<<" IDb="<<IDb<<" r1="<<r1<<" Ks="<<Ks<<" tmpstr="<<tmpstr<<" send_str="<<send_str<<'\n';
		send(sock,cypher_str(send_str),(strlen((char*)send_str)+1),0);
		close(sock);
		char choice='0';
		cout<<"\nfor exit press 'q'"; cin>>choice;
		if(choice=='q'){
			close(lsnr); return 0;
		}
	}
}
