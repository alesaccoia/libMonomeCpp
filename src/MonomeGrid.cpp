/** @file MonomeGrid.cpp
 *  @author Alessandro Saccoia <contact@alsc.co>
 *  @date 3/25/16
 *  @license Public Domain
 */

#include "MonomeGrid.h"

#include <cmath>

#define LONG_PRESS_TIME 0.5F
#define BLACK_MAGIC 135246



struct MonomeCommand {
  enum CommandType { SET_LED, ALL_LEDS, SET_COLUMN, SET_ROW } type;
  union {
    struct {
      int x;
      int y;
      MonomeGrid::LedState state;
    } setLedArgs;
    
    MonomeGrid::LedState allLedsState;
  } args;
};

void handle_press(const monome_event_t *e, void *data) {
  ((MonomeGrid*)data)->buttonTouched(e->grid.x, e->grid.y, e->event_type == MONOME_BUTTON_DOWN);
}


/// -------------


MonomeGrid::MonomeGrid(
  const char* monomeName_
  , unsigned int width_
  , unsigned int height_
  , std::function<void(int, int, ButtonState)> cb_
  , std::function<void(void)> refreshCb_)
  : mWidth(width_)
  , mHeight(height_)
  , mButtonsCb(cb_)
  , mRefreshCb(refreshCb_) {
  
  if( !(mMonome = monome_open(monomeName_)) )
		throw std::runtime_error("Impossible to open monome");
  
  mGrid = new MonomeCell*[mWidth*mHeight];
  for (int i = 0; i < mHeight; ++i) {
    mGrid[i] = new MonomeCell[mWidth];
  }
  
  monome_led_all(mMonome, 0);
  monome_set_rotation(mMonome, MONOME_ROTATE_90);
  
  // slow
  blinkingSpeeds[0].bits = 0x20;
  blinkingSpeeds[0].log2Bits = log2(blinkingSpeeds[0].bits);
  blinkingSpeeds[1].bits = 0x8;
  blinkingSpeeds[1].log2Bits = log2(blinkingSpeeds[1].bits);
  
	monome_register_handler(mMonome, MONOME_BUTTON_DOWN, handle_press, this);
	monome_register_handler(mMonome, MONOME_BUTTON_UP, handle_press, this);
  
  assert(TPCircularBufferInit(&mCommandsBuffer, 16384));
  
  mMonomeThread = std::thread([=] { updateGrid(); });
}

MonomeGrid::~MonomeGrid() {
  monome_close(mMonome);
  for (int i = 0; i < mWidth; ++i) {
    delete [] mGrid[i];
  }
  delete [] mGrid;
}
  
void MonomeGrid::loop() {
  while(1) {
    while(monome_event_handle_next(mMonome));
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
}

void MonomeGrid::resetBlinkingBits(int x, int y) {
  static int currentLedState, currentBlinkState;
  currentLedState = mGrid[x][y].ledState & 0x03;
  int currentSpeed = (currentLedState == LED_BLINK_FAST) ? blinkingSpeeds[1].bits : blinkingSpeeds[0].bits;
  int logSpeed = (currentLedState == LED_BLINK_FAST) ? blinkingSpeeds[1].log2Bits : blinkingSpeeds[0].log2Bits;
  currentBlinkState = currentSpeed - 1;
  currentBlinkState |= 1 << logSpeed;
  currentBlinkState = currentBlinkState << 2;
 
  //clear and reset all relevant bits
  mGrid[x][y].ledState &= ~mGrid[x][y].ledState;
  mGrid[x][y].ledState |= currentLedState;
  mGrid[x][y].ledState |= currentBlinkState;
}

void MonomeGrid::handleLedBlinkLinear(int x, int y) {
  static int currentLedState, currentBlinkState;
  currentLedState = mGrid[x][y].ledState & 0x03;
  int currentSpeed = (currentLedState == LED_BLINK_FAST) ? blinkingSpeeds[1].bits : blinkingSpeeds[0].bits;
  int logSpeed = (currentLedState == LED_BLINK_FAST) ? blinkingSpeeds[1].log2Bits : blinkingSpeeds[0].log2Bits;
  
  // shift right to remove the state bits
  currentBlinkState = (mGrid[x][y].ledState >> 2) & (currentSpeed - 1);
  
  // the bit to the left of the ones used for the brightness mask tells the direction
  int currentDirection = (mGrid[x][y].ledState >> 2) & currentSpeed ? 1 : -1;
  
  // change state if the current value is one of the two extremes (0 and (currentSpeed - 1)
  // also change the direction
  if (currentBlinkState == 0) {
    monome_led_set(mMonome, x, y, 0);
    currentDirection = 1;
  } else if (currentBlinkState == (currentSpeed - 1)) {
    monome_led_set(mMonome, x, y, 1);
    currentDirection = -1;
  }
  if (currentDirection == 1) {
    currentBlinkState |= 1 << logSpeed;
  } else {
    currentBlinkState &= ~(1 << logSpeed);
  }
  // refresh the brightness level
  currentBlinkState += currentDirection;
  // shift by two to the left
  currentBlinkState = currentBlinkState << 2;
  
  //clear and reset all relevant bits
  mGrid[x][y].ledState &= ~mGrid[x][y].ledState;
  mGrid[x][y].ledState |= currentLedState;
  mGrid[x][y].ledState |= currentBlinkState;
}

void MonomeGrid::updateGrid() {
  static int currentLedState, lastLedState;
  while(1) {
    // calls the refresh cb to give the chance to the user to do something extra
    mRefreshCb();
    
    // check the buttons and see if some button shall send the 
    
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    
    for (int x = 0; x < mWidth; ++x)
      for (int y = 0; y < mHeight; ++y) {
        if (mGrid[x][y].buttonState == TOUCH_DOWN) {
          if (std::chrono::duration_cast<std::chrono::seconds>(now - mGrid[x][y].buttonDownTime).count() > LONG_PRESS_TIME) {
            mGrid[x][y].buttonState = TOUCH_LONG;
            mButtonsCb(x,y,TOUCH_LONG);
          }
        }
      }
      
    // Executes the commands
    int numReadBytes;
    MonomeCommand* readCommands = (MonomeCommand*) TPCircularBufferTail(&mCommandsBuffer, &numReadBytes);
    if (readCommands != NULL) {
      assert((numReadBytes % sizeof(MonomeCommand)) == 0);
      int ci = 0;
      int numCommands = numReadBytes / sizeof(MonomeCommand);
      while (ci < numCommands) {
        switch (readCommands[ci].type) {
          case MonomeCommand::ALL_LEDS:
            for (int x = 0; x < mWidth; ++x)
              for (int y = 0; y < mHeight; ++y) {
                if (((mGrid[x][y].ledState) & 0x03) != readCommands[ci].args.allLedsState)
                  mGrid[x][y].ledState = readCommands[ci].args.allLedsState;
              }
            break;
          case MonomeCommand::SET_LED:
            if (((mGrid[readCommands[ci].args.setLedArgs.x][readCommands[ci].args.setLedArgs.y].ledState) & 0x03) != readCommands[ci].args.setLedArgs.state)
              mGrid[readCommands[ci].args.setLedArgs.x][readCommands[ci].args.setLedArgs.y].ledState = readCommands[ci].args.setLedArgs.state;
            break;
          case MonomeCommand::SET_ROW:
            for (int x = 0; x < mWidth; ++x)
              if (((mGrid[x][readCommands[ci].args.setLedArgs.y].ledState) & 0x03) != readCommands[ci].args.setLedArgs.state)
                mGrid[x][readCommands[ci].args.setLedArgs.y].ledState = readCommands[ci].args.setLedArgs.state;
            break;
          case MonomeCommand::SET_COLUMN:
            for (int y = 0; y < mHeight; ++y)
              if (((mGrid[readCommands[ci].args.setLedArgs.x][y].ledState) & 0x03) != readCommands[ci].args.setLedArgs.state)
                mGrid[readCommands[ci].args.setLedArgs.x][y].ledState = readCommands[ci].args.setLedArgs.state;
            break;
          default:
            assert(false && "Unknown command");
        }
        ++ci;
      }
      TPCircularBufferConsume(&mCommandsBuffer, numReadBytes);
    }
    
    // communicates the changes to the physical Monome
    
    for (uint8_t x = 0; x < mWidth; ++x) {
      for (uint8_t y = 0; y < mHeight; ++y) {
        if (mGrid[x][y].ledState != mGrid[x][y].lastLedState) {
          currentLedState = mGrid[x][y].ledState & 0x03;
          lastLedState = mGrid[x][y].lastLedState & 0x03;
          switch (currentLedState) {
            case LED_OFF:
              monome_led_set(mMonome, x, y, 0);
              mGrid[x][y].lastLedState = mGrid[x][y].ledState;
              break;
            case LED_ON:
              monome_led_set(mMonome, x, y, 1);
              mGrid[x][y].lastLedState = mGrid[x][y].ledState;
              break;
            case LED_BLINK_FAST:
              if (lastLedState != LED_BLINK_FAST) {
                resetBlinkingBits(x, y);
              }
              mGrid[x][y].lastLedState = mGrid[x][y].ledState;
              handleLedBlinkLinear(x, y);
              break;
            case LED_BLINK_SLOW:
              if (lastLedState != LED_BLINK_SLOW) {
                resetBlinkingBits(x, y);
              }
              mGrid[x][y].lastLedState = mGrid[x][y].ledState;
              handleLedBlinkLinear(x, y);
              break;
          }
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

void MonomeGrid::setAllLeds(LedState state) {
  MonomeCommand cmd;
  cmd.type = MonomeCommand::ALL_LEDS;
  cmd.args.allLedsState = state;
  TPCircularBufferProduceBytes(&mCommandsBuffer, &cmd, sizeof(MonomeCommand));
}

void MonomeGrid::setOneLed(int x, int y, LedState state) {
  MonomeCommand cmd;
  cmd.type = MonomeCommand::SET_LED;
  cmd.args.setLedArgs.x = x;
  cmd.args.setLedArgs.y = y;
  cmd.args.setLedArgs.state = state;
  TPCircularBufferProduceBytes(&mCommandsBuffer, &cmd, sizeof(MonomeCommand));
}

void MonomeGrid::setRow(int y, LedState state) {
  MonomeCommand cmd;
  cmd.type = MonomeCommand::SET_ROW;
  cmd.args.setLedArgs.y = y;
  cmd.args.setLedArgs.state = state;
  TPCircularBufferProduceBytes(&mCommandsBuffer, &cmd, sizeof(MonomeCommand));
}

void MonomeGrid::setColumn(int x, LedState state) {
  MonomeCommand cmd;
  cmd.type = MonomeCommand::SET_COLUMN;
  cmd.args.setLedArgs.x = x;
  cmd.args.setLedArgs.state = state;
  TPCircularBufferProduceBytes(&mCommandsBuffer, &cmd, sizeof(MonomeCommand));
}

void MonomeGrid::buttonTouched(int x, int y, bool isDown) {
  if (isDown) {
    mGrid[x][y].buttonDownTime = std::chrono::system_clock::now();
  }
  mGrid[x][y].buttonState = isDown ? TOUCH_DOWN : TOUCH_UP;
  mButtonsCb(x,y,mGrid[x][y].buttonState);
}
