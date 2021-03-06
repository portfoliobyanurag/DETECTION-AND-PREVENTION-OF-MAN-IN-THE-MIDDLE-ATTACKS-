#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <string.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

char gtwy[128];
char gateway_ip[128];

void rearpGateway(char* gateway_mac, char* gateway_ip){
  
  
  FILE *fp;
  char path[1035];
  int exists = 0;
  char command[1000];
  snprintf(command, sizeof(command), "arp -s %s %s", gateway_ip, gateway_mac);
  fp = popen(command, "r");
  if (fp == NULL) {
    printf("Failed to block packets\n" );
    exit(1);
  }
  printf("%sREARPED %s TO %s%s\n", KGRN, gateway_ip, gateway_mac, KWHT);
  
}

void blockARPPackets(char* mac_address){
  FILE *fp;
  char path[1035];
  int exists = 0;
  char command[1000];
  snprintf(command, sizeof(command), "arptables -A INPUT --source-mac %s -j DROP", mac_address);
  fp = popen(command, "r");
  if (fp == NULL) {
    printf("Failed to block packets\n" );
    exit(1);
  }
  printf("%sBLOCKING PACKETS FROM %s%s\n", KGRN, mac_address, KWHT);
}

void my_callback(u_char *args, const struct pcap_pkthdr* pkthdr, const u_char*
    packet)
{
    int i=0;
    static int count=0;
    
    char p_source[128] = "";
    char arp_source[128] = "";
    char arp_ip_source[128] = "";
    
    for(i=0;i<pkthdr->len;i++) {
        
        if(i >= 6 && i<= 11){
          
          char j[8];
          snprintf(j, sizeof(j), "%02x", packet[i]);
          strcat(p_source, j);
          if (i != 11)
          strcat(p_source, ":");
        }
        
        if(i >= 22 && i<= 27){
         
          char f[8];
          snprintf(f, sizeof(f), "%02x", packet[i]);
          strcat(arp_source, f);
          if(i != 27)
          strcat(arp_source, ":");
        }
        
        if(i >= 28 && i<= 31){
          
          char h[8];
          snprintf(h, sizeof(h), "%d", packet[i]);
          strcat(arp_ip_source, h);
          if(i != 31)
          strcat(arp_ip_source, ".");
        }
    }   

      char attacker_ip[128] = "";
            if(strncmp(arp_source, gtwy, 20) != 0){
              //Check if ARP Source IP is GATEWAY
              if(strncmp(arp_ip_source, gateway_ip, 20) == 0){
                //MALICIOUS ARP PACKET!
                printf("\x1B[31m MALICIOUS ARP PACKET DETECTED FROM %s.\x1B[37m\n", p_source);
                blockARPPackets(arp_source);
                rearpGateway(gtwy, gateway_ip);
              }
            }
          }


int sniffARPPackets(char* gateway, char* gateway_ipp)
{

    strncpy(gtwy, gateway, 17);
    strncpy(gateway_ip, gateway_ipp, 15);
    int i;
    char *dev;
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* descr;
    const u_char *packet;
    struct pcap_pkthdr hdr;
    struct ether_header *eptr;
    struct bpf_program fp;
    bpf_u_int32 maskp;
    bpf_u_int32 netp;
  
  

    
    pcap_if_t *alldevsp , *device;
	pcap_t *handle; 
  

	char  *devname , devs[100][100];
	int count = 1 , n;
	
	
	printf("Finding available devices ... ");
	if( pcap_findalldevs( &alldevsp , errbuf) )
	{
		printf("Error finding devices : %s" , errbuf);
		exit(1);
	}
	printf("Done");
	
	
	printf("\nAvailable Devices are :\n");
	for(device = alldevsp ; device != NULL ; device = device->next)
	{
		printf("%d. %s - %s\n" , count , device->name , device->description);
		if(device->name != NULL)
		{
			strcpy(devs[count] , device->name);
		}
		count++;
	}
	
	
	printf("Enter the number of the device you want to sniff : ");
	scanf("%d" , &n);
	devname = devs[n];
	 pcap_lookupnet(devname, &netp, &maskp, errbuf);
	
	printf("Opening device %s for sniffing ... " , devname);
	handle = pcap_open_live(devname , BUFSIZ, 1 , -1, errbuf);
	
	if (handle == NULL) 
	{
		fprintf(stderr, "Couldn't open device %s : %s\n" , devname , errbuf);
		exit(1);
	}
	printf("Done\n");
	 if(pcap_compile(handle, &fp, "arp", 0, netp) == -1) {
        fprintf(stderr, "Error calling pcap_compile\n");
        exit(1);
    }


    if(pcap_setfilter(handle, &fp) == -1) {
        fprintf(stderr, "Error setting filter\n");
        exit(1);
    }

	
	
	
	pcap_loop(handle , -1 , my_callback, NULL);
	
	return 0;	
}