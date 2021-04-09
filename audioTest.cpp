
//COMPILE - 
//  sudo make install   -in previous folders
//  sudo apt-get install libboost1.58-all    -for boost filesystem

//	compile code:
//  g++ gettingstarted.cpp -o gettingstarted -lrf24 -pthread -lboost_system -lboost_filesystem
//  -pthread for threads
// -l(L)rf24 is to link the RF24 library to the compiler
// -lboost_system and -lboost_filesystem for boost filesystem

//aplay (and killall aplay)
//arecord
//amixer set Master 50%
//without the % is it out of 65565(?) (short max)



#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <string>      // string, getline()
#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
#include <fstream>	   //for loading files
#include <sstream>	   //for reading files
#include <vector>
#include <RF24/RF24.h> // RF24, RF24_PA_LOW, delay()
#include <bits/stdc++.h>
#include <thread>

//all for system commands 
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

//for file system (TODO LATER)
#include <boost/filesystem.hpp>
//or add -DBOOST_NO_CXX11_SCOPED_ENUMS to compiler flags

using namespace std;
using namespace boost::filesystem;

RF24 radio(22, 0);

char MAX_PAYLOAD_SIZE = 32;
bool newMsg = false;
int RADIO_WRITE_ATTEMPTS = 30;

//MUSIC VARIABLES
std::vector <std::string> playlists;
std::string binarySelectors = "";
std::string buttonStates = "";
std::string potValue = "";
int songNum = 0;
int volume = 30;
std::string playlistName = "test";
std::vector<std::string> playlist;
bool songPlaying = false;
int buttonPressed = -1;
bool musicActive = false; //main isActive var




bool radioWrite(std::string strIn);
void radioRead(char *payloadRx);
void setReadingPipe(std::string strIn);
void setWritingPipe(std::string strIn);
void createPlaylist(std::string playlistName);
std::string cmdExec(std::string in);
void setVolume(int vol);
void musicPlayer(int songNum);
void musicController();
void createPlaylistFile(std::string fileName);
uint32_t getMicros();
std::vector<string> split(std::string str1, char splitChar);
int chooseFromList(std::vector<string> vectorIn);
std::string splitLast(std::string strIn, char chr);
std::string chooseFilePath(std::string initDirectory);
int isFileOrDirectory(std::string inputPath);



struct timespec startTimer, endTimer;


int main(int argc, char** argv) {
	if(cmdExec("bluetoothctl connect EB:06:EF:B6:61:E9").find("Failed")){
		
	}
	else if(cmdExec("bluetoothctl connect 22:05:21:0B:6A:7B").find("Failed")){
			
	}
	else if(cmdExec("bluetoothctl connect 81:5C:58:37:02:B9").find("Failed")){
			
	}
	
	
	createPlaylistFile("list2");
	//createPlaylist("list2");
	/*
	Bluetooth headphones: EB:06:EF:B6:61:E9 Jlab Neon BT
	Car speaker: 81:5C:58:37:02:B9 Oont Angle 3 2B9
	wireless earbuds: 22:05:21:0B:6A:7B TOZO-T6
	*/
	
	
    if (!radio.begin()) { // check hardware
        cout << "radio hardware is not responding!!" << endl;
        return 0; // quit now
    }
    radio.setPayloadSize(MAX_PAYLOAD_SIZE);
    radio.setPALevel(RF24_PA_LOW); // RF24_PA_MAX is default.
	//CoNtroLer 1, Uplink (pi to module)
	setWritingPipe("CRL01U");
	//CoNtroLer 1, Downlink (module to pi)
	setReadingPipe("CRL01D");
    // For debugging info
    // radio.printDetails();       // (smaller) function that prints raw register values
    // radio.printPrettyDetails(); // (larger) function that prints human readable data

	std::thread musicCtrl(musicController);
	musicCtrl.detach();
	
	radio.startListening(); 
	
	while(true){
		newMsg = false;
		while(radio.available()){
			char radioIn[MAX_PAYLOAD_SIZE-1];
			radioRead(radioIn);
			std::string recievedStr = (std::string)radioIn;
			//TODO - once adding more modules, check which module msg is from
			//parse msg
			//first 4 are top binary selector states
			//next 7 are button states (Top left - bottom right)
			//to end of string is pot value (1-3 chars (0-100))
			if(recievedStr[0] == 'D'){ //if data is being sent
			recievedStr = recievedStr.substr(1); //chop off the first letter
				//binarySelectors = recievedStr.substr(0,4); SWITCHES ARE BROKEN
				buttonStates = recievedStr.substr(0,7);
				potValue = recievedStr.substr(7);
				newMsg = true;
			}
		}
		if(newMsg){
			//cout << "switches: " << binarySelectors << " ";
			//cout << "buttons: " << buttonStates << " ";
			//cout << "POT: " << potValue << " ";
			//get state from binary selector
			
			//TODO - switch based on state
			//assuming it's music for now (ignoring them for now)
			
			//check which button is pressed
			if(buttonStates[0] == '1'){//Navigation Left
				buttonPressed = 0;
				//cout << "pressed 0" << endl;
			}
			if(buttonStates[1] == '1'){//Navigation Select
				buttonPressed = 1;
			}
			if(buttonStates[2] == '1'){//Navigation Right
				buttonPressed = 2;
			}
			if(buttonStates[3] == '1'){//prev song
				buttonPressed = 3;
			}
			if(buttonStates[4] == '1'){//stop
				buttonPressed = 4;
			}
			if(buttonStates[5] == '1'){//play
				buttonPressed = 5;
			}
			if(buttonStates[6] == '1'){// next song
				buttonPressed = 6;
			}
			//TODO - update volume
			std::string::size_type st;
			volume = std::stoi(potValue, &st);
			//cout << "Volume: " << volume << endl;
			setVolume(volume);
		}
		
	}
	radio.stopListening();
	
	
    return 0;
}

bool radioWrite(std::string strIn) {
    radio.stopListening();                                          // put radio in TX mode

	char payload[MAX_PAYLOAD_SIZE];
	std::string s = "Hello World!";
    strcpy(payload, strIn.c_str());
	bool returnMe = false;
	//cout << "Sending: " << payload << endl;
	
        clock_gettime(CLOCK_MONOTONIC_RAW, &startTimer);            // start the timer
        bool isSent = radio.write(&payload, sizeof(payload));         // transmit & save the report
        uint32_t timerEllapsed = getMicros();                       // end the timer
		
        if (isSent) {
            // payload was delivered
            cout << "Transmission successful! Time to transmit = ";
            cout << timerEllapsed;                                  // print the timer result
            cout << " us. Sent: " << payload << endl;               // print payload sent
            returnMe = true;                                       // increment float payload
        } else {
			for(int i = 0; i < RADIO_WRITE_ATTEMPTS; i++){
				bool isSent = radio.write(&payload, sizeof(payload));
				if(isSent){break;}
				
			}
            // payload was not delivered
            cout << "radioWrite - Transmission failed or timed out" << endl;
			returnMe = false;
        }
		
		radio.startListening();
		return returnMe;
		
}

void radioRead(char *payloadRx) {

    time_t startTimer = time(nullptr);                       // start a timer
        uint8_t pipe;
        if (radio.available(&pipe)) {
            radio.read(payloadRx, MAX_PAYLOAD_SIZE);
			//cout << "recieved: " << payloadRx << endl;
            startTimer = time(nullptr); //reset timer
        }
}

uint32_t getMicros() {
    // this function assumes that the timer was started using
    // clock_gettime(CLOCK_MONOTONIC_RAW, &startTimer);`

    clock_gettime(CLOCK_MONOTONIC_RAW, &endTimer);
    uint32_t seconds = endTimer.tv_sec - startTimer.tv_sec;
    uint32_t useconds = (endTimer.tv_nsec - startTimer.tv_nsec) / 1000;

    return ((seconds) * 1000 + useconds) + 0.5;
}

void setWritingPipe(std::string strIn){
	uint8_t write[6];
	std::copy(strIn.begin(), strIn.end(), std::begin(write));
	cout << "setWritingPipe - opened writing pipe: " << write << endl;
	radio.openWritingPipe(write); 
}

void setReadingPipe(std::string strIn){
	uint8_t read[6];
	std::copy(strIn.begin(), strIn.end(), std::begin(read));
	cout << "setReadingPipe - opened reading pipe: " << read << endl;
	radio.openReadingPipe(1, read); 
}

void createPlaylist(std::string playlistName){
	//find playlistName.plst
	// if .plst doesnt exist, read and create from playlistName.txt
	//  if .txt doesn't exist, throw error
	
		//TODO - check if file exists
		
		try{
			std::string playlistFullFileName = "/home/pi/Music/playlists/" + playlistName + ".plst";
			std::ifstream infile(playlistFullFileName);
			std::string line;
			int linesRead = 0;
			//cout << "file lines:" << endl;
			playlist.clear();
			while (std::getline(infile, line)) //get each line (never gets run if file is empty)
			{
				linesRead++;
				playlist.push_back(line);//add each line to the playlist in memory
				//cout << "line " << linesRead << ": " << line << endl; //print it cause why not
			}
			if(linesRead == 0){ //if the file doesnt exist, create it from the .txt file
				cout << "createPlaylist - File does not exist!" << endl;
				//create .plst from .txt and read from that
				//TODO - this
			}
			infile.close();
			
			//now playlist is full of file locations of songs
		}
		catch(std::ifstream::failure e){
			cout << "error opening file " << endl;
		}
		
		

		
		
}

std::string cmdExec(std::string in) {
	const char* cmd = in.c_str();
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

void setVolume(int vol){
	std::string temp = "amixer set Master " + std::to_string(vol) + "%";
	cmdExec(temp);
}

void musicController(){
	//check for which button is pressed
	//make playlists chooseable
	delay(100);
	radioWrite("DSPress a button...");
	delay(10);
	int songNum = 0;
	while(songNum <= playlist.size()){
		//main repeat controller
		if(!songPlaying && musicActive && songNum < playlist.size()-1){
			songNum++;
			std::thread song_1(musicPlayer, songNum);
			song_1.detach();
			delay(100);
		}
		//button controls here
		
		switch(buttonPressed){
			case -1:
			break;
			//0,1,and 2 are reserved for file navigation
			
			case 3:{//prev song
			musicActive = true;
			cmdExec("killall aplay");
			if(songNum > 0){songNum--;}
			std::thread song3(musicPlayer, songNum);
			song3.detach();
			radioWrite("DSPrev");
			cout << "prev"<< endl;
			delay(50);
			buttonPressed = -1;
			}
			break;
			
			case 4:{//stop
			radioWrite("DSStop");
			cmdExec("killall aplay");
			buttonPressed = -1;
			}
			break;
			
			case 5:{//play
			radioWrite("DSPlay");
			cmdExec("killall aplay");
			std::thread song5(musicPlayer, songNum);
			song5.detach();
			delay(100);
			buttonPressed = -1;
			}
			break;
			
			case 6:{//next song
			radioWrite("DSNext");
			if(songNum < playlist.size()-1){
				songNum++;
				cmdExec("killall aplay");
				std::thread song6(musicPlayer, songNum);
				song6.detach();
			}
			delay(100);
			buttonPressed = -1;
			}
			break;
			case 10:
			terminate();
			break;
			
			default:
			break;
		}
		
		//cout << chooseFilePath("/home/pi") << endl;
	}

}

void musicPlayer(int songNum){
	songPlaying = true;
	cmdExec("killall aplay");
	std::string temp = "aplay " + playlist[songNum];
	cout << cmdExec(temp) << endl;
	songPlaying = false;
}

void createPlaylistFile(std::string filename){
	//read each line
	string line;
	std::string modifiedFileName = "/home/pi/Music/playlists/"+filename+".txt";
	std::string rootFile = "/home/pi/Music/";
	ifstream myfile (modifiedFileName);
	if (myfile.is_open())
	{
		while ( getline (myfile,line) )
		{		
			//split line into folders
			std::vector<std::string> folders;
			
			std::string lowerWord = "";
			std::locale loc;
			for(int i = 0; i < line.length(); i++){
				if(line[i] == '-'){//split char
					//add it to folders
					folders.push_back(lowerWord);
					lowerWord = "";
				}
				else if(line[i] == ' '){//convert spaces to underscores
					lowerWord += '_';
				}
				else{//otherwise make every character lowercase
					lowerWord += std::tolower(line[i],loc);
				}
				
			}
			folders.push_back(lowerWord);
			lowerWord = "";
			
			//make into folders
			std::string filepath = rootFile;
			for(int i = 0; i < folders.size(); i++){
				filepath += folders[i] + "/";
			}
			
			
			//take the last character out
			filepath = filepath.substr(0,filepath.length()-2);//to account for the extra slash and the new line char
			//add .wav
			filepath += ".wav";
			
			//cout << "createPlaylistFile - filepath is " + filepath << endl;
			
			//go into respective folders and see if file exists
			if(boost::filesystem::exists(filepath)){
				//if it does add it to the end file
				//cout << "createPlaylistFile - adding " + filepath << endl;
				ofstream myfile;
				std::string plstFile = rootFile + "playlists/" + filename + ".plst";
				//cout << "createPlaylistFile - filename: " << plstFile << endl;
				myfile.open (plstFile, std::ofstream::out | std::ofstream::app);
				myfile << filepath << endl;
				myfile.close();
			}
			//if it doesnt throw an error and continue to next line
			else{
				cout << "createPlaylistFile - file: " + filepath + " doesn't exist, skipping." << endl;
			}
			
			
		}
		myfile.close();
	}
	else cout << "Unable to open file"; 
	
	
}

int chooseFromList(std::vector<string> vectorIn){
	//L is left
	//R is right
	//space is select
	//add 30s timeout (make variable)
	
	int posInList = 0;
	radioWrite("Selected: \n" + vectorIn[posInList]);
	while(true){
		if(buttonPressed == 0 && posInList == 0){
			posInList = -1;
			cout << "Selected: BACK" << endl;
			buttonPressed = -1;
			radioWrite("DSSelected: \nBACK");
			delay(10);
		}
		else if(buttonPressed == 0 && posInList > 0){
			posInList--;
			cout << "Selected: " + vectorIn[posInList] << endl;
			buttonPressed = -1;
			radioWrite("DSSelected \n" + vectorIn[posInList]);
			delay(10);
		}
		else if(buttonPressed == 2 && (posInList < vectorIn.size()-1 || posInList == -1)){
			posInList++;
			if(posInList >= 0){
				cout << "Selected: " + vectorIn[posInList] << endl;
				radioWrite("DSSelected \n" + vectorIn[posInList]);
			}
			else{
				cout << "Selected: \nBACK" << endl;
				radioWrite("DSSelected \nBACK");
			}
			buttonPressed = -1;
			
			delay(10);
		}
		else if(buttonPressed == 1){
			if(posInList >= 0){
				cout << "Selected: \nBACK" << endl;
				radioWrite("DSSelected \nBACK");
			}
			else{
				cout << "Selected: " + vectorIn[posInList] << endl;
				radioWrite("DSSelected \n" + vectorIn[posInList]);
			}
			buttonPressed = -1;
			
			delay(10);
			break;
		}
		
	}
	if(posInList >= 0){
		cout << "Selected: " + vectorIn[posInList]" << endl;
	}
	return posInList;
	
}

std::string chooseFilePath(std::string initDirectory){ //input MUST NOT have slash at end of string
	string finalPath = initDirectory + "/";
	while(true){
		string cmdInput = "cd " + finalPath + "; ls";//get to the music directory first
		vector<string> dirList = split(cmdExec(cmdInput), '\n'); //ls consistantly uses \n, dir uses inconsistant spaces/line feeds(\n)
		
		//
		//DO DECISION LOGIC HERE
		//
		int input = chooseFromList(dirList);
		cout << "---------------------" << endl;
		if(input == -1){
			finalPath = splitLast(finalPath, '/');
		}
		else if(input <= dirList.size() && input >= 0){
			finalPath += dirList[input];
			int fileOrDirectory = isFileOrDirectory(finalPath);
			if(fileOrDirectory == 1){ //if a file
				//found the file, exit and output file path
				cout << finalPath << endl;
				break;
			}
			else if(fileOrDirectory == 2){ //if a directory
				finalPath += "/"; // add a slash and continue
			}
			//if its a directory add a '/', if not, exit loop
		}
		else{
			cout << "invalid, please try again" << endl;
		}
		cout << "current path -> " + finalPath << endl;
	}
	cout << "final path -> " + finalPath << endl;
	return finalPath;
	
}
	
int isFileOrDirectory(std::string inputPath){
	
	//returns 1 if a file
	//returns 2 if directory
	//returns 3 if neither
	//returns 0 if does not exist
	//returns -1 if all failed somehow
	
	path p (inputPath);   // p reads clearer than argv[1] in the following code

  if (exists(p))    // does p actually exist?
  {
    if (is_regular_file(p)){        // is p a regular file?   
      //cout << p << " size is " << file_size(p) << '\n';
	  return 1;
	}

    else if (is_directory(p)){      // is p a directory?
      //cout << p << " is a directory\n";
	  return 2;
	}

    else{
      //cout << p << " exists, but is neither a regular file nor a directory\n";
	  return 3;
	}
  }
  else{
    //cout << p << " does not exist\n";
	return 0;
  }

  return -1;
	
}

std::vector<string> split(std::string str1, char splitChar){
	string str = str1; //we're going to be slowly removing this string, so make a copy first
	vector<string> vectorOut;
	while(str.find(splitChar) != string::npos){
		vectorOut.push_back(str.substr(0,str.find(splitChar)));
		str = str.substr(str.find(splitChar)+1);
	}
	return vectorOut;
}

std::string splitLast(std::string strIn, char chr){ //doesn't consider the very first character
	for(int i = strIn.length()-2; i > 0; i--){
		if(strIn[i] == chr){
			return strIn.substr(0,i);
		}
	}
	return "-1";
}

	//TODO - make choosable audio output (including audio jack)
	
	//TODO - create seperate function for updating playlists on the fly
	
	
	
	
	
	