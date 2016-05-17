/** @file 01-grid
 *  @author Alessandro Saccoia <alessandro@alsc.co>
 */

#include "MonomeGrid.h"
#include <iostream>
#include <memory>


void usage();

// free-standing C callback: could have used a lambda or a function
void buttonPushed(int x, int y, MonomeGrid::ButtonState state);
void gridRefreshed();

using namespace std;

unique_ptr<MonomeGrid> monome;
MonomeGrid::LedState** currentLedStatus; // keep track of the current state

int main (int argc, char **argv) {
  if (argc != 4) {
     usage();
  }
  
  const char* monomeName = argv[1];
  int width = atoi(argv[2]);
  int height = atoi(argv[3]);
  
  currentLedStatus = new MonomeGrid::LedState*[width];
  for (int i = 0; i < width; ++i) {
    currentLedStatus[i] = new MonomeGrid::LedState[height];
  }
  
  monome.reset(new MonomeGrid(monomeName, width, height, buttonPushed, gridRefreshed));
  
  cout << endl << "Cycle buttons blink slow, blink fast, on and then off" << endl;
  
  // resets all leds to off
  monome->setAllLeds(MonomeGrid::LedState::LED_OFF);
  
  // resets the currentLedStatus map
  memset((void*)currentLedStatus, MonomeGrid::LedState::LED_OFF, width*height);
  
  // enters the infinite loop
  monome->loop();
}

// free-standing C callback: could have used a lambda or a function
void buttonPushed(int x, int y, MonomeGrid::ButtonState state) {
  if (currentLedStatus[x][y] == MonomeGrid::LED_OFF) {
    if (state == MonomeGrid::ButtonState::TOUCH_UP) {
      monome->setOneLed(x, y, MonomeGrid::LED_BLINK_SLOW);
    }
  } else if (currentLedStatus[x][y] == MonomeGrid::LED_BLINK_SLOW) {
    if (state == MonomeGrid::ButtonState::TOUCH_UP) {
      monome->setOneLed(x, y, MonomeGrid::LED_BLINK_FAST);
    }
  } else if (currentLedStatus[x][y] == MonomeGrid::LED_BLINK_FAST) {
    if (state == MonomeGrid::ButtonState::TOUCH_UP) {
      monome->setOneLed(x, y, MonomeGrid::LED_ON);
    }
  } else if (currentLedStatus[x][y] == MonomeGrid::LED_ON) {
    if (state == MonomeGrid::ButtonState::TOUCH_UP) {
      monome->setOneLed(x, y, MonomeGrid::LED_OFF);
    }
  }
}

// free-standing C callback: could have used a lambda or a function
void gridRefreshed() {
 // do nothing
}


void usage() {
  std::cout << "Usage:" << std::endl
     << "01-grid monomeName width height" << std::endl
     << "monomeName = /dev/tty.usbserial-m40h0351" << std::endl
     << "height = 8 for a 40h" << std::endl
     << "width = 8 for a 40h" << std::endl;
  exit(1);
}