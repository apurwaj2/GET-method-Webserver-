#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sstream>
#include <ctime>
#include <fstream>
#include <vector>
#include <set>

#define BUFF_SIZE 1024

using namespace std;


/*global vars*/
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t emp;
string document_root;
int port;
int timeout = 20;
set<int> connections;
string status_200 = "200 OK";
string status_400 = "400 Bad Request";
string status_403 = "403 Forbidden";
string status_404 = "404 Not Found";
string content_html = "text/html";
string content_text = "text/plain";
string content_jpeg = "image/jpeg";
string content_gif = "image/gif";
string http = "HTTP/1.1 ";
string content_type = "Content-Type: ";
string content_date = "Date: ";
string content_length = "Content-Length: ";

string generate_response(string resp_status, string resp_type_value, int length) {
    string response;
    char buf[1000];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    string date(buf);
    response.append(http);
    response.append(resp_status);
    response.append("\r\n");
    response.append(content_type);
    response.append(resp_type_value);
    response.append("\r\n");
    response.append(content_length);
    response.append(to_string(length+2));
    response.append("\r\n");
    response.append(content_date);
    response.append(date);
    response.append("\r\n");
    response.append("\r\n");
    cout << "Header: " << response << endl;
    memset(buf, 0, sizeof(buf));
    return response;
}

void *process_packet (void *socket)
{
	int clientsock = *((int *) socket);
    char buff[BUFF_SIZE];
    stringstream ss, ext;
    string request_type, file, file_path;
	char *data;			

	// Read from client socket
    if( read(clientsock, buff, 1000) < 0) {
    	cout << "Error in reading client socket" << endl;
        exit(0);
    }

   	//Display content
   	cout <<"Content :" << endl;
   	string content(buff);
   	cout << content << endl;

	//Processing the request
	ss << content;
	ss >> request_type;
	ss >> file;
	cout << "Request type: " << request_type << endl;
	cout << "File: " << file << endl;
	string token;
	ext << file;
	while(getline(ext, token, '.'));
	ss.str(string());
	ss.clear();
	ext.str(string());
	ext.clear();

	//Get absolute path
	if(file.compare("/") == 0) {
		file.append("index.html");
		token = "html";
	}
	file_path = document_root;
	file_path.append(file);
	cout << "File path is : " << file_path << endl;

	string temp;
	// Check if file exists
	int exists = access(file_path.c_str(), F_OK);
	int permission = access(file_path.c_str(), R_OK);

	char *image;
	streampos size;
	struct stat st;
	stat(file_path.c_str(),&st);

	if (exists != -1 && (S_ISDIR(st.st_mode) == 1)) {

		cout << "** Bad Request **" << endl;
		temp = generate_response(status_400, content_html, 0);
		send (clientsock, temp.data(), temp.length(), 0);

	} else if (exists != -1 && permission != -1) {

		pthread_mutex_lock(&lock);

		if (token.compare("txt") == 0) {

			cout << "Text file to be opened" << endl;
			ifstream fd(file_path, ios::in|ios::ate);
			if(fd.is_open()) {
				size = fd.tellg();
				data = new char [size];
				fd.seekg(0, ios::beg);
				fd.read(data, size);
				fd.close();

				temp = generate_response(status_200, content_text, strlen(data)-2);
				temp.append(data);

				send (clientsock, temp.data(), temp.length(), 0);

			} else {
				cout << "Cannot open file" << endl;
			}

		} else {
			ifstream fd(file_path, ios::in|ios::binary|ios::ate);
			if(fd.is_open()) {
				size = fd.tellg();
				data = new char [size];
				fd.seekg(0, ios::beg);
				fd.read(data, size);
				fd.close();

				if(token.compare("html") == 0) {
					temp = generate_response(status_200, content_html, (int)size);
				} else if (token.compare("jpeg") == 0) {
					temp = generate_response(status_200, content_jpeg, (int)size);
				} else if (token.compare("gif") == 0) {
					temp = generate_response(status_200, content_gif, (int)size);
				}

				//Open file and read contents into buffer
				send (clientsock, temp.data(), temp.length(), 0);

				send(clientsock, data, size, 0);
			} else {
				cout << "Cannot open file" << endl;
			}
		}

		pthread_mutex_unlock(&lock);

	} else if(permission == -1 && exists != -1) {

		cout << "Permission Denied!" << endl;
		temp = generate_response(status_403, content_html, 0);
		send (clientsock, temp.data(), temp.length(), 0);

	} else {

		cout << "File not found!!!!!!" << endl;
		temp = generate_response(status_404, content_html, 0);
		send (clientsock, temp.data(), temp.length(), 0);

	}

	cout << "Exit thread" << endl;
	sleep(timeout);
	cout << "Removing connection " << clientsock << endl;
	connections.erase(clientsock);
	close(clientsock);
	pthread_exit(NULL);

}

int main(int argc, char* argv[]) {

    int sockfd, clientsock;
    struct sockaddr_in serv_addr, clientaddr;
    socklen_t addr_size, clientaddrlen;

    if(argc == 5) {
        cout << "argc = " << argc << endl;

        document_root = argv[2];
        port = stoi(argv[4]);

        cout << "document-root " << document_root << endl;
        cout << "port " << port << endl;
    } else if (argc < 5) {
        cout << "Less number of arguments than expected!" <<endl;
        cout << "Expected format ./server -document-root ~/COEN317/P1/document_root -port 8002" <<endl;
    } else {
        cout << "More arguments than expected!" <<endl;
        cout << "Expected format ./server -document-root ~/COEN317/P1/document_root -port 8002" <<endl;
    }

	//Create socket - domain, type, protocol
    // AF_INET - internet domain protocol family
    // SOCK_STREAM - on top of TCP 
    // 0 to select default
    // sockfd socket id
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        cout << "Error in opening socket!" << endl;
        exit(1);
    }

    //Erase data located in server_addr of size of server_addr
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    //address in n/w byte order
    // htonl converts unsigned integer hostlong from host byte order to n/w byte order
    // INADDR_ANY binds socket to all available interface
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    // htons converts unsigned integer hostshort from host byte order to n/w byte order
    serv_addr.sin_port=htons(port);
    addr_size = sizeof(serv_addr);


    //Bind socket
    if(bind(sockfd, (struct sockaddr *)&serv_addr, addr_size) < 0) {
        cout << "Error in socket binding!" << endl;
        exit(1);
    }

    // max 5 backlog queue
    if(listen(sockfd, 5) < 0) {
        cout << "Error in listening at socket!" << endl;
        exit(0);
    }

	pthread_t tid;

	while(1) {

		cout << "Waiting for new connection request!" << endl;
        if((clientsock = accept(sockfd, (sockaddr*) &clientaddr, (socklen_t*) &clientaddrlen)) < 0) {
            cout << "Error in accepting new connection!" << endl;
            exit(0);
        }
		connections.insert(clientsock);

		if(pthread_create(&tid, NULL, process_packet, &clientsock) != 0) {
			cout << "Failed to create new thread!" << endl;
		}
		
		pthread_detach(tid);

		if(timeout > 5)
			timeout -= connections.size();	
		else
			timeout = 20;

		cout << "Timeout " << timeout << endl;
	}

	return 0;

}	
