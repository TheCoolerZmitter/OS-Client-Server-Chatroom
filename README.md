To make the files in an Ubuntu Linux terminal, "make" must be installed ("sudo apt install make" or "sudo apt install build-essential"), then type "make".
Alternatively, you can run the commands in the makefile manually.
Once the files are made, enter "./main_server" to start the server program.

In another window, enter "./main_client 1004" to start a client program.  The first instance will automatically put you in a new room, but any additional
client programs you run will bring you to a room selection screen where you can choose to join an existing room or start a new one.
Each client can choose a username and is given a random text color that will be seen by other users when they chat in the same room.
