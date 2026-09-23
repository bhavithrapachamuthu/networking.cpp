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
#define PACKET_VIEW_ALL 13
#define PACKET_SEARCH 14
#define PACKET_DELETE 15 
#define FRAME_SINGLE 0
#define FRAME_MORE 1
struct Packet{
    unsigned char header;
    unsigned char length;
    unsigned char data[256];
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
    int empId;
    int cusId;
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
mutex dataMutex;
unsigned char makeHeader(int packetID,int frameType){
    return ((packetID & 0x0F) << 1) | (frameType & 0x01);
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
Node<Employee>*findbyname(Node<Employee>*root,const char*name){
    if(root==NULL){
        return NULL;
    }
    Node<Employee>*result=findbyname(root->left,name);
    if(result!=NULL){
        return result;
    }
    if(strcmp(root->data.empName,name)==0){
        return root;
    }
    return findbyname(root->right,name);
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
template<typename T>
Node<T>*deleteNode(Node<T>*root,int id){
    if(root==NULL){
        return NULL;
    }
    if(id< root->data.id){
        root->left=deleteNode(root->left,id);
    }
    else if(id>root->data.id){
        root->right=deleteNode(root->right,id);
    }
    else{
        if(root->left==NULL && root->right==NULL){
            delete root;
            return NULL;
        }
        else if(root->left==NULL){
            Node<T>*temp=root->right;
            delete root;
            return temp;
        }
        else if(root->right==NULL){
            Node<T>*temp=root->left;
            delete root;
            return temp;
        }
        else{
            Node<T>*temp=root->right;
            while(temp->left!=NULL){
               temp=temp->left; 
            }
            root->data=temp->data;
            root->right=deleteNode(root->right,temp->data.id);    
        }
    }
    return root;
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
bool sendPacket(SOCKET client,int packetID,int length,int frameType,const void* data){
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
    if(length>0){
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
    int packetId=getPacketID(packet.header);
    int frametype=getFrameType(packet.header);
    printf("\n----------------------------------\n");
    printf("Packet Id: %d\n",packetId);
    printf("Length: %d\n",packet.length);
    if(frametype==FRAME_MORE){
        printf("Frame Type: MORE\n");
    }
    else{
        printf("Frame Type: SINGLE\n");
    }
    printf("\n----------------------------------\n");
    return true;
}
void sendResult(SOCKET client,const char* msg){
    int length = strlen(msg);
    if(length > 255)
    length = 255;
    sendPacket(client,PACKET_RESULT,length,FRAME_SINGLE,msg);
}
void saveallEmp(Node<Employee>*root,FILE*f){
    if(root==NULL){
        return;
    }
    saveallEmp(root->left,f);
    unsigned char type=1;
    fwrite(&type,sizeof(type),1,f);
    fwrite(&root->data,sizeof(Employee),1,f);
    saveallEmp(root->right,f);
}
void saveallCus(Node<Customer>*root,FILE*f){
    if(root==NULL){
        return;
    }
    saveallCus(root->left,f);
    unsigned char type=2;
    fwrite(&type,sizeof(type),1,f);
    fwrite(&root->data,sizeof(Customer),1,f);
    saveallCus(root->right,f);
}
void saveallSale(Node<Sale>*root,FILE*f){
    if(root==NULL){
        return;
    }
    saveallSale(root->left,f);
    unsigned char type=3;
    fwrite(&type,sizeof(type),1,f);
    fwrite(&root->data,sizeof(Sale),1,f);
    saveallSale(root->right,f);
}
void savefile(){
    FILE*f=fopen("server.bin","wb");
    if(f==NULL){
        printf("File saving Failed\n");
        return;
    }
    saveallEmp(empRoot,f);
    saveallCus(cusRoot,f);
    saveallSale(saleRoot,f);
    fclose(f);
    printf("File updated sucessfully\n");
}
template<typename T>
void loadvalue(FILE*f,Node<T>*&root){
    T data;
    if(fread(&data,sizeof(T),1,f)==1){
        if(find(root,data.id)== NULL){
            insert(root,data);
        }
    }
}
void loadfile(){
    FILE*f=fopen("server.bin","rb");
    if(f==NULL){
        printf("No old data found!\n");
        return;
    }
    unsigned char type;
    while (fread(&type,sizeof(type),1,f)==1){
        if(type==1){
            loadvalue(f,empRoot);
        }
        else if(type==2){
            loadvalue(f,cusRoot);
        }
        else if(type==3){
            loadvalue(f,saleRoot);
        }
        else{
            printf("Invalid!");
            break;
        }
    }
    fclose(f);
    printf("\nData loaded successfully\n");
}
void displayempData()
void sendEmployee(Node<Employee>*root){
    if(root==NULL){
        return;
    }
    printf("%-10s %-20s %-15s\n","ID","EMPLOYEE NAME","SALARY");
    sendEmployee(root->left);
    printf("Salary: %.2f\n",root->data.salary);
    printf("%-10d %-20s %-15.2f\n",root->data.id,root->data.empName,root->data.salary);
    sendEmployee(root->right);
}
void sendCustomer(Node<Customer>*root){
    if(root==NULL){
        return;
    }
    sendCustomer(root->left);
    printf("ID: %d\n",root->data.id);
    printf("Name: %s\n",root->data.cusName);
    printf("Address: %s\n",root->data.address);
    sendCustomer(root->right);
}
void sendSale(Node<Sale>*root){
    if(root==NULL){
        return;
    }
    sendSale(root->left);
    printf("Sale ID: %d\n",root->data.id);
    printf("Employee ID: %d\n",root->data.empId);
    printf("Customer ID: %d\n",root->data.cusId);
    printf("Amount: %.2f\n",root->data.amount);
    printf("Date: %s\n",root->data.date);
    sendSale(root->right);
}
void ViewAll(){
    lock_guard<mutex>lock(dataMutex);
    printf("\n===========================================\n");
    printf("\n           EMPLOYEE DETIALS\n");
    printf("\n===========================================\n");
    printf("\n----------------------------------------------\n");
    printf("ID  EMPLOYEE NAME  EMPLOYEE SALARY\n");
    printf("\n----------------------------------------------\n");
    sendEmployee(empRoot);
    printf("\n===========================================\n");
    printf("\n           CUSTOMER DETIALS\n");
    printf("\n===========================================\n");
    printf("\n----------------------------------------------\n");
    printf("ID   CUSTOMER NAME      ADDRESS\n");
    printf("\n----------------------------------------------\n");
    sendCustomer(cusRoot);
    printf("\n===========================================\n");
    printf("\n           SALE DETIALS\n");
    printf("\n===========================================\n");
    printf("\n----------------------------------------------\n");
    printf("SALE.ID  EMP.ID   CUS.ID  AMOUNT     DATE\n");
    printf("\n----------------------------------------------\n");
    sendSale(saleRoot);
    printf("\n======================\n");
    printf("View All completed\n");
    printf("\n======================\n");
}
void searchSale(Node<Sale>*root,int empId){
    if(root==NULL){
        return;
    }
    searchSale(root->left,empId);
    if(root->data.empId==empId){
        Sale&s=root->data;
        printf("Sale ID: %d\n",s.id);
        printf("Sale ID: %d\n",s.empId);
        printf("Customer Id: %d\n",s.cusId);
        printf("Amount: %.2f\n",s.amount);
        printf("Date: %s\n",s.date);
    }
    searchSale(root->right,empId);
}
void handleSearch(){
    char name[50];
    printf("\nSearching Employee Name: %s\n",name);
    scanf("%49s",name);
    lock_guard<mutex>lock(dataMutex);
    Node<Employee>*emp=findbyname(empRoot,name);
    if(emp==NULL){
        printf("\nEmployee not found.\n");
        return;
    }
    printf("\n=============EMPLOYEE DETAILS=============\n");
    printf("Employee Id: %d\n",emp->data.id);
    printf("Employee Name: %s\n",emp->data.empName);
    printf("Salary: %.2f\n",emp->data.salary);
    printf("\n=============SALE DETAILS=============\n");
    printf("\n----------------------------------------------\n");
    printf("Sale.ID   Emp.Id   Cus.Id   Amount    Date");
    printf("\n----------------------------------------------\n");
    searchSale(saleRoot,emp->data.id);
}
void handleEmp(SOCKET client,Packet&p){
    Employee e;
    memset(&e,0,sizeof(e));
    if(p.length!=sizeof(int)){
        sendResult(client,"INVALID EMPLOYEE ID");
        return;
    }
    memcpy(&e.id,p.data,sizeof(int));
    lock_guard<mutex>lock(dataMutex);
    printf("Employee Id: %d\n",e.id);
    if(find(empRoot,e.id)!=NULL){
        printf("Employee Id already exists!\n");
        sendResult(client,"DUPLICATE ID");
        return;
    }
    sendResult(client,"ID AVAILABLE");
    if(!receivePacket(client,p)){
        return;
    }
    if(p.length>=sizeof(e.empName)){
        sendResult(client,"EMPLOYEE NAME TOO LONG");    
        return;
    }
    memcpy(e.empName,p.data,p.length);
    e.empName[p.length]='\0';
    if(!receivePacket(client,p)){
        return;
    }
    if(p.length!=sizeof(float)){
        sendResult(client,"INVALID SALARY");
        return;
    }
    memcpy(&e.salary,p.data,sizeof(float));
    insert(empRoot,e);
    savefile();
    sendResult(client,"EMPLOYEE ADDED");
    printf("\nEmployee added successfully\n");
}
void handleCus(SOCKET client,Packet&p){
    Customer c;
    memset(&c,0,sizeof(c));
    if(p.length!=sizeof(int)){
        sendResult(client,"INVALID CUSTOMER ID");
        return;
    }
    memcpy(&c.id,p.data,sizeof(int));
    lock_guard<mutex>lock(dataMutex);
    if(find(cusRoot,c.id)!=NULL){
        printf("\nCustomer Id already exists\n");
        sendResult(client,"DUPLICATE ID");
        return;
    }
    sendResult(client,"ID AVAILABLE");
    if(!receivePacket(client,p)){
        return;
    }
    if(p.length>=sizeof(c.cusName)){
        sendResult(client,"CUSTOMER NAME TOO LONG");    
        return;
    }
    memcpy(c.cusName,p.data,p.length);
    c.cusName[p.length]='\0';
    if(!receivePacket(client,p)){
        return;
    }
    if(p.length>=sizeof(c.address)){
        sendResult(client,"CUSTOMER ADDRESS TOO LONG");    
        return;
    }
    memcpy(c.address,p.data,p.length);
    c.address[p.length]='\0';
    insert(cusRoot,c);
    savefile();
    sendResult(client,"CUSTOMER ADDED");
    printf("\nCustomer added successfully\n");
}
void handleSale(SOCKET client, Packet p){
    Sale s;
    int saleId=1;
    {
        lock_guard<mutex>lock(dataMutex);
        if(saleRoot!=NULL){
            Node<Sale>*temp=saleRoot;
            while (temp->right!=NULL)
            {
                temp=temp->right;
            }
            saleId=temp->data.id;
        }
    }
    s.id=saleId+1;
    printf("Generated Sale Id: %d\n",s.id);
    sendResult(client,"SALE ID GENERATED");
    while(true){
        if(!receivePacket(client,p))
        return;
        if(getPacketID(p.header)!=PACKET_SALE_EMP_ID)
        continue;
        if(p.length!=sizeof(int));
        continue;
        memcpy(&s.empId,p.data,sizeof(int));
        {
            lock_guard<mutex>lock(dataMutex);
            Node<Employee>*emp=find(empRoot,s.empId);
            if(emp==NULL){
                sendResult(client,"Employee Id doesn't exists");
                continue;
            }
        }
        sendResult(client,"EMPLOYEE ID VALID");
        break;
    }
    while(true){
        if(!receivePacket(client,p))
        return;
        if(getPacketID(p.header)!=PACKET_SALE_EMP_ID){
            sendResult(client,"INVALID EMPLOYEE ID");
            return;
        }
        if(p.length!=sizeof(int));
        continue;
        memcpy(&s.cusId,p.data,sizeof(int));
        {
            lock_guard<mutex>lock(dataMutex);
            Node<Customer>*cus=find(cusRoot,s.cusId);
            if(cus==NULL){
                sendResult(client,"Customer Id doesn't exists");
                continue;
            }
        }
        sendResult(client,"CUSTOMER ID VALID");
        break;
    }
    if(!receivePacket(client,p))
    return;
    if(getPacketID(p.header)!=PACKET_SALE_AMOUNT){
        sendResult(client,"INVALID SALE AMOUNT");
        return;
    }
    memcpy(&s.amount,p.data,sizeof(float));
    if(!receivePacket(client,p)){
        return;
    }
    if(getPacketID(p.header)!=PACKET_SALE_DATE){
        sendResult(client,"INVALID SALE DATE");
        return;
    }
    if(p.length>=sizeof(s.date)){
        sendResult(client,"SALE DATE TOO LONG");
        return;
    }
    memcpy(s.date,p.data,p.length);
    s.date[p.length]='\0';
    {
        lock_guard<mutex>lock(dataMutex);
        saleRoot=insert(saleRoot,s);
        savefile();
    }
    sendResult(client,"SALE ADDED SUCCESSFULLY");
    printf("Sale Addedd successfully\n");
    printf("Sale Id: %d\n",s.id);
    printf("Employee Id: %d\n",s.empId);
    printf("Customer Id: %d\n",s.empId);
    printf("Amount: %.2f\n",s.amount);
    printf("Date: %s\n",s.date);
}
void deleteEmp(){
    int id;
    printf("\n========DELETE EMPLOYEE=========\n");
    printf("\nEmployee Id to delete: %d\n",id);
    scanf("%d",&id);
    lock_guard<mutex>lock(dataMutex);
    Node<Employee>*emp=find(empRoot,id);
    if(emp==NULL){
        printf("\nEmployee not found\n");
        return;
    }
    empRoot=deleteNode(empRoot,id);
    savefile();
    printf("\nEmployee deleted successfully\n");
}
void deleteCus(){
    int id;
    printf("\n========DELETE CUSTOMER========\n");
    printf("\nCustomer Id to delete: %d\n",id);
    scanf("%d",&id);
    lock_guard<mutex>lock(dataMutex);
    Node<Customer>*cus=find(cusRoot,id);
    if(cus==NULL){
        printf("\nCustomer not found\n");
        return;
    }
    cusRoot=deleteNode(cusRoot,id);
    savefile();
    printf("\nCustomer deleted successfully\n");
}
void deleteSale(){
    int id;
    printf("\n========DELETE SALE========\n");
    printf("\nSale Id to delete: %d\n",id);
    scanf("%d",&id);
    lock_guard<mutex>lock(dataMutex);
    Node<Sale>*sale=find(saleRoot,id);
    if(sale==NULL){
        printf("\nSale not found\n");
        return;
    }
    saleRoot=deleteNode(saleRoot,id);
    savefile();
    printf("\nSale deleted successfully\n");
}
void handleAll(SOCKET client){
    Packet p;
    while(true){
        if(!receivePacket(client,p)){
            printf("Client disconnected\n");
            break;
        }
        int packetId=getPacketID(p.header);
        switch(packetId){
            case PACKET_EMPLOYEE_ID:
            handleEmp(client,p);
            break;

            case PACKET_CUSTOMER_ID:
            handleCus(client,p);
            break;

            case PACKET_SALE_ID :
            handleSale(client,p);
            break;

            case PACKET_SALE_EMP_ID:
            case PACKET_SALE_CUS_ID:
            printf("Unexpected Sale Packet: %d\n",packetId);
            break;

            case PACKET_VIEW_ALL:
            printf("View All packet recv from client\n");
            break;

            case PACKET_SEARCH:
            printf("Search packet recv from client\n");
            break;

            case PACKET_DELETE:
            printf("Delete packet recv from client\n");
            break;
            default:
            printf("Unknown Packet Id: %d\n",packetId);
            break;
        }
    }
}
void clientHandler(SOCKET client,int clientNum){
    printf("\n=================================\n");
    printf("Client %d connected\n",clientNum);
    printf("\n=================================\n");
    handleAll(client);
    closesocket(client);
    printf("Client %d disconnected\n",clientNum);
}
void menu(bool&serverStatus,SOCKET serversocket){
    while(serverStatus){
        int choice;
        printf("\n=================================\n");
        printf("              MENU\n");
        printf("\n=================================\n");
        printf("1. Search Employee\n");
        printf("2. Delete Employee\n");
        printf("3. Delete Customer\n");
        printf("4. Delete Sale\n");
        printf("5. View all\n");
        printf("--------------------------------\n");
        printf("Enter choice: ");
        if(scanf("%d",&choice)!=1){
            while(getchar()!='\n');
            continue;
        }
        switch(choice){
            case 1:
            handleSearch();
            break;

            case 2:
            deleteEmp();
            break;

            case 3:
            deleteCus();
            break;

            case 4:
            deleteSale();
            break;

            case 5:
            ViewAll();
            break;

            case 6:
            serverStatus=false;
            closesocket(serversocket);
            printf("\nServer closing\n");
            return;
        
            default:
            printf("Invalid choice\n");
        }
    }
}
int main(){
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){
        printf("WSAStartup failed\n");
        return 1;
    }
    SOCKET serverSocket =socket(AF_INET,SOCK_STREAM,0);
    if(serverSocket==INVALID_SOCKET){
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }
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
    printf("SERVER STARTED\n");
    printf("\nPORT: %d\n",PORT);
    loadfile();
    bool serverStatus=true;
    thread menuThread(menu,ref(serverStatus),serverSocket);
    int clientNum=0;
    while(serverStatus){
        SOCKET client=accept(serverSocket,NULL,NULL);
        if(client==INVALID_SOCKET){
            printf("Accept Failed\n");
            continue;
        }
        clientNum++;
        int currentClient=clientNum;
        thread t(clientHandler,client,currentClient);
        t.detach();
    }
    serverStatus=false;
    if(menuThread.joinable()){
        menuThread.join();
    }
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
