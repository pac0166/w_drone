/*Jetson Nano J41 Header
source:	https://www.jetsonhacks.com/nvidia-jetson-nano-j41-header-pinout/

 +-----+-----+---------+------+---+---Nano---+---+------+---------+-----+-----+
 |     |     |   Name  | Mode | V | Physical | V | Mode | Name    |     |     |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 |     |     |    3.3V |      |   |  1 || 2  |   |      |    5.0V |     |     |
 |     |     |i2c_2_SDA|      |   |  3 || 4  |   |      |    5.0V |     |     |
 |     |     |i2c_2_SCL|      |   |  5 || 6  |   |      |      0V |     |     |
 |     |     |GPIO 216 |      |   |  7 || 8  |   |      |UART_2_TX|     |     |
 |     |     |      0V |      |   |  9 || 10 |   |      |UART_2_RX|     |     |
 |     |     |GPIO  50 |      |   | 11 || 12 |   |      |GPIO  79 |     |     |
 |     |     |GPIO  14 |      |   | 13 || 14 |   |      |      0V |     |     |
 |     |     |GPIO 194 |      |   | 15 || 16 |   |      |GPIO 232 |     |     |
 |     |     |    3.3V |      |   | 17 || 18 |   |      |GPIO  15 |     |     |
 |     |     |GPIO  16 |      |   | 19 || 20 |   |      |      0V |     |     |
 |     |     |GPIO  17 |      |   | 21 || 22 |   |      |GPIO  13 |     |     |
 |     |     |GPIO  18 |      |   | 23 || 24 |   |      |GPIO  19 |     |     |
 |     |     |      0V |      |   | 25 || 26 |   |      |GPIO  20 |     |     |
 |     |     |i2c_1_sda|      |   | 27 || 28 |   |      |i2c_1_scl|     |     |
 |     |     |GPIO 149 |      |   | 29 || 30 |   |      |      0V |     |     |
 |     |     |GPIO 200 |      |   | 31 || 32 |   |      |GPIO 168 |     |     |
 |     |     |GPIO  38 |      |   | 33 || 34 |   |      |      0V |     |     |
 |     |     |GPIO  76 |      |   | 35 || 36 |   |      |GPIO  51 |     |     |
 |     |     |GPIO  12 |      |   | 37 || 38 |   |      |GPIO  77 |     |     |
 |     |     |      0V |      |   | 39 || 40 |   |      |GPIO  78 |     |     |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 |     |     |   Name  | Mode | V | Physical | V | Mode | Name    |     |     |
 +-----+-----+---------+------+---+---Nano---+---+------+---------+-----+-----+

--- 	SYSFS EXAMPLE:
source:	https://www.jetsonhacks.com/2019/06/07/jetson-nano-gpio/

	//Map GPIO Pin	//gpio79 is pin 12 on the Jetson Nano
$ echo 79 > /sys/class/gpio/export
	// Set Direction
$ echo out > /sys/class/gpio/gpio79/direction
	// Bit Bangin'!
$ echo 1 > /sys/class/gpio/gpio79/value
$ echo 0 > /sys/class/gpio/gpio79/value
	// Unmap GPIO Pin
$ echo 79 > /sys/class/gpio/unexport
	// Query Status all pins
$ cat /sys/kernel/debug/gpio

In the above code, the 79 refers to a translation of the Linux sysfs GPIO named gpio79.
If we look at the Jetson Nano J41 Header Pinout, we can see that gpio79 is physically
pin 12 of the header.		*/

#ifndef rom_wiringnano_h
#define rom_wiringnano_h

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <chrono>
#include <thread>

namespace rom {	//contain everything in ::rom to avoid naming collision

// **********************************************************************************
// **********************************************************************************
class file {			// create or open files; buffer them in ram; an use them if they where std::vector<uint8_t>
private:                	// This class is good handling Files that are smaler then 10MB
std::fstream 	fst;       	// It reads the whole file into ram and writes it out only
std::string 	filename;   	// in case of change
uint8_t 	is_unsync;      // private status flag: 0 if sync, 1 if buffer is newer, 2 if file is newer
std::vector<uint8_t> data;	// user data that should contain the entire file contents

void open(const std::string& mode = "in") {
fst.open(filename.c_str(),std::ios::out|std::ios::app); 	//This should create the file if it does not exist
fst.close();							//close the file
if (mode == "out")      {fst.open(filename.c_str(),std::ios::out);}	//open it again for writing
if (mode == "in")       {fst.open(filename.c_str(),std::ios::in);}	//open it again for reading
if (!fst.is_open()) {throw std::runtime_error("A file with name "+filename+" failed to open in Mode: "+mode);}
}

void close(void)        {fst.close();}		//close file

void flush(void) {			//synchronise data with file
if 	(!is_unsync) 	{return;}	//flush is fast if data is already syncronized
else if (is_unsync == 1) {      	//write buffer to file
        open("out");			//open file for writing
        fst.seekp(0,std::ios::beg);	//go to begin of file
        fst.write(reinterpret_cast<char*>(&(*data.begin())),data.size()); //write the whole data container
        }
else if (is_unsync == 2) {     		//read from file to buffer
	open("in");			//open file for reading
        fst.seekg (0, std::ios::end);	//go to end of file
        auto size{fst.tellg()};  	//get size of file
	data.resize(size);		//reserve enough memory in std::vector<uint8_t> data
        fst.seekg (0, std::ios::beg);	//go to begin of file
	//the next line is certainly not what the inventors of c++ want us to do, :-P
	//but it's fast, it works and a do not have to read hundreds of pages about std::fstream :-)
        fst.read(reinterpret_cast<char*>(&(*data.begin())), size);
        }
else 	{throw std::runtime_error("invalid value of member is_unsync in object of type ::rom::file");}
is_unsync = 0;			//updat the status of this object; it should be in sync by now
close();			//close file, we do not keep it open longer than necessary
}

public:
void reread(void) {	//get data from file and discard values in local buffer
is_unsync = 2;
flush();
}

void rewrite(void) {	//save local data to file
is_unsync = 1;
flush();
}

void save(void) {rewrite();}	//save local data to file

//there is no construction without reading the content of file to data container
//except if we are dealig with strange writeonly files from linux; yes it's true, files can be "writeonly"
file(const std::string& name = "time.log",uint8_t writeonly = 0):fst{},
	filename(name),is_unsync(writeonly?1:2),data{} {	//defaultname is "time.log"
flush();
}

~file(void) {flush();}	//file will be saved by destructor,at this time there is NO "closing without saving changes"

void push_back(uint8_t datain) {	//add one byte at the end of file
data.push_back(datain);
is_unsync = 1;
}

//delegating push_back() for container
void push_back(const std::vector<uint8_t>& datain) {for (auto a:datain) {push_back(a);}}

//delegating push_back() for container
void push_back(const std::string& stin) {for (auto ch:stin) {push_back(uint8_t(ch));}}

uint8_t& at(size_t pos) {	//get reference to one byte of the file at position pos
is_unsync = 1;
if (pos>=data.size())  {data.resize(pos+1);}	//increase size of data if pos is larger than current size of file
return data.at(pos);				//it may create some garbage data in the file!!!!
}

uint8_t at(size_t pos) const {	//get a copy of the one byte at pos
if (pos>=data.size()) {throw std::runtime_error("Trying to get item from file: "+filename+" that does not exist right now.");}
return data.at(pos);
}

void resize(size_t sz) {	//can be used for increasing and decreasing filesize
data.resize(sz);		//may delete some of its data
is_unsync = 1;
}

};//class rom_file
// **********************************************************************************
// **********************************************************************************


// **********************************************************************************
// **********************************************************************************
class wiringnano {// this class creates a representation for one GPIO Pin on Jeton Nano
private:
uint8_t wp;		//pinnumber for gpio;
uint8_t val;		//latest output value: 0->low  1->high  3->input
rom::file* exp;  	//the files are pointers to rom::file to get the best control of construction an destruction timing
rom::file* unexp;	//files for export and unexport gpios to sysf interface
rom::file* dirfile;	//file for controlling it's dirrection: output? input?
rom::file* valuefile;	//file for gpio value: 0 or 1

static const std::vector<uint8_t>& available_gpio(void) {//in class initialisation of static variables is strange in c++11 because
static std::vector<uint8_t> available_gpio_int{216,50,79,14,194,232,15,16,17,13,18,	//we have to wrap it inside a static member function
						19,20,149,200,168,38,76,51,12,77,78};	//and return it by const reference
return available_gpio_int;
}

static const std::vector<std::string>& available_directions(void) {
static std::vector<std::string> available_directions_int {"high","low","in","out"};
return available_directions_int;
}

static const std::string& export_path(void) {
static std::string export_path_int{"/sys/class/gpio/export"};
return export_path_int;
}

static const std::string& unexport_path(void) {
static std::string unexport_path_int{"/sys/class/gpio/unexport"};
return unexport_path_int;
}

std::string pinstr(void) 		{return std::to_string(wp);}	//get pinnumber as human readable std::string
std::string direction_path(void) 	{return std::string{"/sys/class/gpio/gpio"+pinstr()+"/direction"};}	//create a std::string that contains the entire path of file
std::string value_path(void) 		{return std::string{"/sys/class/gpio/gpio"+pinstr()+"/value"};}		//create a std::string that contains the entire path of file

void export_(void) {	//echo ?? > /sys/class/gpio/export
exp->resize(0);
exp->push_back(pinstr());
exp->rewrite();
}

void unexport_(void) {	//echo ?? > /sys/class/gpio/unexport
unexp->resize(0);
unexp->push_back(pinstr());
unexp->rewrite();
}

void set_direction(const std::string& dir) {	//echo out > /sys/class/gpio/gpio??/direction
for (auto& s:available_directions()) {		//echo in > /sys/class/gpio/gpio??/direction
	if (dir == s)	{
		dirfile->resize(0);
		dirfile->push_back(dir);
		dirfile->rewrite();
		return;
		}
	}
throw std::runtime_error("Direction: "+dir+" is not available for gpio"+pinstr()+".");
}

uint8_t read_value(void) {	//return value of file:  /sys/class/gpio/gpio79/value
valuefile->reread();		//reread from file on disk
return (valuefile->at(0)=='0')?0:1;
}

public:
wiringnano(const wiringnano&) = delete;			//no copy until now
wiringnano operator=(const wiringnano&) = delete;	//no copy until now

//for construction you have to provide gpio number; see table at the begin of this file
wiringnano(uint8_t nr):wp(nr),val{3},exp{nullptr},unexp{nullptr},dirfile{nullptr},valuefile{nullptr} {
if (std::find(available_gpio().begin(),available_gpio().end(),nr)==available_gpio().end())
	{throw std::runtime_error("Gpio: "+pinstr()+" is not available.");}
exp = new rom::file{export_path(),1};
unexp = new rom::file{unexport_path(),1};
export_();
dirfile = new rom::file(direction_path());
valuefile = new rom::file(value_path());
set_direction("in");
}

~wiringnano(void) {	//destruction will be automatic, files will be closed in the right order
set_direction("in");
delete valuefile;
delete dirfile;
unexport_();
delete unexp;
delete exp;
}

void pullhi() {	//set digital output to high voltage
if (val != 1)	{set_direction("high");}
val = 1;
}

void pulllo() {	//set digital output to high voltage
if (val != 0)	{set_direction("low");}
val = 0;
}

void flow() {	//set gpio pin to input mode without reading it's voltage level
if (val !=3) {set_direction("in");}
val =3;
}

void write(uint8_t bit) {	//write 0 or 1 to gpio pin
if (val != bit) {
	if (bit == 1) 		{pullhi();}
	else if (bit == 0)	{pulllo();}
}
}

uint8_t read() {	//read voltage level from gpio pin
flow();
return read_value();
}

};	//class wiringnano
// **********************************************************************************
// **********************************************************************************


//usage example function
void wiringnano_t(void) {	//simple demo of gpio output on jetson nano
wiringnano pin50{50};		//create a wiringnano gpio interface for gpio 79 == pin 12
wiringnano pin232{232};		//create a wiringnano gpio interface for gpio 232 == pin 16
				// (see table at begin of file)
std::cout <<  "Connect a Led to pin 12 of your Jetson-Nano and wire it via a 4k7 resistor to"	<< std::endl;
std::cout <<  "pin 6 (GND). Dim the lights of your room! It should be blinking for 20 seconds. "<< std::endl;
for (uint32_t i{0};i<20;++i){						//loop 10 times
	pin50.pulllo();							//send 0.0 Volt to pin 12
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));	//wait 0.25 sec.
	pin50.pullhi();							//send 3.3 Volt to pin 12
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));	//wait 0.25 sec.
	}
// std::cout << "We are stopping blinking now and measue the voltage on pin 16 "<< std::endl;
// std::cout << "Pin 16 is: " << (pin232.read()?"high":"low ") << std::endl; //read and output status of pin 16
// std::cout << "You can pull it high by connecting it via a 1k resistor to pin 1 (3.3V). "<< std::endl;
// std::cout << "You can pull it low by connecting it via a 1k resistor to pin 6 (GND). "<< std::endl;
// std::cout << "Do not make any direct connections without resistors!!! "<< std::endl;
std::cout << "This will probably cause some damege to your Jetson-Nano.  :-( "<< std::endl;
}

}	//namespace rom
#endif  //rom_wiringnano.h

