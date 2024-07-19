#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
/*

	This program for client.c will run its initialization stages which will prompt the user to input filename which will be sent to server as well as ask the
	user to enter ACK Loss ratio which will be used to simulate ACK Loss from client to server. The program follows the same procedure by creating the socket
	and filling out the server struct then directly transports the filename from client to server then run into the UDP transmission loop which takes any packet
	sent from server and processes the logic for writing in out.txt as well as sending an ACK and finally closing out.txt and socket.

*/
typedef struct{//HEADER STRUCT
	uint16_t sizeNum; //Count
	uint16_t packNum; //Packet Sequence Number
	char data[80]; //Packet text
} UDPHeader;

typedef struct{//ACK HEADER STRUCT
	uint16_t ACKSeqNum;//ACK SEQUENCE NUMBER
} ACKHeader;
//Simulate ACK Loss function!
int SimulateACKLoss(double ACKLossValue){
	float CompareValue = (float)rand()/RAND_MAX; //value between 0 and 1
	if(CompareValue < ACKLossValue){
		return 1; //lost
	}else{
		return 0; //sent
	}
}
//MAIN!
int main() {
	int port = 5100; //port number
	int MAXLINE = 80; //80 or byte per data packet
	char *serverIP = "127.0.0.1"; //localhost
	char text[MAXLINE]; //text to be written into out.txt
	char filename[MAXLINE]; //input file name
	char ACKLoss[MAXLINE]; //input ack loss ratio
	FILE *file; //file to be written into

	int rcvHeader;
	UDPHeader HeaderServer;//initialization of header struct
	ACKHeader ACKServer; //init of ack header
	struct sockaddr_in serverAddr;//struct for server socket addr


	int len = 0;
	double ACKLossValue = 2;
	//creates the socket for client and if returned -1 it failed
	int clientsocketID = socket(AF_INET, SOCK_DGRAM, 0);
	if(clientsocketID == -1){
		printf("Error creating socket\n");
		exit(0);
	}

	//creates and fills out the structure for the address of server
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);


	//grabs filename from initialization input
	printf("Enter file: ");
	fgets(filename, MAXLINE, stdin);
	len = strlen(filename);
	filename[len - 1] = '\0';

	//prompts the user to enter ACK Loss Ratio and must be between 0 and 1 and is a loop to make sure it is between 0 and 1 inclusive
	while(ACKLossValue < 0 || ACKLossValue > 1){
		printf("Enter Ack Loss Ratio [0 - 1]: ");
		fgets(ACKLoss, MAXLINE, stdin);
		int len2 = strlen(ACKLoss);
		ACKLoss[len-1] = '\0';
		ACKLossValue = atof(ACKLoss);
		if(ACKLossValue >= 0 && ACKLossValue < 1){
			break;
		}
		printf("Try again.\n");
	}

	//sends the filename to server
	HeaderServer.sizeNum = htonl(len);
	HeaderServer.packNum = htonl(0);
	strcpy(HeaderServer.data, filename);
	int clientSendFilename = sendto(clientsocketID, &HeaderServer, sizeof(HeaderServer), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if(clientSendFilename == -1){
		printf("Error");
		exit(0);
	}

	//opens a new file named "out.txt" and prompts that it can be written into returns null if there was an error
	file = fopen("out.txt","w");
	if(file == NULL){
		printf("error opening to write\n");
		exit(0);
	}

	//STATS TO TRACK
	int totalPackets = 0; //packets receieved successfully counter
	int dupePackets = 0; //counter for duplicate packets
	int totalPackets_NODUPES = 0; //total packet counter without dupes
	int TotalDataBytes_NODUPES = 0; //total bytes counter from nondupe packets
	int ACKSTrans_NOLOSS = 0; //ACKs sent without loss counter
	int DroppedACKS = 0; //counter for dropped acks
	int TotalACKSGenerated = 0; //counter for all acks generated

	//UDP TRANSMISSION START
	/*
		What the overall loop is processing first grabbing whatever is sent from server and determining if it is an EOF or any other packet then deal with it accordingly.
		The continuation of the loop will have it process if the recieved packet is a duplicate and whether or not it is will determine if it writes in out.txt
		Then the program will generate the ACK and fill its properties then simulate ack loss which will then determine if the ACK is sent or not thus is the loop.
	*/
	socklen_t serverAddrLen = sizeof(serverAddr);
	int prevSeqNum = 0; //previous sequence number tracker
	//loop is always waiting to receieve packet from server as long as it is greater than or equal to 4 bytes which is standard takes into account EOF
	while((rcvHeader = recvfrom(clientsocketID, &HeaderServer, sizeof(HeaderServer),0,(struct sockaddr *)&serverAddr,&serverAddrLen)) >= 4){
		bool retrans = false; //retransmitted packet boolean set to false
		int currPackNum = ntohs(HeaderServer.packNum); //transfers packet data to local variables
		int currSize = ntohs(HeaderServer.sizeNum);
		if(currSize == 0){ //if the value of count is 0 means EOF
			printf("End of Transmission Packet with sequence number %d received\n",currPackNum);
			break;
		}else{ //anything else received
			//copy data and add to total packets
			totalPackets += 1;
			strcpy(text, HeaderServer.data);
			if(currPackNum == prevSeqNum){ //if the current sequence number is the same as previous then means that the packet was recieved twice or retransmitted from server
				retrans = true;
			}
			if(retrans == false){ //if retrans is false then add to counters and write in out.txt
				totalPackets_NODUPES += 1;
				printf("Packet %d received with %d data bytes\n",currPackNum,currSize);
				fwrite(text, 1, currSize, file);
				TotalDataBytes_NODUPES += currSize;
				printf("Packet %d delivered to user\n",currPackNum);
			}else if(retrans == true){ //if retrans is true simply add to respective trackers and state it is a dupe thus dont write in out.txt
				dupePackets += 1;
				printf("Duplicate packet %d received with %d data bytes\n",currPackNum,currSize);
			}
			//set ACK Sequence number to packet sequence number and add to acks generated
			ACKServer.ACKSeqNum = htons(currPackNum);
			TotalACKSGenerated += 1;
			//set previous to current
			prevSeqNum = currPackNum;
			printf("ACK %d generated for transmission\n", currPackNum);
			//simulate loss
			int SimACKLoss = SimulateACKLoss(ACKLossValue);
			if(SimACKLoss == 1){ //ACK lost from Simulate ACK Loss and add to tracker
				printf("ACK %d lost\n",currPackNum);
				DroppedACKS += 1;
			}else if(SimACKLoss == 0){ //ACK sent correctly
				//send ACK to server and add to counter
				int ACKSend = sendto(clientsocketID, &ACKServer, sizeof(ACKServer), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
				printf("ACK %d successfully transmitted\n",currPackNum);
				ACKSTrans_NOLOSS += 1;
			}
		}
	}

	//UDP TRANSMISSION END

	//this last section rerts the statistics and closes the file and socket
	/*
		ENTER FINAL STATS HERE!
		1. Total number of data packets received successfully
		2. Number of duplicate data packets received)
		3. Number of data packets received successfully, not including duplicates
		4. Total number of data bytes received which are delivered to user (this should be the sum of the count fields of all received packets not including duplicates)
		5. Number of ACKs transmitted without loss
		6. Number of ACKs generated but dropped due to loss
		7. Total number of ACKs generated (with and without loss)
	*/
	printf("-- CLIENT END STATS --\n");
	printf("1. Total number of data packets received successfully : %d\n",totalPackets);
	printf("2. Number of duplicate data packets received : %d\n",dupePackets);
	printf("3. Number of data packets received successfully, not including duplicates : %d\n",totalPackets_NODUPES);
	printf("4. Total number of data bytes received which are delivered to user : %d\n",TotalDataBytes_NODUPES);
	printf("5. Number of ACKs transmitted without loss : %d\n",ACKSTrans_NOLOSS);
	printf("6. Number of ACKs generated but dropped due to loss : %d\n",DroppedACKS);
	printf("7. Total number of ACKs generated : %d\n",TotalACKSGenerated);
	//close file and socket then end
	fclose(file);
	close(clientsocketID);
	return 0;
}
