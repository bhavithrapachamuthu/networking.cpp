#include<stdio.h>
#include<WinSock2.h>
#include<Windows.h>
#include<cstring>
#include<thread>
#include<mutex>
using namespace std;
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
#define PACKET_SALE_EMP_ID 10
#define PACKET_SALE_CUS_ID 11
#define PACKET_RESULT 12
#define FRAME_SINGLE 0
#define FRAME_MORE 1
struct Packet
{
    unsigned char header;
    unsigned char length;
    unsigned char data[255];
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
    char date[26];
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
int nextSaleId=1;
unsigned char makeHeader(int packetID,int frameType){
    return ((packetID & 0x0F) << 1) |(frameType & 0x01);
}
int getPacketID(unsigned char header){
    return (header >> 1) & 0x0F;
}
int getFrameType(unsigned char header){
    return header & 0x01;
}
template<class T>
Node<T>* find(Node<T>* root, int id){
    while(root != NULL)
    {
        if(id == root->data.id)
            return root;

        if(id < root->data.id)
            root = root->left;
        else
            root = root->right;
    }
    return NULL;
}
bool sendAll(SOCKET client,const char*buffer,int length){
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
    if(length<0 || length>255){
        return false;
    }
    unsigned char header =makeHeader(packetID,frameType);
    unsigned char len=(unsigned char)length;
    if(!sendAll(client,(char*)&header,1)){
        return false;
    }
    if(!sendAll(client,(char*)&len,1)){
        return false;
    }
    if(length > 0){
        if(!sendAll(client,(const char*)data,length)){
            return false;
        }
    }
    return true;
}
bool receivePacket(SOCKET client,Packet& packet){
    if(!recvAll(client,(char*)&packet.header,1))
    {
        return false;
    }
    if(!recvAll(client,(char*)&packet.length,1)){
        return false;
    }
    if(packet.length > 0){
        if(!recvAll(client,(char*)packet.data,packet.length)){
            return false;
        }
    }
    return true;
}
bool receiveResult(SOCKET client){
    Packet packet;
    if(!receivePacket(client,packet)){
        return false;
    }
    int packetID =getPacketID(packet.header);
    if(packetID!=PACKET_RESULT){
        return false;
    }
    char result[256];
    int len=packet.length;
    if(len>255)
    len=255;
    memcpy(result,packet.data,len);
    result[packet.length]='\0';
    printf("\nServer: %s\n",result);
    return true;
}
void addEmployee(SOCKET client){
    int id;
    char name[50];
    float salary;
    while(1){
        printf("\nEnter Employee ID : ");
        if(scanf("%d", &id) != 1)
        {
            printf("Invalid ID. Enter only numbers.\n");
            while(getchar() != '\n');
            continue;
        }
        sendPacket(client,PACKET_EMPLOYEE_ID,sizeof(int),FRAME_MORE,&id);
        Packet p;
        if(!receivePacket(client, p))
        {
            printf("Server disconnected\n");
            return;
        }
        p.data[p.length] = '\0';
        if(strcmp((char*)p.data, "DUPLICATE ID") == 0){
            printf("\nEmployee ID already exists!Try different Id.\n");
            continue;
        }
        if(strcmp((char*)p.data, "ID AVAILABLE") == 0){
            break;
        }
        printf("Server: %s\n", p.data);
        return;
    }
    while(getchar() != '\n');
    printf("Enter Employee Name : ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    sendPacket(client,PACKET_EMPLOYEE_NAME,strlen(name),FRAME_MORE,name);
    while(1){
        printf("Enter Employee Salary : ");
        if(scanf("%f", &salary) != 1){
            printf("Invalid Salary. Enter numbers only.\n");
            while(getchar() != '\n');
            continue;
        }

        break;
    }
    sendPacket(client,PACKET_EMPLOYEE_SALARY,sizeof(float),FRAME_SINGLE,&salary);
    Packet result;
    if(!receivePacket(client, result)){
        printf("Server disconnected\n");
        return;
    }
    result.data[result.length] = '\0';
    if(strcmp((char*)result.data, "EMPLOYEE ADDED") == 0){
        printf("\nEmployee added successfully\n");
    }
    else{
        printf("\nServer: %s\n", result.data);
    }
}
void addCustomer(SOCKET client)
{
    int id;
    char name[50];
    char address[100];
    while(1){
        printf("\nEnter Customer ID : ");
        if(scanf("%d", &id) != 1){
            printf("Invalid ID. Enter only numbers.\n");
            while(getchar() != '\n');
            continue;
        }
        sendPacket(client,PACKET_CUSTOMER_ID,sizeof(int),FRAME_MORE,&id);
        Packet p;
        if(!receivePacket(client, p)){
            printf("Server disconnected\n");
            return;
        }
        p.data[p.length] = '\0';
        if(strcmp((char*)p.data, "DUPLICATE ID") == 0){
            printf("\nCustomer ID already exists!Try different Id.\n");
            continue;
        }
        if(strcmp((char*)p.data, "ID AVAILABLE") == 0){
            break;
        }
        printf("Server: %s\n", p.data);
        return;
    }
    while(getchar() != '\n');
    printf("Enter Customer Name : ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    sendPacket(client,PACKET_CUSTOMER_NAME,strlen(name),FRAME_MORE,name);
    printf("Enter Customer Address : ");
    fgets(address, sizeof(address), stdin);
    address[strcspn(address, "\n")] = '\0';
    sendPacket(client,PACKET_CUSTOMER_ADDRESS,strlen(address),FRAME_SINGLE,address);
    Packet result;
    if(!receivePacket(client, result)){
        printf("Server disconnected\n");
        return;
    }
    result.data[result.length] = '\0';
    if(strcmp((char*)result.data, "CUSTOMER ADDED") == 0){
        printf("\nCustomer added successfully\n");
    }
    else{
        printf("\nServer: %s\n", result.data);
    }
}
void addSale(SOCKET client){
    int saleId;
    int empId;
    int cusId;
    float amount;
    char date[26];
    if(!sendPacket(client,PACKET_SALE_ID,0,FRAME_MORE,NULL)){
        printf("Failed to request Sale Id\n");
        return;
    }
    Packet p;
    if(!receivePacket(client,p)){
        printf("Server disconnected\n");
        return;
    }
    if(getPacketID(p.header)!=PACKET_SALE_ID || p.length!=sizeof(int)){
        printf("Invalid Sale Id response\n");
        return;
    }
    memcpy(&saleId,p.data,sizeof(int));
    printf("\nGenerated Sale Id: %d\n",saleId);
    while(1){
        printf("Enter Employee ID : ");
        if(scanf("%d", &empId) != 1){
            printf("Invalid Employee ID. Enter only numbers.\n");
            while(getchar() != '\n');
            continue;
        }
        if(!sendPacket(client,PACKET_SALE_EMP_ID,sizeof(int),FRAME_MORE,&empId)){
            printf("Server disconnected\n");
            return;
        }
        if(!receivePacket(client,p)){
            printf("Server disconnected\n");
            return;
        }
        p.data[p.length]='\0';
        if(strcmp((char*)p.data,"EMPLOYEE FOUND")==0){
            printf("Employee ID valid\n");
            break;
        }
        printf("Employee Id doesn't exist\n");
    }
    while(1){
        printf("Enter Customer ID : ");
        if(scanf("%d", &cusId) != 1){
            printf("Invalid Customer ID. Enter only numbers.\n");
            while(getchar() != '\n');
            continue;
        }
        if(!sendPacket(client,PACKET_SALE_CUS_ID,sizeof(int),FRAME_MORE,&cusId)){
            printf("Server disconnected\n");
            return;
        }
        if(!receivePacket(client,p)){
            printf("Server disconnected\n");
            return;
        }
        if(strcmp((char*)p.data,"CUSTOMER FOUND")==0){
            printf("Customer ID valid\n");
            break;
        }
        printf("Customer Id doesn't exist\n");
    }
    while(1){
        printf("Enter Sale Amount : ");
        if(scanf("%f", &amount) != 1){
            printf("Invalid Amount. Enter numbers only.\n");
            while(getchar() != '\n');
            continue;
        }
        break;
    }
    sendPacket(client,PACKET_SALE_AMOUNT,sizeof(float),FRAME_MORE,&amount);
    while(getchar() != '\n');
    time_t now;
    time(&now);
    strcpy(date, ctime(&now));
    date[strcspn(date, "\n")] = '\0';
    printf("Sale Date : %s\n", date);
    sendPacket(client,PACKET_SALE_DATE,strlen(date),FRAME_SINGLE,date);
    Packet result;
    if(!receivePacket(client, result))
    {
        printf("Server disconnected\n");
        return;
    }
    result.data[result.length] = '\0';
    if(strcmp((char*)result.data, "SALE ADDED") == 0){
        printf("\nSale added successfully\n");
    }
    else{
        printf("\nServer: %s\n", result.data);
    }
}
int main(){
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){
        printf("WSAStartup failed\n");
        return 1;
    }
    int noofClients;
    while(true){
        printf("\nEnter no.of clients: ");
        if(scanf("%d",&noofClients)!=1){
            printf("Enter numbers only\n");
            while(getchar()!='\n');
            continue;
        }
        if(noofClients<=0 || noofClients>10){
            printf("Enter clients 1 to 10\n");
            continue;
        }
        break;
    }
    SOCKET*client=new SOCKET[noofClients];
    for(int i=0;i<noofClients;i++){
        client[i]=socket(AF_INET,SOCK_STREAM,0);
        if(client[i]==INVALID_SOCKET){
            printf("\nClient %d socket creation failed\n",i+1);
            continue;
        }
        sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port =htons(PORT);
        server.sin_addr.s_addr =inet_addr("127.0.0.1");
        if(connect(client[i],(sockaddr*)&server,sizeof(server)) == SOCKET_ERROR){
            printf("\nClient %d connection failed\n",i);
            closesocket(client[i]);
            client[i]=INVALID_SOCKET;
            continue;
        }
        printf("\n=====================================\n");
        printf("  Client %d connected to Server\n",i+1);
        printf("\n=====================================\n");
    }
    printf("\n=====================================\n");
    printf("    All Client connected to Server\n");
    printf("\n=====================================\n");
    int choice;
    int count;
    int index;
    int clientNum;
    do{
        printf("\n========================\n");
        printf("1. Add Employee\n");
        printf("2. Add Customer\n");
        printf("3. Add Sale\n");
        printf("4. View all\n");
        printf("5. Search Employee\n");
        printf("6. Delete Employee\n");
        printf("7. Delete Customer\n");
        printf("8. Delete Sale\n");
        printf("9. Exit\n");
        printf("========================\n");
        printf("Enter Choice : ");
        if(scanf("%d", &choice)!=1){
            printf("\nEnter numbers only\n");
            while (getchar()!='\n');
            continue;
        }
        while(true){
            printf("\nEnter Client number (1-%d): ",noofClients);
            if(scanf("%d",&clientNum)!=1){
                printf("\nEnter numbers only\n");
                while (getchar()!='\n');
                continue;
            }
            break;
        }
        index=clientNum -1;
        if(client[index]==INVALID_SOCKET){
            printf("\nClient %d is not connected\n",clientNum);
            continue;
        }
        if(choice == 1){
            printf("\nEnter no.of Employess :");
            if(scanf("%d",&count)!=1){
                printf("\nEnter numbers only\n");
                while (getchar()!='\n');
                continue;
            }
            for(int i=0;i<count;i++){
                printf("\n=========EMPLOYEE %d===========\n",i+1);
                addEmployee(client[index]);
            }
        }
        else if(choice == 2){
            printf("\nEnter no.of Customers :");
            if(scanf("%d",&count)!=1){
                printf("\nEnter numbers only\n");
                while (getchar()!='\n');
                continue;
            }
            for(int i=0;i<count;i++){
                printf("\n=========CUSTOMER %d===========\n",i+1);
                addCustomer(client[index]);
            }
        }
        else if(choice == 3){
            printf("\nEnter no.of Sales :");
            if(scanf("%d",&count)!=1){
                printf("\nEnter numbers only\n");
                while (getchar()!='\n')
                continue;
            }
            for(int i=0;i<count;i++){
                printf("\n=========SALE %d===========\n",i+1);
                addSale(client[index]);
            }
        }
        else if(choice == 4){
            printf("\nExiting\n");
        }
        else{
            printf("Invalid Choice\n");
        }
    }while(choice!=4);
    for(int i=0;i<noofClients;i++){\
        if(client[i]!=INVALID_SOCKET){
            closesocket(client[i]);
        }
    }
    delete[] client;
    WSACleanup();
    printf("\nAll clients finished\n");
    return 0;
}
