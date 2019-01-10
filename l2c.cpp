#include<iostream>
#include <stdio.h>
#include<stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define shft_default 1
#define dhg 2
#define	dhp 5
#define dha 2
#define BUFFSZ 64
#define PWSZ 4
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

unsigned char* cypher_str(unsigned char* str,usi shft=shft_default){
	usi str_len=strlen((char*)(str));
	for(usi i=0;i<str_len;i++){
		str[i]=(((usi)(str[i])+shft)%256);
	}
	return str;
}

unsigned char* decypher_str(unsigned char* str, unsigned int str_len,usi shft=shft_default){
	for(usi i=0;i<str_len;i++){
		str[i]=((usi)((usi)str[i]+(256-shft))%256);
	}
	return str;
}

bool udp_rqst(int& sock,struct sockaddr_in& addr,struct sockaddr_in& recv_addr){
	unsigned char udp_recv_str[BUFFSZ], udp_send_str[]="udp_request";
	cout<<"\nsizeof(udp_send_str)="<<sizeof(udp_send_str)<<" strlen(udp_send_str)="<<strlen((char*)udp_send_str)<<" sizeof(udp_recv_str)="<<sizeof(udp_recv_str)<<" strlen(udp_recv_str)="<<strlen((char*)udp_recv_str)<<'\n';
	unsigned int recv_addr_len=sizeof(recv_addr);// unsigned char udp_recv_str[BUFFSZ];
	if((sock=socket(AF_INET,SOCK_DGRAM,0))<0){
		perror("\nsocket:"); return 0;
	}
	addr.sin_family=AF_INET;
	addr.sin_port=htons(7700);
	addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
	sendto(sock,cypher_str(udp_send_str),(strlen((char*)udp_send_str)+1),0,(struct sockaddr*)(&addr),sizeof(addr));
	recvfrom(sock,udp_recv_str,sizeof(udp_recv_str),0,(struct sockaddr*)(&recv_addr),&recv_addr_len);
	cout<<"\nrecv_addr_len="<<recv_addr_len<<" sizeof(recv_addr)="<<sizeof(recv_addr)<<" ntohs(recv_addr.sin_port)="<<ntohs(recv_addr.sin_port)<<" htonl(recv_addr.sin_addr.s_addr)="<<htonl(recv_addr.sin_addr.s_addr)<<" INADDR_LOOPBACK="<<INADDR_LOOPBACK<<'\n';
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
	crc_val recv_crc,calc_crc;
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

unsigned char* pass_input(unsigned char* send_str){
	for(usi j=0;j<PWSZ;j++){
		cin>>send_str[j];
		cout<<"\033[1A";
		for(usi k=0;k<=j;k++) cout<<'*';
	}
	send_str[PWSZ]='\0';
}

int main(){
	int sock,kdc_sock; struct sockaddr_in addr,recv_addr,kdc_addr; srand(time(NULL));
	if(!udp_rqst(sock,addr,recv_addr)){
		cout<<"\nudp_rqst\n"; return 1;
	}
	unsigned char dh_send_str[BUFFSZ],cdh_recv_str[BUFFSZ],dh_recv_str[BUFFSZ],send_str[BUFFSZ],recv_str[BUFFSZ],choice='0',choice1='0';
	bool crc_match=0;
	if((sock=socket(AF_INET,SOCK_STREAM,0))<0){
		perror("\nsocket:"); return 1;
	}
	if(connect(sock,(struct sockaddr*)(&addr),sizeof(addr))<0){
		perror("\nconnect:"); return 1;
	}
	usi send_dhv=dhg;
	for(usi i=0;i<dha-1;i++){
		send_dhv=(send_dhv*dhg)%dhp;
	}
	sprintf((char*)dh_send_str,"%d",send_dhv);
	send_c_crc(sock,dh_send_str);
	recv_c_crc(sock,cdh_recv_str,dh_recv_str,crc_match);
	if(crc_match){
		usi s_key=atoi((char*)dh_recv_str);
		for(usi i=0;i<dha-1;i++){
			s_key=(s_key*dhg)%dhp;
		}
		cout<<"\ns_key="<<s_key<<'\n';
	}
	else{
		cout<<"\ncrc!";
	}
	cout<<" admin: 0; user: 1; info: 2\n";
	//cin>>choice;
	sprintf((char*)send_str,"%c",choice);
	send(sock,cypher_str(send_str),(strlen((char*)send_str)+1),0);
	if(choice=='0'){
		/*
		unsigned char auth_send_str[BUFFSZ],c_auth_recv_str[BUFFSZ],dc_auth_recv_str[BUFFSZ],tmpstr[BUFFSZ];
		kdc_addr.sin_family=AF_INET;
		kdc_addr.sin_port=htons(7701);
		kdc_addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
		if((kdc_sock=socket(AF_INET,SOCK_STREAM,0))<0){
			perror("\nsocket:"); return 1;
		}
		if(connect(kdc_sock,(struct sockaddr*)(&kdc_addr),sizeof(kdc_addr))<0){
			perror("\nconnect:"); return 1;
		}
		unsigned char r1=rand()%9+'0',r2=r1,fr1,fr2,Ks;
		sprintf((char*)tmpstr,"%c%s",r1,"l2s");	sprintf((char*)auth_send_str,"%s%s","l2c",cypher_str(tmpstr));
		send(kdc_sock,auth_send_str,strlen((char*)auth_send_str)+1,0);
		recv(kdc_sock,c_auth_recv_str,sizeof(c_auth_recv_str),0);//EKA(f(r1), Ks, IdB, EKB(Ks, IdA)) -> EKB(Ks, IdA), EKs(r2)
		strcpy((char*)dc_auth_recv_str,(char*)decypher_str(c_auth_recv_str,strlen((char*)c_auth_recv_str)));
		fr1=dc_auth_recv_str[0]; Ks=dc_auth_recv_str[1];
		if(fr1==(r1+1)) cout<<"\npossibly KDC";
		else cout<<"\nfr1";
		sprintf((char*)auth_send_str,"%s%c",(dc_auth_recv_str+5),((r2+Ks)%256));
		send(sock,auth_send_str,(strlen((char*)auth_send_str)+1),0);
		recv(sock,recv_str,sizeof(recv_str),0);
		fr2=(recv_str[0]+(256-Ks))%256;
		cout<<"\ndc_auth_recv_str="<<dc_auth_recv_str<<" auth_send_str="<<auth_send_str<<" recv_str="<<recv_str<<" fr2="<<fr2<<" r2="<<r2;
		if(fr2==(r2+1)) cout<<"\npossibly l2s";
		else cout<<"\nfr2";
		close(kdc_sock);
		*/
		
		usi Ks=1;
		while(1){
			cout<<"\nHello, admin; password ["<<PWSZ<<" symbols]:\n";
			for(usi i=0;i<3;i++){
				pass_input(send_str); cout<<"\nsend_str="<<send_str;
				send(sock,cypher_str(send_str,Ks),(strlen((char*)send_str)+1),0); cout<<"\nsend_str="<<send_str;
				recv(sock,recv_str,sizeof(recv_str),0); cout<<"\nrecv_str="<<recv_str;
				if(strcmp("ack",(char*)decypher_str(recv_str,strlen((char*)recv_str),Ks))==0){
					cout<<"\nack="<<recv_str;
					break;
				}
				else{
					if(i==2){
						close(sock); return 0;
					}
					cout<<"\ni="<<i<<" password:\n";
				}
			}
			cout<<"\nch_password: 0; add_user: 1; blck_user: 2; close: 3\n";
			cin>>choice1;
			sprintf((char*)send_str,"%c",choice1);
			send(sock,cypher_str(send_str,Ks),(strlen((char*)send_str)+1),0);
			if(choice1=='0'){//change admin's password;
				cout<<"\ncurrent password:\n";
				pass_input(send_str); cout<<"\n0 send_str="<<send_str;
				send(sock,cypher_str(send_str,Ks),(strlen((char*)send_str)+1),0);//send current admin's password;
				cout<<"\n0 send_str="<<send_str;
				recv(sock,recv_str,sizeof(recv_str),0);//receive "ack";
				cout<<"\n0 recv_str="<<recv_str;
				if(strcmp("ack",(char*)decypher_str(recv_str,strlen((char*)recv_str),Ks))==0){
					unsigned char n_pass[PWSZ+1],c_n_pass[PWSZ+1];
					cout<<"\nenter new password:\n";
					pass_input(n_pass);
					cout<<"\nconfirm:\n";
					pass_input(c_n_pass);
					cout<<"\nn_pass="<<n_pass<<" c_n_pass="<<c_n_pass;
					if(strcmp((char*)n_pass,(char*)c_n_pass)==0){
						sprintf((char*)send_str,"%c",'0');
						send(sock,cypher_str(send_str,Ks),(strlen((char*)send_str)+1),0);
						sprintf((char*)send_str,"%s",n_pass);
						send(sock,cypher_str(send_str,Ks),(strlen((char*)send_str)+1),0);
						cout<<"\npassword match";
					}
					else{
						sprintf((char*)send_str,"%c",'1');
						send(sock,cypher_str(send_str,Ks),(strlen((char*)send_str)+1),0);
						cout<<"\npassword mismatch";
					}
				}
			}
			else if(choice1=='1'){//add new user;
				cout<<"\nn_user_login:\n"; cin>>send_str;
				send(sock,cypher_str(send_str,Ks),(strlen((char*)send_str)+1),0);
				recv(sock,recv_str,sizeof(recv_str),0);
				if(strcmp("ack",(char*)decypher_str(recv_str,strlen((char*)recv_str),Ks))==0){
					cout<<"\nn_user_password:\n"; cin>>send_str;
					send(sock,cypher_str(send_str,Ks),(strlen((char*)send_str)+1),0);
				}
				else cout<<"\nn_user_login";
			}
			else if(choice1=='2'){//block user;
				cout<<"\nu_login:\n"; cin>>send_str;
				send(sock,cypher_str(send_str,Ks),(strlen((char*)send_str)+1),0);
				recv(sock,recv_str,sizeof(recv_str),0);
				if(strcmp("ack",(char*)decypher_str(recv_str,strlen((char*)recv_str),Ks))==0){
					cout<<"\nack";
				}
				else{
					cout<<"\nu_login mismatch";
				}
			}
			else if(choice1=='3'){//exit;
				close(sock); return 0;
			}
		}
	}
	else if(choice=='1'){
		while(1){
			cout<<"\nn_user_login:\n"; cin>>send_str;
			send(sock,cypher_str(send_str),(strlen((char*)send_str)+1),0);
			recv(sock,recv_str,sizeof(recv_str),0); decypher_str(recv_str,strlen((char*)recv_str));
			unsigned char ack_str[4]='\0';
			if(strlen(recv_str)>2){
				strcpy((char*)ack_str,(char*)recv_str);
				ack_str[3]='\0';
			}
			if(strcmp("ack",(char*)ack_str))==0){//login finded;
				bool conf_fl=(recv_str[3]-'0');
				cout<<"\nn_user_password:\n";
				for(usi i=0;i<3;i++){
					pass_input(send_str);
					if(conf_fl){
						unsigned char conf_str[PWSZ+1];
						cout<<"\nconfirm:\n";
						pass_input(conf_str);
						if(strcmp((char*)send_str,(char*)conf_str)==0){//password confirmed;
							send(sock,cypher_str(send_str),(strlen((char*)send_str)+1),0);
							recv(sock,recv_str,sizeof(recv_str),0);
							if(strcmp("ack",(char*)decypher_str(recv_str,strlen((char*)recv_str))==0){
								break;
							}
							else{
								if(i==2){
									close(sock); return 0;
								}
								cout<<"\ni="<<i<<" password:\n";
							}
						}
						else{
							if(i==2){
								close(sock); return 0;
							}
							cout<<"\ni="<<i<<" password:\n";
						}
					}
					else{
						send(sock,cypher_str(send_str),(strlen((char*)send_str)+1),0);
						recv(sock,recv_str,sizeof(recv_str),0);
						if(strcmp("ack",(char*)decypher_str(recv_str,strlen((char*)recv_str))==0){
							break;
						}
						else{
							if(i==2){
								close(sock); return 0;
							}
							cout<<"\ni="<<i<<" password:\n";
						}
					}
				}
			}
			else{
				cout<<"\nn_user_login";
				break;
			}
			cout<<"\nch_password: 0; recv_q: 1; close: 3\n";
			cin>>choice1;
			if(choice1=='0'){//ch_password;
			
			}
			else if(choice1=='1'){//recv_q;
			
			}
			else{//close;
			
			}
		}
		
	}
	else{
	
	}
	
	
	
	
	close(sock);
	
	
	
	return 0;
}
