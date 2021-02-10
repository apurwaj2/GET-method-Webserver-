Name: Apurwa Jadhav

Assignment name : Programming Assignment 1

Date : Jan 31st 2021

Description : The program implements a simple webserver supporting HTTP1.1 and it's GET method.
 It listens on the port mentioned in the command line arguement and fetches files present in the document_root 
 which is also passed as a command line argument. 
 It supports txt, html, jpeg and gif files. 
 Status codes supported - 200, 400, 403, 404 

 Main file - server.cpp multithreaded - pthread implementation of webserver. 
 1. For every connection create a thread and detach it to handle the request and send response. 
	Close the connection after putting the thread to sleep for timeout amount.

	sleep(timeout);
    cout << "Removing connection " << clientsock << endl;
    connections.erase(clientsock);  // removing the connection from the set of connections
    close(clientsock);
 
 2. Calculating timeout after new connection 
	Using a set of connections to get the total number of active connections

	    if(timeout > 5)
            timeout -= connections.size();
        else
            timeout = 20;

	Resetting timeout to 20 after it goes below 5
 3. Permission not granted file mode tested with 060
	Set the mode using the below command
	chmod 060 mode.html

List of files submitted :
1. server.cpp
2. index.html (SCU Homepage - Modified a bit by removing references to some links to make it work) 
3. sample.html
4. panda.jpeg
5. doggo.gif
6. mode.html - mode 060  cannot open file 403 error
7. dir - empty folder - 400 error
8. select_server.cpp - another implementation using select - does not close connection

Instrustions to run the program :
1. g++ server.cpp -o server -lpthread
2. ./server -document-root ~/COEN317/P1/document_root -port 8003


Other information :

 Another file select_server.cpp select command in linux to support multiple client connections.
 This does not close connection. Tried to attempt event driven but did not use non-blocking socket and hence 
 worked on multithreading using pthreads.

