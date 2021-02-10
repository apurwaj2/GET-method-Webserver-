#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sstream>
#include <fstream>

using namespace std;

#define BUFF_SIZE 1000

string document_root;
int port;
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


int main(int argc, char* argv[]) {

	int sockfd, max_sd;
	struct sockaddr_in serv_addr, clientaddr;
	socklen_t addr_size, clientaddrlen;
	char buff[BUFF_SIZE];
	stringstream ss, ext;
	string request_type, file, file_path;
	int opt = 1;
    int clientsock, client_socket[10], max_clients = 10, activity, valread;

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


	//set of socket descriptors  
    fd_set readfds;   

	//initialise all client_socket[] to 0 so not checked  
    for (int i = 0; i < max_clients; i++)   
    {   
        client_socket[i] = 0;   
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

	//Setting server socket to allow multiple connections 
    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  
          sizeof(opt)) < 0 )   
    {   
        cout << "Error in seting socket option " << endl;
        exit(1);   
    }   

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

	while(1) {

		cout << "Waiting for new connection request!" << endl;

		//clear the socket set  
        FD_ZERO(&readfds);   
     
        //add sockfd to set  
        FD_SET(sockfd, &readfds);   
		max_sd = sockfd;

 		//add client sockets to set 
        for (int i = 0 ; i < max_clients ; i++)
        {
            //socket descriptor 
            clientsock = client_socket[i];

            //if valid socket descriptor then add to read list 
            if(clientsock > 0)
                FD_SET(clientsock, &readfds);

            //highest file descriptor number, need it for the select function 
            if(clientsock > max_sd)
                max_sd = clientsock;
        }

		//wait for an activity on one of the sockets , timeout is NULL, wait indefinitely 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
        if ((activity < 0) && (errno!=EINTR))
        {
            cout << "Error in select" << endl;
        }

		//If something happened on the server socket - an incoming connection 
 		if (FD_ISSET(sockfd, &readfds)) {

			if((clientsock = accept(sockfd, (sockaddr*) &clientaddr, (socklen_t*) &clientaddrlen)) < 0) {
				cout << "Error in accepting new connection!" << endl;
				exit(0);
			}

			//add new socket to array of sockets 
            for (int i = 0; i < max_clients; i++)
            {
                //if position is empty 
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = clientsock;
                    break;
                }
            }
		}

		
	   	//else its some IO operation on some other socket 
       	for (int i = 0; i < max_clients; i++)
       	{
            clientsock = client_socket[i];

            if (FD_ISSET(clientsock , &readfds))
            {
	
				// Read from client socket
				int status = read(clientsock, buff, BUFF_SIZE);
				if(status < 0) {
					cout << "Error in reading client socket" << endl;
					exit(0);
				} else if (status > 0) {		

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

						char *data;
						streampos size;
						struct stat st;
						stat(file_path.c_str(),&st);
					
						if (exists != -1 && (S_ISDIR(st.st_mode) == 1)) {

							cout << "** Bad Request **" << endl;
							temp = generate_response(status_400, content_html, 0);
							send (clientsock, temp.data(), temp.length(), 0);

						} else if (exists != -1 && permission != -1) {

							
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

						} else if(permission == -1 && exists != -1) {

							cout << "Permission Denied!" << endl;
							temp = generate_response(status_403, content_html, 0);
							send (clientsock, temp.data(), temp.length(), 0);

						} else {

							cout << "File not found!!!!!!" << endl;
							temp = generate_response(status_404, content_html, 0);
							send (clientsock, temp.data(), temp.length(), 0);

						}

						temp.clear();
						token.clear();
						request_type.clear();
						content.clear();
						file.clear();
						file_path.clear();
						memset(buff, 0, BUFF_SIZE);
						memset(data, 0, size);
				} else {
					//Close the socket and mark as 0 in list for reuse 
                    close(clientsock);
                    client_socket[i] = 0;
				}
			}
		}
	} 
	

   	return 0;
}
