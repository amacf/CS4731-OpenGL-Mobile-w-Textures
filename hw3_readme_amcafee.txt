CS4731 Homework 3
Andrew McAfee
12/2/2016

The structure of my implementation is very simple. Everything is implemented in the hw3.cpp file. A number of provided files are also used, including InitShader.cpp, Angel.h, and mat.h. No changes have been made to these provided files. The structure of hw3.cpp is as follows: putAllPLYInBuffers is called, which reads 6 PLY files and puts the triangle vertices into a buffer, as well as the face normals into another buffer. Points for "arms" are calculated and put in a seperate buffer.A CTM is maintained and updated in the display function using updateCTM, PushMatrix, and PopMatrix. GLUT callbacks are used to capture mouse and keyboard information that changes state information and tells OpenGL to redraw.


To compile and run this program, simply use Visual Studio 2010 default features as configured on the Zoolab machines. Open the file Visual Studio solution file "Homework3.sln". The program should build and run out of the box simply by clicking the "Start Debugging" button, or by any other method within Visual Studio 2010.
