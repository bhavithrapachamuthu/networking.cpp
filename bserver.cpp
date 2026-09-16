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
#define FRAME_SINGLE 0
#define FRAME_MORE 1
struct Packet{
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
template<class T>
Node<T>* insert(Node<T>*& root, T data){
    Node<T>*newNode=new Node<T>;
    newNode->data=data;
    if(root==NULL)
    {
        root=newNode;
        return newNode;
    }
    Node<T>* temp=root;
    while(true)
    {
        if(data.id < temp->data.id)
        {
            if(temp->left==NULL)
            {
                temp->left=newNode;
                return newNode;
            }
            temp=temp->left;
        }
        else
        {
            if(temp->right==NULL)
            {
                temp->right = newNode;
                return newNode;
            }
            temp = temp->right;
        }
    }
}
bool sendAll(SOCKET sock,char*buffer,int length){
    int total=0;
    while(total < length)
    {
        int n=send(sock,buffer+total,length-total,0);
        if(n <= 0)
            return false;
        total+=n;
    }
    return true;
}
bool recvAll(SOCKET sock,char* buffer,int length){
    int total = 0;
    while(total < length)
    {
        int n=recv(sock,buffer+total,length-total,0);
        if(n <= 0)
            return false;
        total+=n;
    }
    return true;
}
bool receivePacket(SOCKET client,Packet& packet){
    if(!recvAll(client,(char*)&packet.header,1))
    {
        return false;
    }
    int length = getLength(packet.header);
    if(length > 15)
        return false;
    if(length > 0){
        if(!recvAll(client,(char*)packet.data,length)){
            return false;
        }
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
void sendResult(SOCKET client,const char* msg){
    int length = strlen(msg);
    if(length > 15)
    length = 15;
    Packet packet;
    unsigned char header =makeHeader(PACKET_RESULT,length,FRAME_SINGLE);
    send(client,(char*)&header,sizeof(header),0);
    send(client,msg,length,0);
}
template<typename T>
void savefile(T&data,unsigned char type){
    FILE*fp=fopen("bhavi.bin","ab");
    if(fp==NULL){
        printf("File open failed\n");
        return;
    }
    fwrite(&type,sizeof(type),1,fp);
    fwrite(&data,sizeof(T),1,fp);
    fclose(fp);
    printf("Data saved to bhavi.bin\n");
}
void sendViewAll(SOCKET client)
{
    FILE* fp = fopen("bhavi.bin", "rb");
    if(fp == NULL){
        sendResult(client, "NOFILE");
        return;
    }
    unsigned char type;
    while(fread(&type,sizeof(type),1,fp) == 1){
        if(type == 1){
            Employee emp;
            if(fread(&emp,sizeof(emp),1,fp) != 1)
                break;
            sendPacket(client,1,1,FRAME_MORE,&type);
            sendPacket(client,PACKET_EMPLOYEE_ID,sizeof(int),FRAME_MORE,&emp.id);
            int nameLength = strlen(emp.empName);
            sendPacket(client,PACKET_EMPLOYEE_NAME,nameLength,FRAME_SINGLE,emp.empName);
            sendPacket(client,PACKET_EMPLOYEE_SALARY,sizeof(float),FRAME_SINGLE,&emp.salary);
        }
        else if(type == 2){
            Customer cus;
            if(fread(&cus,sizeof(cus),1,fp) != 1)
                break;
            sendPacket(client,2,1,FRAME_MORE,&type);
            sendPacket(client,PACKET_CUSTOMER_ID,sizeof(int),FRAME_MORE,&cus.id);
            int nameLen = strlen(cus.cusName);
            sendPacket(client,PACKET_CUSTOMER_NAME,nameLen,FRAME_MORE,cus.cusName);
            int addressLength = strlen(cus.address);
            sendPacket(client,PACKET_CUSTOMER_ADDRESS,addressLength,FRAME_SINGLE,cus.address);
        }
        else if(type == 3){
            Sale sale;
            if(fread(&sale,sizeof(sale),1,fp) != 1)
                break;
            sendPacket(client,3,1,FRAME_MORE,&type);
            sendPacket(client,PACKET_SALE_ID,sizeof(int),FRAME_MORE,&sale.id);
            sendPacket(client,PACKET_SALE_AMOUNT,sizeof(float),FRAME_MORE,&sale.amount);
            int dateLength = strlen(sale.date);
            sendPacket(client,PACKET_SALE_DATE,dateLength,FRAME_SINGLE,sale.date);
        }
    }
    fclose(fp);
    sendResult(client, "END");
}
void handleAdd(SOCKET client)
{
    Employee emp;
    Customer cus;
    Sale sale;
    memset(&emp, 0, sizeof(emp));
    memset(&cus, 0, sizeof(cus));
    memset(&sale, 0, sizeof(sale));
    bool empID = false;
    bool empName = false;
    bool empSalary = false;
    bool cusID = false;
    bool cusName = false;
    bool cusAddress = false;
    bool saleID = false;
    bool saleAmount=false;
    bool saleDate=false;
    while(true){
        Packet packet;
        if(!receivePacket(client, packet)){
            printf("Client disconnected.\n");
            return;
        }
        int packetID = getPacketID(packet.header);
        int length = getLength(packet.header);
        int frameType = getFrameType(packet.header);
        printf("\nReceived Packet");
        printf("\nPacket ID = %d", packetID);
        printf("\nLength = %d", length);
        printf("\nFrame Type = %d\n", frameType);
        if(packetID == PACKET_EMPLOYEE_ID)
        {
            if(length != sizeof(int))
            {
                sendResult(client, "BAD ID");
                continue;
            }
            memcpy(&emp.id,packet.data,sizeof(int));
            empID = true;
            printf("Employee ID = %d\n",emp.id);
        }
        else if (packetID==PACKET_EMPLOYEE_NAME){
            if(length>=sizeof(emp.empName)){
                sendResult(client,"BAD NAME");
                continue;
            }
            memcpy(emp.empName,packet.data,length);
            emp.empName[length]='\0';
            empName=true;
            printf("Employee Name: %s\n",emp.empName);
        }
        else if(packetID == PACKET_EMPLOYEE_SALARY){
            if(length != sizeof(float)){
                sendResult(client, "BAD SALARY");
                continue;
            }
            memcpy(&emp.salary,packet.data,sizeof(float));
            empSalary = true;
            printf("Employee Salary = %.2f\n",emp.salary);
        }
        else if(packetID == PACKET_CUSTOMER_ID){
            if(length != sizeof(int)){
                sendResult(client, "BAD CUSTOMER ID");
                continue;
            }
            memcpy(&cus.id,packet.data,sizeof(int));
            cusID = true;
            printf("Customer ID = %d\n",cus.id);
        }
        else if(packetID == PACKET_CUSTOMER_NAME){
            if(length >= sizeof(cus.cusName)){
                sendResult(client, "BAD CUSTOMER NAME");
                continue;
            }
            memcpy(cus.cusName,packet.data,length);
            cus.cusName[length] = '\0';
            cusName = true;
            printf("Customer Name = %s\n",cus.cusName);
        }
        else if(packetID == PACKET_CUSTOMER_ADDRESS){
            if(length >= sizeof(cus.address)){
                sendResult(client, "BAD ADDRESS");
                continue;
            }
            memcpy(cus.address,packet.data,length);
            cus.address[length] = '\0';
            cusAddress = true;
            printf("Customer Address = %s\n",cus.address);
        }
        else if(packetID == PACKET_SALE_ID){
            if(length != sizeof(int)){
                sendResult(client, "BAD SALE ID");
                continue;
            }
            memcpy(&sale.id,packet.data,sizeof(int));
            saleID=true;
            printf("Sale Id: %d\n",sale.id);
        }
        else if(packetID==PACKET_SALE_AMOUNT){
            if(length!=sizeof(float)){
                sendResult(client,"BAD SALE AMOUNT");
                continue;
            }
            memcpy(&sale.amount,packet.data,sizeof(float));
            saleAmount=true;
            printf("Sale Amount: %.2f\n",sale.amount);
        }
        else if(packetID==PACKET_SALE_DATE){
            if(length>=sizeof(sale.date)){
                sendResult(client,"BAD SALE DATE");
                continue;
            }
            memcpy(sale.date,packet.data,length);
            sale.date[length]='\0';
            saleDate=true;
            printf("Sale date: %s\n",sale.date);
        }
        if(empID &&empName &&empSalary){
            if(find(empRoot, emp.id) != NULL){
                sendResult(client,"DUPLICATE");
                empID = false;
                empName = false;
                empSalary = false;
                continue;
            }
            insert(empRoot, emp);
            savefile(emp,1);
            printf("Employee added successfully.\n");
            sendResult(client, "OK");
            empID = false;
            empName = false;
            empSalary = false;
            memset(&emp,0,sizeof(emp));
        }
        if(cusID &&cusName &&cusAddress){
            if(find(cusRoot, cus.id) != NULL){
                sendResult(client,"DUPLICATE");
                cusID = false;
                cusName = false;
                cusAddress = false;
                continue;
            }
            insert(cusRoot, cus);
            savefile(cus,2);
            printf("Customer added successfully.\n");
            sendResult(client, "OK");
            cusID = false;
            cusName = false;
            cusAddress = false;
            memset(&cus,0,sizeof(cus));
        }
        if(saleID && saleAmount && saleDate){
            if(find(saleRoot,sale.id)!= NULL){
                sendResult(client,"DUPLICATE SALE");
                saleID=false;
                saleAmount=false;
                saleDate=false;
                continue;
            }
            insert(saleRoot,sale);
            savefile(sale,3);
            printf("Sale added successfully");
            sendResult(client,"OK");
            saleID=false;
            saleAmount=false;
            saleDate=false;
            memset(&cus,0,sizeof(cus));
        }
    }
}
int main(){
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);
    SOCKET serverSocket =socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr =inet_addr("127.0.0.1");
    if(bind(serverSocket,(sockaddr*)&server,sizeof(server))==SOCKET_ERROR){
        printf("Bined failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    if(listen(serverSocket, 5)==SOCKET_ERROR){
        printf("Listen failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    printf("Server waiting...\n");
    SOCKET client =accept(serverSocket,NULL,NULL);
    if(client==INVALID_SOCKET){
        printf("Accept Failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    printf("Client connected.\n");
    handleAdd(client);
    closesocket(client);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}