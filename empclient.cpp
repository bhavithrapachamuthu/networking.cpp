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
    int clientSocket=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in server;
    server.sin_family=AF_INET; 
    server.sin_port=htons(8080);
    server.sin_addr.s_addr=inet_addr("127.0.0.1");
    connect(clientSocket,(sockaddr*)&server,sizeof(server));
    Employee e;
    printf("Enter Employee Id: ");
    scanf("%d",&e.id);
    printf("Enter Employee Name: ");
    scanf("%s",e.name);
    printf("Enter Salary: ");
    scanf("%f",&e.salary);
    send(clientSocket,(char*)&e,sizeof(e),0);
    printf("Employee Details sent");
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}