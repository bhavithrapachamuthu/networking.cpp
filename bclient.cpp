#include <stdio.h>
#include <WinSock2.h>
#include <cstring>
#pragma comment(lib,"ws2_32.lib")
#define PORT 8080
#define PACKET_EMPLOYEE_ID 1
#define PACKET_EMPLOYEE_NAME 2
#define PACKET_EMPLOYEE_SALARY 3
#define PACKET_CUSTOMER_ID 4
#define PACKET_CUSTOMER_NAME 5
#define PACKET_CUSTOMER_ADDRESS 6
#define PACKET_SALE_ID 7
#define PACKET_SALE_AMOUNT 8
#define PACKET_SALE_DATE 9
#define PACKET_RESULT 10
#define PACKET_VIEW_ALL 11
#define FRAME_SINGLE 0
#define FRAME_MORE 1
struct Packet
{
    unsigned char header;
    unsigned char data[100000];
};
class Data{
public:
    int id;
};
class Employee : public Data{
public:
    char empName[50];
    float salary;
};
class Customer : public Data{
public:
    char cusName[50];
    char address[100];
};
class Sale : public Data{
public:
    float amount;
    char date[20];
};
template<class T>
struct Node{
    T data;
    Node<T>* left;
    Node<T>* right;

    Node()
    {
        left = NULL;
        right = NULL;
    }
};
Node<Employee>* empRoot = NULL;
Node<Customer>* cusRoot = NULL;
Node<Sale>* saleRoot = NULL;
unsigned char makeHeader(int packetID,int length,int frameType){
    return ((packetID & 0x07) << 5) |((length & 0x0F) << 1) |(frameType & 0x01);
}
int getPacketID(unsigned char header){
    return (header >> 5) & 0x07;
}
int getLength(unsigned char header){
    return (header >> 1) & 0x0F;
}
int getFrameType(unsigned char header){
    return header & 0x01;
}
bool sendAll(SOCKET client,char* buffer,int length){
    int total = 0;
    while(total < length){
        int n = send(client,buffer + total,length - total,0);
        if(n <= 0)
            return false;
        total += n;
    }
    return true;
}
bool recvAll(SOCKET client,char* buffer,int length)
{
    int total = 0;
    while(total < length){
        int n = recv(client,buffer + total,length - total, 0);
        if(n <= 0)
            return false;
        total += n;
    }
    return true;
}
bool sendPacket(SOCKET client,int packetID,int length,int frameType,const void* data)
{
    Packet packet;
    packet.header =makeHeader(packetID,length,frameType);
    if(!sendAll(client,(char*)&packet.header,1)){
        return false;
    }
    if(length > 0){
        memcpy(packet.data,data,length);
        if(!sendAll(client,(char*)packet.data,length)){
            return false;
        }
    }
    return true;
}
bool receiveResult(SOCKET client){
    Packet packet;
    if(!recvAll(client,(char*)&packet.header,1)){
        return false;
    }
    int packetID =getPacketID(packet.header);
    int length =getLength(packet.header);
    int frameType =getFrameType(packet.header);
    if(length > 16)
        return false;
    if(length > 0){
        if(!recvAll(client,(char*)packet.data,length)){
            return false;
        }
    }
    packet.data[length] = '\0';
    printf("\nServer Response\n");
    printf("Packet ID  = %d\n", packetID);
    printf("Length     = %d\n", length);
    printf("Frame Type = %d\n", frameType);
    printf("Message    = %s\n",packet.data);
    return true;
}
void addEmployee(SOCKET client)
{
    int id;
    char name[50];
    float salary;
    printf("\nEnter Employee ID : ");
    scanf("%d", &id);
    getchar();
    printf("Enter Employee Name : ");
    fgets(name,sizeof(name),stdin);
    name[strcspn(name,"\n")]='\0';
    printf("Enter Employee Salary : ");
    scanf("%f", &salary);
    sendPacket(client,PACKET_EMPLOYEE_ID,sizeof(int),FRAME_MORE,&id);
    int namelen=strlen(name);
    sendPacket(client,PACKET_EMPLOYEE_NAME,namelen,FRAME_MORE,name);
    sendPacket(client,PACKET_EMPLOYEE_SALARY,sizeof(float),FRAME_SINGLE,&salary);
    receiveResult(client);
}
void addCustomer(SOCKET client){
    int id;
    char name[50];
    char address[100];
    printf("\nEnter Customer ID : ");
    scanf("%d", &id);
    getchar();
    printf("Enter Customer Name : ");
    fgets(name,sizeof(name),stdin);
    name[strcspn(name,"\n")]='\0';
    printf("Enter Customer Address : ");
    fgets(address,sizeof(address),stdin);
    address[strcspn(address,"\n")]='\0';
    sendPacket(client,PACKET_CUSTOMER_ID,sizeof(int),FRAME_MORE,&id);
    int namelen=strlen(name);
    sendPacket(client,PACKET_CUSTOMER_NAME,namelen,FRAME_MORE,name);
    int addresslen=strlen(address);
    sendPacket(client,PACKET_CUSTOMER_ADDRESS,addresslen,FRAME_MORE,address);
    receiveResult(client);
}
void addSale(SOCKET client){
    int id;
    float amount;
    char date[20];
    printf("\nEnter Sale ID : ");
    scanf("%d", &id);
    printf("Enter Sale Amount : ");
    scanf("%f", &amount);
    getchar();
    printf("Enter Sale Date : ");
    fgets(date,sizeof(date),stdin);
    date[strcspn(date,"\n")]='\0';
    sendPacket(client,PACKET_SALE_ID,sizeof(int),FRAME_MORE,&id);
    sendPacket(client,PACKET_SALE_AMOUNT,sizeof(float),FRAME_MORE,&amount);\
    int datelen=strlen(date);
    sendPacket(client,PACKET_SALE_DATE,datelen,FRAME_SINGLE,date);
    receiveResult(client);
}
void viewAll(SOCKET client)
{
    // View request
    sendPacket(client,PACKET_VIEW_ALL,0,FRAME_SINGLE,NULL);
    while(true)
    {
        Packet packet;
        if(!recvAll(client,(char*)&packet.header,1))
            return;
        int packetID =getPacketID(packet.header);
        int length =getLength(packet.header);
        int frameType =getFrameType(packet.header);
        if(length > 0){
            if(!recvAll(client,(char*)packet.data,length))
                return;
        }
        if(packetID == PACKET_RESULT){
            packet.data[length] = '\0';
            if(strcmp((char*)packet.data,"END") == 0){
                printf("\n--- VIEW END ---\n");
                break;
            }
            printf("\nServer : %s\n",packet.data);
            continue;
        }
        if(packetID == 1)
        {
            int type;
            memcpy(&type,packet.data,1);
            printf("EMPLOYEE\n");
            recvAll(client,(char*)&packet.header,1);
            length =getLength(packet.header);
            recvAll(client,(char*)packet.data,length);
            int id;
            memcpy(&id,packet.data,sizeof(int));
            printf("ID     : %d\n", id);
            char name[50] = "";
            while(true){
                recvAll(client,(char*)&packet.header,1);
                length =getLength(packet.header);
                frameType =getFrameType(packet.header);
                recvAll(client,(char*)packet.data,length);
                strncat(name,(char*)packet.data,length);
                if(frameType == FRAME_SINGLE)
                    break;
            }
            printf("Name   : %s\n", name);
            recvAll(client,(char*)&packet.header,1);
            length =getLength(packet.header);
            recvAll(client,(char*)packet.data,length);
            float salary;
            memcpy(&salary,packet.data,sizeof(float));
            printf("Salary : %.2f\n", salary);
        }
        else if(packetID == 2){
            printf("CUSTOMER\n");
            recvAll(client,(char*)&packet.header,1);
            length =getLength(packet.header);
            recvAll(client,(char*)packet.data,length);
            int id;
            memcpy(&id,packet.data,sizeof(int));
            printf("ID : %d\n", id);
            char name[50] = "";
            while(true){
                recvAll(client,(char*)&packet.header,1);
                length =getLength(packet.header);
                frameType =getFrameType(packet.header);
                recvAll(client,(char*)packet.data,length);
                strncat(name,(char*)packet.data,length);
                if(frameType == FRAME_SINGLE)
                    break;
            }
            printf("Name: %s\n", name);
            char address[100] = "";
            while(true){
                recvAll(client,(char*)&packet.header,1);
                length =getLength(packet.header);
                frameType =getFrameType(packet.header);
                recvAll(client,(char*)packet.data,length);
                strncat(address,(char*)packet.data,length);
                if(frameType == FRAME_SINGLE)
                    break;
            }
            printf("Address: %s\n", address);
        }
        else if(packetID == 3){
            printf("SALE\n");
            recvAll(client,(char*)&packet.header,1);
            length =getLength(packet.header);
            recvAll(client,(char*)packet.data,length);
            int id;
            memcpy(&id,packet.data,sizeof(int));
            printf("ID: %d\n", id);
            recvAll(client,(char*)&packet.header,1);
            length =getLength(packet.header);
            recvAll(client,(char*)packet.data,length);
            float amount;
            memcpy(&amount,packet.data,sizeof(float));
            printf("Amount: %.2f\n", amount);
            recvAll(client,(char*)&packet.header,1);
            length =getLength(packet.header);
            recvAll(client,(char*)packet.data,length);
            packet.data[length] = '\0';
            printf("Date: %s\n",packet.data);
        }
    }
}
int main(){
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);
    SOCKET client =socket(AF_INET,SOCK_STREAM,0);
    if(client == INVALID_SOCKET){
        printf("Socket creation failed\n");
        return 1;
    }
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port =htons(PORT);
    server.sin_addr.s_addr =inet_addr("127.0.0.1");
    if(connect(client,(sockaddr*)&server,sizeof(server)) == SOCKET_ERROR)
    {
        printf("Connection failed\n");
        return 1;
    }
    printf("Connected to Server\n");
    int choice;
    while(true)
    {
        printf("\n========================\n");
        printf("1. Add Employee\n");
        printf("2. Add Customer\n");
        printf("3. Add Sale\n");
        printf("4. View all\n");
        printf("5. Exit\n");
        printf("========================\n");
        printf("Enter Choice : ");
        scanf("%d", &choice);
        if(choice == 1){
            addEmployee(client);
        }
        else if(choice == 2){
            addCustomer(client);
        }
        else if(choice == 3){
            addSale(client);
        }
        else if(choice == 4){
            viewAll(client);
        }
        else if(choice == 5){
            break;
        }
        else{
            printf("Invalid Choice\n");
        }
    }
    closesocket(client);
    WSACleanup();
    return 0;
}