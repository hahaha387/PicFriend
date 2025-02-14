
#include "network.h"
#include "encodephoto.h"


#define SERV_PORT 9878
#define LISTENQ 1000
#define INFTIM -1 //poll永远等待
#define MAXLINE 1024


using json = nlohmann::json;


Network::Network()
{

}

Network::Network(int &fd)
    :m_listenFd(fd)
{

}

int Network::createSocket()
{
    m_listenFd = socket(AF_INET,SOCK_STREAM,0);
    if(m_listenFd==INVALID_SOCKET_FD){
         printf("Create socket failed. Errorn info: %d %s\n",errno,strerror(errno));
    }
    return m_listenFd;
}

int Network::bindSocket()
{
    struct sockaddr_in servaddr;
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    if(bind(m_listenFd,(struct sockaddr *)&servaddr,sizeof(servaddr)) == -1){
        printf("Bind socket failed. Errorn info: %d %s\n",errno,strerror(errno));
        return -1;
    }
    return 0;
}

int Network::listenSocket()
{
    if(listen(m_listenFd,LISTENQ)<0){
        std::cerr<<"Listen socket error.Errorn info "<<errno<<","<<strerror(errno)<<std::endl;
        return -1;
    }
    return 0;
}

int Network::acceptSocket()
{
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    int listenFd = accept(m_listenFd, (struct sockaddr *)&cliaddr,&clilen);
    if(listenFd<0){
        printf("Accept socket failed. Errorn info: %d %s\n",errno,strerror(errno));
    }
    return listenFd;
}

int Network::pollSocket()
{
    struct pollfd pd;
    pd.fd  = m_listenFd;
    pd.events = POLLRDNORM;
    if(poll(&pd,1,INFTIM)<=0){
        printf("Poll failed. Errorn info: %d %s\n",errno,strerror(errno));
    }
    return 1;

}

Network::~Network()
{
    closeSocket();
}

void Network::closeSocket()
{
    close(m_listenFd);
}

int Network::sendMessage(char *buf,size_t size)
{
    int send_size = 0 , msg_size = strlen(buf);

    send_size = send(m_listenFd, &msg_size, sizeof(5), 0);

    int pos = 0;
    std::string tmp(buf);
    while (msg_size > 0) {
       send_size = send(m_listenFd, buf+pos, 1024, 0);

       if (send_size < 0) {
            printf("Server write error. Errorn info: %d %s\n",errno,strerror(errno));
            return false;
       }
       pos += send_size;
       msg_size -= send_size;;

    }
    return true;
}

/*
std::string Network::receiveMessage(int& m_listenFd)
{
    char buf[MAXLINE];
    memset(buf,0,sizeof(buf));
    int n=recv(m_listenFd,buf,sizeof(buf),0);
    if( n == -1){
        if(errno == ECONNRESET || errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN){
            printf("Server read error. Errorn info: %d %s\n",errno,strerror(errno));
        }
        return nullptr;
    }else if(n==0){
        printf("The opposite end has closed the socket.\n");
        return nullptr;
    }
    std::string s(buf);
    return s;
}*/

bool Network::receiveMessage(char* buffer)
{
    int one_size = 0,msg_size;
    std::string msg;

//    //每次发送数据之前，先发送一个固定长度的自定义包头，包头中定义了这一次数据的长度。
//    //服务端先按照包头长度接受包头数据，再按照包头数据中指定的数据长度接受这一次通信的数据。
//    //我们使用一个int类型作为“包头”，代表发送数据的长度。
//    //而int类型固定4字节，因此服务端每次先接受4字节的数据x，再接受x字节的字符串数据。

    one_size = read(m_listenFd,&msg_size,sizeof(4));
    int pos = 0;
    while (msg_size > 0) {
        one_size = recv(m_listenFd, buffer+pos , 1024, 0);
        if (one_size == 0) {
            printf("client disconnect\n");
            return false;
        }
        if (one_size < 0) {
            printf("Server read error. Errorn info: %d %s\n",errno,strerror(errno));
            return false;
        }
        pos += one_size;
        msg_size -= one_size;
    }
    return true;
}

int Network::sendFile(std::string path)
{
//    int file_fd = open(path.c_str(),O_RDWR | O_CREAT,664);
//    if(file_fd < 0){
//        perror("open");
//        return -1;
//     }
//           //6、//设置file_fd文件描述符属性
//     struct stat stat_buf;
//     stat(path.c_str(),&stat_buf);
//     auto size = stat_buf.st_size;
//    int size1=sendfile(m_listenFd,file_fd,nullptr,size);
//    std::cout<<"传输的数据为"<<size1<<"字节"<<std::endl;

    std::string photo=encodePhoto(path);

    return 1;
}

int Network::sendFile(char *buf, size_t size, std::string filePath)
{
    if(m_listenFd<0){
        printf("Client socket error.Errorn info: %d %s\n",errno,strerror(errno));
        return false;
    }
    FILE *fq;
    if( ( fq = fopen(filePath.c_str(),"rb") ) == NULL ){
        printf("File open.\n");
        close(m_listenFd);
        exit(1);
    }
    int len;
    while(!feof(fq)){
        len = fread(buf, 1, sizeof(buf), fq);
        if(len != ::send(m_listenFd, buf, len,0)){
            printf("Server file write error. Errorn info: %d %s\n",errno,strerror(errno));
            break;
        }
    }
    fclose(fq);
    return 1;
}

std::string Network::receiveFile(std::string filePath)
{
    FILE *fp;
    if((fp = fopen(filePath.c_str(),"ab")) == NULL ){
       printf("File.\n");
       close(m_listenFd);
       exit(1);
    }

    char buf[MAXLINE];
    memset(buf,0,sizeof(buf));
    int n;

    while(1){
        n = ::recv(m_listenFd, buf, MAXLINE,0);
        if(n<0) printf("Server file read error. Errorn info: %d %s\n",errno,strerror(errno));
        if(n == 0) break;
        fwrite(buf, 1, n, fp);
    }
    buf[n] = '\0';
    fclose(fp);

    return "接收文件成功！！";
}

