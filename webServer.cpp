// **************************************************************************************
// * webServer (webServer.cpp)
// * - Implements a very limited subset of HTTP/1.0, use -v to enable verbose debugging output.
// * - Port number 1701 is the default, if in use random number is selected.
// *
// * - GET requests are processed, all other metods result in 400.
// *     All header gracefully ignored
// *     Files will only be served from cwd and must have format file\d.html or image\d.jpg
// *
// * - Response to a valid get for a legal filename
// *     status line (i.e., response method)
// *     Cotent-Length:
// *     Content-Type:
// *     \r\n
// *     requested file.
// *
// * - Response to a GET that contains a filename that does not exist or is not allowed
// *     statu line w/code 404 (not found)
// *
// * - CSCI 471 - All other requests return 400
// * - CSCI 598 - HEAD and POST must also be processed.
// *
// * - Program is terminated with SIGINT (ctrl-C)
// **************************************************************************************
#include "webServer.h"
#include <cerrno>
#include <signal.h>


// **************************************************************************************
// * Signal Handler.
// * - Display the signal and exit (returning 0 to OS indicating normal shutdown)
// * - Optional for 471, required for 598
// **************************************************************************************
void sig_handler(int signo) {
  DEBUG << "Caught a signal: " << signo << ENDL;
  DEBUG << "Closing the socket" << ENDL;
  closefrom(3);
  exit(1);
}


// **************************************************************************************
// * processRequest,
//   - Return HTTP code to be sent back
//   - Set filename if appropriate. Filename syntax is valided but existance is not verified.
// **************************************************************************************
int readHeader(int sockFd,std::string &filename) {
  return 0;
}


// **************************************************************************
// * Send one line (including the line terminator <LF><CR>)
// * - Assumes the terminator is not included, so it is appended.
// **************************************************************************
void sendLine(int socketFd, std::string &stringToSend) {
  return;
}

// **************************************************************************
// * Send the entire 404 response, header and body.
// **************************************************************************
void send404(int sockFd) {
  return;
}

// **************************************************************************
// * Send the entire 400 response, header and body.
// **************************************************************************
void send400(int sockFd) {
  return;
}


// **************************************************************************************
// * sendFile
// * -- Send a file back to the browser.
// **************************************************************************************
void sesendFile(int sockFd,std::string filename) {
  return;
}


// **************************************************************************************
// * processConnection
// * -- process one connection/request.
// **************************************************************************************
int processConnection(int sockFd) {
 
  // Call readHeader()

  // If read header returned 400, send 400

  // If read header returned 404, call send404

  // 471: If read header returned 200, call sendFile
  
  // 598 students
  // - If the header was valid and the method was GET, call sendFile()
  // - If the header was valid and the method was HEAD, call a function to send back the header.
  // - If the header was valid and the method was POST, call a function to save the file to dis.

  return 0;
}
    

int main (int argc, char *argv[]) {

  // ********************************************************************
  // * Process the command line arguments
  // ********************************************************************
  int opt = 0;
  while ((opt = getopt(argc,argv,"d:")) != -1) {
    
    switch (opt) {
    case 'd':
      LOG_LEVEL = std::stoi(optarg);
      break;
    case ':':
    case '?':
    default:
      std::cout << "useage: " << argv[0] << " -d LOG_LEVEL" << std::endl;
      exit(-1);
    }
  }

  // Calling the signal call
  signal(SIGINT, sig_handler);
  DEBUG << "Setting up signal handlers" << ENDL;

  // Create the socket
  int listenFd = socket(AF_INET, SOCK_STREAM, 0);
  DEBUG << "Calling Socket() assigned file descriptor " << listenFd << ENDL;

  // Check to see if the socket() call failed
  if (listenFd == -1){
    std::cout << "Socket() call failed" << std::endl;
    return -1;
  }

  // Port value
  uint16_t port;
  DEBUG << "Calling bind()" << ENDL;
  
  //----- Bind verification loop -----

  // Boolean exitLoop condition
  bool exitLoop = false;

  // Int port placeholder
  int tempPort = 1029;

  // fill out regular sockaddr_in structure
  sockaddr_in serverAddress;
  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  // Bind loop
  while (!exitLoop){

    // Assign tempPort to port
    port = tempPort;

    // Fill out sockaddr_in port
    serverAddress.sin_port = htons(port);

    // Bind the socket, and verify the return value
    if (bind(listenFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1){
      // The bind call has failed. Using the value stored in errno, determine if the port is already being used, or if another error took place
      if (errno == EADDRINUSE){
        // Output error message
        std::cout << "Port number is already being used. Trying a new port" << std::endl;
        
        // Increase tempPort variable by one, then try again
        tempPort = tempPort + 1;
      } else {
        // Output error message
        std::cout << "There is something else wrong with the program" << std::endl;

        // Return from main (quit the program)
        close(listenFd);
        return -1;
      }
    }
    else {
      // Set exitLoop to true to exit the validation loop
      exitLoop = true;
    }
  }

  std::cout << "Using port: " << port << std::endl;


  // Calling the listen() function with a backlog of 5
  listen(listenFd, 5);
  DEBUG << "Calling listen()" << ENDL;

  // ----- Accept validation loop -----
  int quitProgram = 0;
  while (!quitProgram) {
    int connFd = 0;
    DEBUG << "Calling connFd = accept(fd,NULL,NULL)." << ENDL;

    // Calling the accept function
    connFd = accept(listenFd, NULL, NULL);

    // Check if the accept call returned -1. If so, continue to the next iteration
    if (connFd == -1){
      continue;
    }

    DEBUG << "We have recieved a connection on " << connFd << ". Calling processConnection(" << connFd << ")" << ENDL;
    quitProgram = processConnection(connFd);
    DEBUG << "processConnection returned " << quitProgram << " (should always be 0)" << ENDL;
    DEBUG << "Closing file descriptor " << connFd << ENDL;
    close(connFd);
  }
  

  ERROR << "Program fell through to the end of main. A listening socket may have closed unexpectadly." << ENDL;
  closefrom(3);

}
