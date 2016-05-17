/** @file 02-sequencer
 *  @author Alessandro Saccoia <alessandro@alsc.co>
 *
 *  Shows a little sequencer made using the internal callback
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
int currentStep = 0;;
bool isSequencerRunning = false;
std::chrono::system_clock::time_point current_time;
int width;

int main (int argc, char **argv) {
  if (argc != 4) {
     usage();
  }
  
  const char* monomeName = argv[1];
  width = atoi(argv[2]);
  int height = atoi(argv[3]);
  
  monome.reset(new MonomeGrid(monomeName, width, height, buttonPushed, gridRefreshed));
  
  cout << endl << "Shows a sequencer: press any button to start, any button to stop" << endl;
  
  // resets all leds to off
  monome->setAllLeds(MonomeGrid::LED_OFF);
  
  // enters the infinite loop
  monome->loop();
}

// free-standing C callback: could have used a lambda or a function
void buttonPushed(int x, int y, MonomeGrid::ButtonState state) {
  isSequencerRunning = !isSequencerRunning;
  if (isSequencerRunning) {
    currentStep = 0;
    current_time = std::chrono::system_clock::now();
  } else {
    monome->setAllLeds(MonomeGrid::LED_OFF);
  }
}

// free-standing C callback: could have used a lambda or a function
void gridRefreshed() {
  if (!isSequencerRunning) {
    return;
  }
  std::chrono::system_clock::time_point time_now = std::chrono::system_clock::now();
  if (time_now > current_time + std::chrono::milliseconds(MS_PER_BEAT)) {
    monome->setAllLeds(MonomeGrid::LED_OFF);
    current_time += std::chrono::milliseconds(MS_PER_BEAT);
    currentStep += 1;
    currentStep %= width;
  }
  monome->setRow(currentStep, MonomeGrid::LED_ON);
}


void usage() {
  std::cout << "Usage:" << std::endl
     << "02-sequencer monomeName width height" << std::endl
     << "monomeName = /dev/tty.usbserial-m40h0351" << std::endl
     << "height = 8 for a 40h" << std::endl
     << "width = 8 for a 40h" << std::endl;
  exit(1);
}