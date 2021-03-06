#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netpacket/packet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include "analyze.h"

int init_raw_socket(char *device,int promiscFlag,int ipOnly);
char *my_ether_ntoa_r(u_char *hwaddr,char *buf,socklen_t size);
int print_ether_header(struct ether_header *eh,FILE *fp);

int main(int argc,char *argv[],char *envp[]){
	int soc,size;
	u_char buf[65535];
	if(argc<=1){
		fprintf(stderr,"Itest device-name\n");
		return 1;
	}
	if((soc=init_raw_socket(argv[1],0,0))==-1){
		fprintf(stderr,"init_raw_socket:error:%s\n",argv[1]);
		return -1;
	}
	while(1){
		if((size=read(soc,buf,sizeof(buf)))<=0){
			perror("read error");
		}
		else{
			analyze_packet(buf,size);
		}
	}
	close(soc);
	return 0;
}

int init_raw_socket(char *device,int promiscFlag,int ipOnly){
	struct ifreq ifreq;
	struct sockaddr_ll sa;
	int soc;
	
	//get file_descriptor
	if(ipOnly){
		if((soc=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL)))<0){
			perror("socket error");
			return -1;	
		}
	}
	else{
		if((soc=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL)))<0){
			perror("socket error");
			return -1;
		}
	}

	memset(&ifreq,0,sizeof(struct ifreq));
	strncpy(ifreq.ifr_name,device,sizeof(ifreq.ifr_name)-1);
	if(ioctl(soc,SIOCGIFINDEX,&ifreq)<0){
		perror("ioctl error");
		close(soc);
		return -1;
	}

	//set Inter_Face_IDNEX,family and protocol to sockaddr_ll
	//next, set sockaddr_ll to soc
	sa.sll_family=PF_PACKET;
	if(ipOnly){
		sa.sll_protocol=htons(ETH_P_IP);
	}
	else{
		sa.sll_protocol=htons(ETH_P_ALL);
	}
	sa.sll_ifindex=ifreq.ifr_ifindex;
	if(bind(soc,(struct sockaddr *)&sa,sizeof(sa))<0){
		perror("bind error");
		close(soc);
		return -1;
	}
	
	//enable mode of promiscuous
	if(promiscFlag){
		if(ioctl(soc,SIOCGIFFLAGS,&ifreq)<0){
			perror("ioctl error2");
			close(soc);
			return -1;
		}
		ifreq.ifr_flags=ifreq.ifr_flags|IFF_PROMISC;
		if(ioctl(soc,SIOCSIFFLAGS,&ifreq)<0){
			perror("ioctl error3");
			close(soc);
			return -1;
		}
	}
	return(soc);
}
