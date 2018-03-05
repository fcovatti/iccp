Objective:
-------
This project aims to develop an ICCP Client that connects to a SCADA ICCP Server (http://www.sage.cepel.br/) and send the data to an HMI client (http://sourceforge.net/projects/oshmiopensubstationhmi/).

Compilation:
-------
The source code was developed in C language.
For compilation, it is necessary to have the libiec61850 Library (http://libiec61850.com/).


License:
-------
ICCP itself is released under the GPLv3. The complete license text can be found at http://www.gnu.org/licenses/gpl.html.
If you want to use this library in projects where GPL is not an option you can also purchase a commercial license. Commercial Licenses and support is provided by fcovatti@gmail.com.


Example:
-------
In order to test the client it is necessary to have an ICCP running server. This client was tested communicating with the Sage ICCP server (sage.cepel.br).

cd /tmp
git glone https://github.com/fcovatti/libiec_iccp_mod.git
git clone https://github.com/fcovatti/iccp.git
cd lib_iccp_mod
make
cd ../iccp
make

The client is compiled under iccp/client folder with the name iccp_client

If you want to compile for windows platforms instead of "make" use the command:
make TARGET=WIN32

The two main configuration files are iccp_config.txt and point_list.txt:

In the iccp_config.txt it is necessary to set the ip of the iccp server, For example:
SERVER_NAME_1="10.0.0.1"
Also the iccp_client is suposed to send the data to an local or remote Human Machine Interface (HMI). Ex:
IHM_ADDRES=''127.0.0.1"

The point_list.txt contains the name of the points that exist on the iccp_server
The columns NPONTO and ID should be unique. NPONTO is used between the iccp client and HMI. The ID is the point Identifier in the iccp server
The column TIPO contains the type of data (A:analog or D:digital). If more information is required, please seee the documentation about the other columns on the oshmi project.

To see the data received from the server it is possible to run the command line HMI in the iccp/ihm directory or use a graphical interface https://sourceforge.net/projects/oshmiopensubstationhmi/files/

The hist directory contains an historian, so it is possible to write the received data to a mysql database instead of sending to an HMI interface
