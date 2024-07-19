#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

/*
	This code for server.c will based on the given inputs to initialize the server, create a socket and bind that socket to the
	struct socket address which will NOT enter a listen state since this code is to represent UDP transfer which is different
	than the previous code due to it being TCP transfer. The client will send it's information necessary for the filename to
	the server which then the server will store the information and open the file and read from the file which then it will
	send data packets to the client and then recieve an ACK back from the client and continue in a loop till an EOF is reached
	then the server will print stats and close the file and socket. An added feature to server is to simulate packet loss which
	is part of an input value during initialization which will simulate a packet being "lost" thus will prompt the program to
	"retransmit" the packet, another additional function to this program is that another input value is "Timeout" which is to
	simulate a timeout function for values that are "lost" from the client ACK.
*/
typedef struct {//HEADER STRUCT
	uint16_t sizeNum; //COUNTER
	uint16_t packNum; //SEQUENCE NUMBER
	char data[80];
} UDPHeader;

typedef struct { //ACK HEADER STRUCT
	uint16_t ACKSeqNum; //ACK SEQ NUMBER
} ACKHeader;
//SIMULATE LOSS FUNCTION!
int SimulateLoss(double PacketLossRatio){
	float CompareValue = (float)rand()/RAND_MAX; //value between 0 and 1
	if(CompareValue < PacketLossRatio){
		return 1; //packet is lost
	}else{
		return 0; //packet is sent
	}
}
//MAIN
int main() {
	int port = 5100, MAXLINE = 80; //port and maxline values
	char text[MAXLINE];//text that is sent in as well as what will be used for fgets
	FILE *file;//file to be opened and read

	UDPHeader HeaderServer; //initialization of struct for header
	ACKHeader ACKServer; //initialization of struct for ACK header

	struct sockaddr_in serverAddr, clientAddr; //establish server sockaddr

	//packet sequence value
	int packetSeq;

	//create a socket for server and return -1 if fail
	int serversocketID = socket(PF_INET, SOCK_DGRAM, 0);//create the socket
	int ACKHeaderRcv;

	//INPUT LOOP FOR TIMEOUT VALUE
	int nMicroSecondValue = 11;
	while(nMicroSecondValue > 10 || nMicroSecondValue < 1){
		printf("Enter Timeout Value [1-10]: ");
		fgets(text, MAXLINE, stdin);
		int len = strlen(text);
		text[len - 1] = '\0';
		nMicroSecondValue = atoi(text);
		if(nMicroSecondValue >= 1 && nMicroSecondValue <= 10){
			break;
		}
		printf("Try again.(Timeout)\n");
	}
	int TimeoutMicroSeconds = (10^nMicroSecondValue);
	double TimeoutSeconds = TimeoutMicroSeconds/1000000;

	//INPUT LOOP FOR PACKET LOSS RATIO
	double PacketLossRatio = 2;
	while(PacketLossRatio < 0 || PacketLossRatio > 1){
		printf("Enter Packet Loss Ratio [0-1]: ");
		fgets(text, MAXLINE, stdin);
		int len2 = strlen(text);
		text[len2 - 1] = '\0';
		PacketLossRatio = atof(text);
		if(PacketLossRatio >= 0 && PacketLossRatio < 1){ //can be set to 0 but not 1 since essentially bricks and no packet is sent
			break;
		}
		printf("Try again. (PacketLossRatio)\n");
	}


	if(serversocketID == -1){//if the socket returns a -1 then there was an error
		printf("Socket error\n");
		exit(0);
	}


	//fill out the struct with the family, port, and address
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);


	//to bind the socket to the struct and will return -1 if failed
	int serverSocketBind = bind(serversocketID, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
	if(serverSocketBind == -1){
		printf("Bind error\n");
		exit(0);
	}


	//creates a socket and struct to be accepted and to accept the client
	socklen_t clientAddrLen = sizeof(clientAddr);

	//recieve filename from client
	int read_bytes = recvfrom(serversocketID, &HeaderServer, sizeof(HeaderServer), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
	if(read_bytes == -1){
		printf("Didnt receive filename\n");
		exit(0);
	}

	//grab first packet number
	packetSeq = ntohs(HeaderServer.packNum);

	//open filename sent from client
	file = fopen(HeaderServer.data, "r");
	if(file == NULL){
		printf("Couldnt open file\n");
		exit(0);
	}


	//Below are the necessary stat trackers for server
	int initPackets = 0; //initial packets transmission counter
	int initDataBytes = 0; //initial packets transmission data bytes counter
	int retransPackets = 0; //retransmitted packets counter
	int droppedPackets = 0; //packets that have been dropped counter
	int successPackets = 0; //packets that have been successfully sent counter
	int recievedACKS = 0; //amount of ACKS received
	int expiredTimeoutCount = 0; //amount timeout has occured

	//UDP TRANSMISSION HERE
	//time struct
	struct timeval tv;
	//while fgets is not null it continues the loop
	/*
		This is the meat of the program as well as the UDP transmission which will first determine if it can grab and read from the file and put that text
		into text and set all of the initialization process for the packet then enter another loop to determine if the packet has been sent since it is
		to determine if the packet has receieved and ack as well as being transmitted. The program will simulate loss as well as determine if the packet
		was retransmitted hence meaning that it didnt pass the second while loop as is still reading from the same text. The program will determine packet
		loss and if there is then state as such and rerun, however if not lost then enter another while loop to determine it being sent and recieved which
		the packet will be sent to client and this set the socket to a timeout value for receiveing since ACK could be lost and therefore leading to another
		timeout situation and if the timeout occurs set the proper variables to timeout and break yet if the ACK is received then set the proper variables
		and exit the loop which will exit the greater loop therefore heading to the next condition of the great loop. Thus the program repeats till the EOF
		condition is met and dealt with accordingly which sends a packet of count 0.
	*/
	while(fgets(text,MAXLINE,file) != NULL){
		//init values to determine if a packet has been sent, retrans, and recieved the ack
		int transmitted = 0;
		int retransmit = 0;
		int recvACK = 0;
		//if and else statements change the packet between 0 and 1
		if(packetSeq == 0){
			packetSeq = 1;
			HeaderServer.packNum = htons(packetSeq);
		}else{
			packetSeq = 0;
			HeaderServer.packNum = htons(packetSeq);
		}
		//converts data from fgets to the packet
		HeaderServer.sizeNum = htons(strlen(text));
		strcpy(HeaderServer.data,text);
		int count = strlen(text);
		//another loop to transmit data since it can be transmitted but not recieve the ack thus an or statement
		while(transmitted == 0 || recvACK == 0){
			//run simulation loss
			int SimLoss = SimulateLoss(PacketLossRatio);
			//check value if timeout has occured
			int timeoutCheck = 0;
			//check if packet has been retransmitted which prints out different outputs and adds to different values
			if(retransmit == 0){ //not retransmitted
				initPackets += 1;
				initDataBytes += count;
				printf("Packet %d generated for transmission with %d data bytes\n",packetSeq,count);
			}else if(retransmit == 1){ //retransmitted packet
				retransPackets += 1;
				printf("Packet %d generated for re-transmission with %d data bytes\n",packetSeq,count);
			}
			if(SimLoss == 0){//transmitted normally
				//transmission loop and must break if transmitted and timeout but must continue loop again if ack hasnt been reached
				while((transmitted == 0) && (timeoutCheck == 0) || (recvACK == 0)){
					//sends the generated packet to the client
					int rcvHeader = sendto(serversocketID, &HeaderServer, sizeof(HeaderServer), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
					printf("Packet %d successfully transmitted with %d data bytes\n",packetSeq,count);
					successPackets += 1;
					transmitted = 1;
					int rcvACK = 0;
					//sets the timeout values for the time struct
					tv.tv_sec = 0; //TimeoutSeconds
					tv.tv_usec = TimeoutMicroSeconds;
					//timeout which utilizes socket options for socket ID which SO_RCVTIMEO needs the time struct for a timeout value or that the recv is an error whcih the program will count as a timeout
					if((setsockopt(serversocketID, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) || (((ACKHeaderRcv = recvfrom(serversocketID, &ACKServer, sizeof(ACKServer), 0, (struct sockaddr *)&clientAddr, &clientAddrLen)) < 0))){
						timeoutCheck = 1;
						printf("Timeout expired for packet numbered %d\n",packetSeq);
						expiredTimeoutCount += 1;
						retransmit = true;
						break; //break from loop to retransmit packet
					}else{//packet ACK has been recieved
						ACKHeaderRcv = recvfrom(serversocketID, &ACKServer, sizeof(ACKServer), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
						printf("ACK %d received\n",ntohs(ACKServer.ACKSeqNum));
						recievedACKS += 1;//received ack counter
						recvACK = 1;
					}
				}
			}else if(SimLoss == 1){ //Simulation packet loss returns loss
				retransmit = 1; //packet has been retransmitted
				printf("Packet %d lost\n",packetSeq);
				droppedPackets += 1; //counter for dropped packets
				sleep(TimeoutSeconds);
				printf("Timeout expired for packet numbered %d\n",packetSeq);
				expiredTimeoutCount += 1;
			}
		}
	}
	//END OF FILE PACKET
	if(feof(file)){
		if(packetSeq == 0){
			packetSeq = 1;
			HeaderServer.packNum = htons(packetSeq);
		}else{
			packetSeq = 0;
			HeaderServer.packNum = htons(packetSeq);
		}
		HeaderServer.sizeNum = htons(0);
		//send end of file to client which ends the program
		int sendEOF = sendto(serversocketID, &HeaderServer, sizeof(HeaderServer), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
		printf("End of Transmission Packet with sequence number %d transmitted\n",packetSeq);
	}

	//UDP TRANSMISSION END
	//STATS
	/*
		ENTER FINAL STATS HERE!
		1. Number of data packets generated for transmission (initial transmission only)
		2. Total number of data bytes generated for transmission, initial transmission only (this should be the sum of the count fields of all packets generated for transmission for the first time only)
		3. Total number of data packets generated for retransmission (initial transmissions plus retransmissions)
		4. Number of data packets dropped due to loss
		5. Number of data packets transmitted successfully (initial transmissions plus retransmissions)
		6. Number of ACKs received
		7. Count of how many times timeout expired
	*/
	printf("-- SERVER END STATS --\n");
	printf("1. Number of data packets generated for transmission : %d\n",initPackets);
	printf("2. Total number of data bytes generated for transmission, initial transmission only : %d\n",initDataBytes);
	printf("3. Total number of data packets generated for retransmission : %d\n",retransPackets + initPackets);
	printf("4. Number of data packets dropped due to loss : %d\n",droppedPackets);
	printf("5. Number of data packets transmitted successfully : %d\n",successPackets);
	printf("6. Number of ACKs received : %d\n",recievedACKS);
	printf("7. Count of how many times timeout expired : %d\n",expiredTimeoutCount);
	//close the file and socket
	fclose(file);
	close(serversocketID);
	return 0;
}

