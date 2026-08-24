#include<stdio.h>
#include<WinSock2.h>
class Employee{
    public:
    int id;
    char name[50];
    float salary;
};
int main(){
    WSADATA data;
    WSAStartup(MAKEWORD(2,2),&data);
    int serverSocket=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in server,client;
    int clientSize=sizeof(client);
    server.sin_family=AF_INET;
    server.sin_port=htons(8080);
    server.sin_addr.s_addr=INADDR_ANY;
    bind(serverSocket,(sockaddr*)&server,sizeof(server));
    listen(serverSocket,5);
    printf("Sever is waiting");
    int clientSocket=accept(serverSocket,(sockaddr*)&client,&clientSize);
    printf("\nClient Connected");
    Employee e;
    recv(clientSocket,(char*)&e,sizeof(e),0);
    printf("\nEmployee Details");
    printf("\nID: %d\n",e.id);
    printf("Name: %s\n",e.name);
    printf("Salary: %.2f\n",e.salary);
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
