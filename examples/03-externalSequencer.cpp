/** @file 03-externalSequencer
 *  @author Alessandro Saccoia <alessandro@alsc.co>
 *
 *  Shows how to visualize data that is happening in another thread, in this case 
 *  the beat count
 */

#include "MonomeGrid.h"
#include <iostream>
#include <memory>
#include <chrono>

#define BPM 120.0F
#define MS_PER_BEAT (int)((60.F / BPM) * 1000)

void usage();

// free-standing C callback: could have used a lambda or a function
void buttonPushed(int x, int y, MonomeGrid::ButtonState state);
void gridRefreshed();

using namespace std;

unique_ptr<MonomeGrid> monome;
int currentStep = 0;
bool isSequencerRunning = false;
std::chrono::system_clock::time_point current_time;
int width;

void sequencer() {
  while (true) {
    currentStep += 1;
    currentStep %= width;
    std::this_thread::sleep_for(std::chrono::milliseconds(MS_PER_BEAT));
  }
}

int main (int argc, char **argv) {
  if (argc != 4) {
     usage();
  }
  
  const char* monomeName = argv[1];
  width = atoi(argv[2]);
  int height = atoi(argv[3]);
  
  monome.reset(new MonomeGrid(monomeName, width, height, buttonPushed, gridRefreshed));
  
  cout << endl << "Shows the beat of a sequencer running in another thread" << endl;
  
  // resets all leds to off
  monome->setAllLeds(MonomeGrid::LED_OFF);
  
  // enters the infinite loop
  monome->loop();
}

// free-standing C callback: could have used a lambda or a function
void buttonPushed(int x, int y, MonomeGrid::ButtonState state) {

}

// free-standing C callback: could have used a lambda or a function
void gridRefreshed() {
  static int lastStep = -1;
  if (lastStep != currentStep) {
    monome->setAllLeds(MonomeGrid::LED_OFF);
    monome->setRow(currentStep, MonomeGrid::LED_ON);
  }
}


void usage() {
  std::cout << "Usage:" << std::endl
     << "03-externalSequencer monomeName width height" << std::endl
     << "monomeName = /dev/tty.usbserial-m40h0351" << std::endl
     << "height = 8 for a 40h" << std::endl
     << "width = 8 for a 40h" << std::endl;
  exit(1);
}