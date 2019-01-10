#include<iostream>
#include <stdio.h>
#include<stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define shft 1
#define dhg 2
#define	dhp 5
#define dhb 3
#define BUFFSZ 64
#define CRC16 0x8005
using namespace std;
typedef unsigned short int usi;

uint16_t gen_crc16(const uint8_t *data, uint16_t size){
	uint16_t out = 0;
	int bits_read = 0, bit_flag;
	if(data == NULL)/* Sanity check: */
		return 0;
	while(size > 0){
		//cout<<"\nout="<<bitset<16>(out)<<" out>>15= "<<bitset<16>(out>>15)<<" bits_read="<<bits_read;
		bit_flag = out >> 15;
		out <<= 1;/* Get next bit: */
		out |= (*data >> bits_read) & 1; // item a) work from the least significant bits
		//cout<<"\tout="<<bitset<16>(out)<<" bit_flag= "<<bit_flag;
		bits_read++;/* Increment bit counter: */
		if(bits_read > 7){
			bits_read = 0;
			data++;
			size--;
		}
		if(bit_flag)/* Cycle check: */
		out ^= CRC16;
	}
	int i;// item b) "push out" the last 16 bits
	for (i = 0; i < 16; ++i){
		//cout<<"\n1out="<<bitset<16>(out)<<" bit_flag= "<<bit_flag;
		bit_flag = out >> 15;
		out <<= 1;
		if(bit_flag)
			out ^= CRC16;
	}
	uint16_t crc = 0;// item c) reverse the bits
	i = 0x8000;
	int j = 0x0001;
	for (; i != 0; i >>=1, j <<= 1){
		if (i & out) crc |= j;
	}
	//cout<<"\n2out="<<bitset<16>(out)<<" crc= "<<bitset<16>(crc);
	return crc;
}

union crc_val{
	uint16_t val;
	unsigned char vals[2]; 
};

struct u_admin{
	unsigned char login[BUFFSZ];
	unsigned int pw_hash;
};

struct u_user{
	unsigned char login[BUFFSZ];
	unsigned int pw_hash;
	usi n_enters;
	bool accsfl;
};

unsigned char* cypher_str(unsigned char* str){
	usi str_len=strlen((char*)str);
	for(usi i=0;i<str_len;i++){
		str[i]=(((usi)((usi)str[i])+shft)%256);
	}
	return str;
}

unsigned char* decypher_str(unsigned char* str, unsigned int str_len){
	for(usi i=0;i<str_len;i++){
		str[i]=((usi)((usi)str[i]+(256-shft))%256);
	}
	return str;
}

usi calc_pw_hash(unsigned char* str){
	usi calc_val=0;
	for(usi i=0;i<strlen((char*)str);i++){
		calc_val+=(usi)str[i];
	}
}

bool udp_rqst(int& sock,int& reusefl,struct sockaddr_in& addr,struct sockaddr_in& recv_addr){
	unsigned int recv_addr_len=sizeof(recv_addr); unsigned char udp_recv_str[BUFFSZ], reqst_str[]="udp_request", udp_send_str[]="1";
	if((sock=socket(AF_INET,SOCK_DGRAM,0))<0){
		perror("\nsocket:"); return 0;
	}
	if(setsockopt(sock,SOL_SOCKET, SO_REUSEADDR,&(reusefl),sizeof(int))<0){
		perror("\nsetsockopt!:"); return 0;
	}
	addr.sin_family=AF_INET;
	addr.sin_port=htons(7700);
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(sock,(struct sockaddr*)(&addr),sizeof(addr))<0){
		perror("\nbind:"); return 0;
	}
	recvfrom(sock,udp_recv_str,BUFFSZ,0,(struct sockaddr*)(&recv_addr),&recv_addr_len);
	if(strcmp((char*)udp_recv_str,(char*)cypher_str(reqst_str))==0){
		sendto(sock,cypher_str(udp_send_str),(strlen((char*)udp_send_str)+1),0,(struct sockaddr*)(&recv_addr),recv_addr_len);
	}
	else{
		cout<<"\nudp_recv_str="<<udp_recv_str<<"| reqst_str="<<reqst_str<<'|'<<'\n';
	}
	close(sock);
	return 1;
}

int send_c_crc(int& sock,unsigned char* send_str){
	crc_val gcrc;
	gcrc.val=gen_crc16(send_str,strlen((char*)send_str));//calc send crc;
	sprintf((char*)(send_str+strlen((char*)send_str)),"%c%c",*gcrc.vals,*(gcrc.vals+1));
	cout<<"\nsend_str="<<send_str<<" strlen(send_str)="<< strlen((char*)send_str)<<'\n';
	return send(sock,cypher_str(send_str),strlen((char*)send_str)+1,0);//send
}

int recv_c_crc(int& sock,unsigned char* c_recv_str,unsigned char* dc_recv_str,bool& crc_match){
	crc_val recv_crc,calc_crc; calc_crc.val=0;
	int recv_ret=recv(sock,c_recv_str,sizeof(c_recv_str),0);///recv
	strcpy((char*)dc_recv_str,(char*)decypher_str(c_recv_str,strlen((char*)c_recv_str)));
	//dh_recv_str=decypher_str(cdh_recv_str,strlen((char*)cdh_recv_str));
	recv_crc.vals[0]=dc_recv_str[(strlen((char*)dc_recv_str)-2)]; recv_crc.vals[1]=dc_recv_str[(strlen((char*)dc_recv_str)-1)]; dc_recv_str[(strlen((char*)dc_recv_str)-2)]='\0';
	calc_crc.val=gen_crc16(dc_recv_str,strlen((char*)dc_recv_str));
	cout<<"\nrecv_crc.val="<<recv_crc.val<<" calc_crc.val="<<calc_crc.val;
	if(recv_crc.val==calc_crc.val) crc_match=1;
	else crc_match=0;
	return recv_ret;
}

int main(){
	int sock,lsnr; int reusefl=1; struct sockaddr_in addr, recv_addr;
	if(!udp_rqst(sock,reusefl,addr,recv_addr)){
		cout<<"\nudp_rqst\n"; return 1;
	}
	unsigned char dh_send_str[BUFFSZ],cdh_recv_str[BUFFSZ],dh_recv_str[BUFFSZ],recv_str[BUFFSZ],send_str[BUFFSZ]; bool crc_match=0; 
	if((lsnr=socket(AF_INET,SOCK_STREAM,0))<0){
		perror("\nlsnr<0: "); return 1;
	}
	if(setsockopt(lsnr,SOL_SOCKET, SO_REUSEADDR,&(reusefl),sizeof(int))<0){
		perror("\nsetsockopt!:"); return 1;
	}
	if(bind(lsnr, (struct sockaddr*)(&addr),sizeof(addr))<0){
		perror("\nbind!: "); return 1;
	}
	listen(lsnr,1);
	if((sock=accept(lsnr,NULL,NULL))<0){
		perror("\naccept!: "); return 1;
	}
	
	usi send_dhv=dhg;
	for(usi i=0;i<dhb-1;i++){
		send_dhv=(send_dhv*dhg)%dhp;
	}
	recv_c_crc(sock,cdh_recv_str,dh_recv_str,crc_match);
	sprintf((char*)dh_send_str,"%d",send_dhv);//cout<<"\nsend_dhv="<<send_dhv<<" dh_send_str="<<dh_send_str<<" strlen(dh_send_str)="<<strlen((char*)dh_send_str);
	send_c_crc(sock,dh_send_str);
	if(crc_match){
		usi s_key=atoi((char*)dh_recv_str);
		for(usi i=0;i<dhb-1;i++){
			s_key=(s_key*dhg)%dhp;
		}
		cout<<"\ns_key="<<s_key<<'\n';
	}
	else{
		cout<<"\ncrc!";
	}
	usi Ks=1;
	//recv(sock)
	/*recv(sock,recv_str,sizeof(recv_str),0);//EKB(Ks, IdA), EKs(r2) -> EKs(f(r2));
	unsigned char Ks=((decypher_str(recv_str,strlen((char*)recv_str)-1))[0]), fr2=((recv_str[strlen((char*)recv_str)-1]+(256-Ks))%256)+1;
	sprintf((char*)send_str,"%c",(fr2+Ks+0)%256);
	cout<<"\nfr2="<<fr2<<" send_str="<<send_str<<'\n';
	send(sock,send_str,(strlen((char*)send_str)+1),0);*/
	
	u_admin admin; admin.login="admin"; admin.pw_hash=calc_pw_hash("1111");
	for(usi i=0;i<3;i++){
		recv(sock,recv_str,sizeof(recv_str),0); decypher_str(recv_str,strlen((char*)recv_str),Ks);
		if(admin.pw_hash==calc_pw_hash(recv_str)){
			strcpy((char*)send_str,"ack");
			send(sock,cypher_str(send_str,Ks),(strlen((char*)send_str)+1),0);
			break;
		}
		else{
			if(i==2){
				close(sock);
				return 0;
			}
			strcpy((char*)send_str,"q");
			send(sock,cypher_str(send_str,Ks),(strlen((char*)send_str)+1),0);
		}
	}
	
	
	
	
	
	
	
	
	
	
	
	return 0;
}
