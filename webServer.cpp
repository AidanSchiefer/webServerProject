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
#include <array>


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
  // Set the default return code to 400
  int returnCode = 400;

  // Read everything up to and including the end of the header
  // Create a char buffer and string container
  char buffer[10] = {0};
  std::string bufferToString = "";

  // Call read continuously
  while (true){
    // Zero out the buffer to remove any leftover junk
    memset(buffer, 0, sizeof(buffer));

    // Store the read result in an int
    int readResult = read(sockFd, buffer, sizeof(buffer));

    // If the buffer is 0, exit the loop
    if (readResult == 0){
      break;
    }
    else if (readResult == -1) {
      ERROR << "Pipe Broken" << ENDL;
      return returnCode;
    }
    // Add the buffer to the buffer string
    bufferToString += buffer;
    // Check if buffer has the termination pattern, else continue the loop
    if (bufferToString.find("\r\n\r\n") != std::string::npos){
      break;
    }
  }

  // Parse through the header to find a valid get
  if (bufferToString.find("\r\n") != std::string::npos){
    // Copy the first line
    int index = bufferToString.find("\r\n");
    std::string headerFirstLine = bufferToString.substr(0, index);
    std::smatch match;

    // Check if the first line contains GET
    if (headerFirstLine.substr(0, 3) == "GET"){
      // Set regex parameters for file name matching
      std::regex filePattern1(R"(file\d\.html)");
      std::regex filePattern2(R"(image\d\.jpg)");

      // Determine if the file name matches the perscribed format. Return status codes based off of the result
      if (std::regex_search(headerFirstLine, match, filePattern1) || std::regex_search(headerFirstLine, match, filePattern2)){
        filename = "data/" + match[0].str();
        returnCode = 200;
      }
      else{
        filename = "";
        returnCode = 404;
      }
    }
  }


  return returnCode;
}


// **************************************************************************
// * Send one line (including the line terminator <LF><CR>)
// * - Assumes the terminator is not included, so it is appended.
// **************************************************************************
void sendLine(int socketFd, std::string &stringToSend) {
  // Create a string to send the header line back
  std::string toSend = stringToSend + "\r\n";

  // Write the message back
  write(socketFd, toSend.c_str(), toSend.size());
}

// **************************************************************************
// * Send the entire 404 response, header and body.
// **************************************************************************
void send404(int sockFd) {
  // String definitions for lines to be sent
  std::string msg404 = "HTTP/1.0 404 Not Found";
  std::string contentType = "content-type: text/html";
  std::string empty = "";
  std::string errorMsg = "The requested file was not valid";

  // sendLine function calls
  sendLine(sockFd, msg404);
  sendLine(sockFd, contentType);
  sendLine(sockFd, empty);
  sendLine(sockFd, errorMsg);
  sendLine(sockFd, empty);
}

// **************************************************************************
// * Send the entire 400 response, header and body.
// **************************************************************************
void send400(int sockFd) {
  // Create a 400 error string
  std::string msg400 = "HTTP/1.0 400 Bad Request";
  std::string empty = "";
  // Send the properly formatted 400 response
  sendLine(sockFd, msg400);

  // Send a blank line
  sendLine(sockFd, empty);
}


// **************************************************************************************
// * sendFile
// * -- Send a file back to the browser.
// **************************************************************************************
void sendFile(int sockFd,std::string filename) {
  // Create send line string formats
  std::string msg200 = "HTTP/1.0 200 OK";
  std::string contentTypeText = "content-type: text/html";
  std::string contentTypeImage = "content-type: image/jpg";
  std::string contentLength = "content-length: ";
  std::string empty = "";
  char buffer[10] = {0};
  struct stat fileInfo;

  // Call the stat function to get the file size
  int statResult = stat(filename.c_str(), &fileInfo);
  // If the stat call fails, send a 404 and exit the function
  if (statResult == -1){
    send404(sockFd);
    return;
  }

  // Send the HTTP 200 message
  sendLine(sockFd, msg200);

  // Send the blank line
  sendLine(sockFd, empty);

  // Create regex patterns to identify file content type
  std::regex textPattern(R"(data/file\d\.html)");
  std::regex imagePattern(R"(data/image\d\.jpg)");

  // Send header line based off content type
  if (std::regex_match(filename, textPattern)){
    sendLine(sockFd, contentTypeText);
  }
  else if (std::regex_match(filename, imagePattern)){
    sendLine(sockFd, contentTypeImage);
  }
  else {
    send404(sockFd);
    return;
  }

  // Send the content length
  contentLength += std::to_string(fileInfo.st_size);
  sendLine(sockFd, contentLength);

  // Send a blank line
  sendLine(sockFd, empty);

  // Open the file
  int bytesWritten = 0;
  int openResult = open(filename.c_str(), O_RDONLY);

  if (openResult == -1){
    send404(sockFd);
    return;
  }

  // Write loop
  while (bytesWritten != fileInfo.st_size){
    // Clear out the buffer
    memset(buffer, 0, sizeof(buffer));

    // Read up to 10 bytes from the file into the memory buffer
    int readAmount = read(openResult, buffer, sizeof(buffer));

    // I the buffer is 0, exit the loop
    if (readAmount == 0){
      break;
    }
    else if (readAmount == -1){
      close(openResult);
      return;
    }

    // Write the number of bytes that have been read
    write(sockFd, buffer, readAmount);

    // Increase bytes written
    bytesWritten += readAmount;
  }

  // Close the opened file
  close(openResult);

  return;
}


// Process connection function
int processConnection(int sockFd) {
 
  // Create a fileName string to pass into readHeader()
  std::string fileName;

  // Int variable to store header return value
  int returnHeader = 0;

  // Call readHeader()
  returnHeader = readHeader(sockFd, fileName);

  // If read header returned 400, send 400
  if (returnHeader == 400){
    send400(sockFd);
  }

  // If read header returned 404, call send404
  if (returnHeader == 404){
    send404(sockFd);
  }

  // 471: If read header returned 200, call sendFile
  if (returnHeader == 200){
    sendFile(sockFd, fileName);
  }

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
