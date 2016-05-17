/** @file MonomeGrid.h
 *  @author Alessandro Saccoia <contact@alsc.co>
 *  @date 3/25/16
 *  @license Public Domain
 */

#ifndef __MonomeGrid__
#define __MonomeGrid__

#include <stdio.h>
#include <monome.h>
#include <thread>
#include "TPCircularBuffer.h"

/*!
  @class			MonomeGrid
	
  A C++ wrapper around libmonome: hides some of the complexity of the libmonome's
  C Interface offering a C++11, callback-based solution.
 
  The std::function called TouchCallback is called whenever a button is pressed,
  released, or a long press is detected (default time = 0.5s). In this function
  is possible to change something in the monome itself, for example enabling the
  corresponding LED by calling one of the setXXX methods.
  
  The std::function GridRefreshed is called each time the grid is refreshed, that
  is ~20 ms. It runs on the monome thread. One example of using this callback
  is readin the MIDI clock of a sequencer, and updating the LEDs accordingly.
  Note that there's an internal thread that changes all the LEDs every 20 ms,
  so you are't supposed to do anything that is very time consuming here.
  
  All the SetXXX methods are thread safe, so for example you can call them
  from an audio or MIDI callback without problems: they use the lock-free 
  TPCircularBuffer implementation. Please see the (few) examples.
  
  Note that multiple calld to SetXXX with the same value (ON, OFF, BLINK) don't
  afect the performance. So feel free to clear the grid how many times you want.
 
  When using this class, always consider the monome as made of two very
  different things:
  - a MxN input matrix of push buttons (you decide what to do when they are pushed)
  - a MxN input matrix of LEDs (you decide when to light them up)
 
*/

class MonomeGrid {
 public:
  /// Represent the current state of a pushbutton as reported by the callback
  enum ButtonState {
    TOUCH_DOWN,
    TOUCH_UP,
    TOUCH_LONG // called when the user holds the button for more than 0.5 seconds
  };
  
  
  /// Type of callback called when the user presses a button on the grid
  typedef std::function<void(int, int, ButtonState)> TouchCallback;
  
  /**  Called each time the LED grid state is refreshed (~20 ms).
   *   The refreshCb_ can be used for:
   *    - drive a sequencer
   *    - change the state of the LEDs
   */
  typedef std::function<void(void)> GridRefreshed;
 
  /** Constructor
   *  @param monomeName The name used to open the monome connection
   *  @param width The width of the monome (i.e. 8 for the 40h)
   *  @param height The height of the monome (i.e. 8 for the 40h)
   *  @param touchCb_ Called when the user presses a button
   *  @param refreshCb_ The function called every LED refresh cycle
   *
   *  @note refreshCb_ is called on an internal thread, precautions must be used
   *
   */
  MonomeGrid(const char* monomeName
   , unsigned int width
   , unsigned int height
   , TouchCallback touchCb_ // called when the user presses a button, 0 = down, 1 = up, 2 = long
   , std::function<void(void)> refreshCb_);
  
  /// Destructor
  ~MonomeGrid();
  
  /**
   * This method should be called at the end of "main" for console applications.
   * It will start the Monome thread
   */
  void loop();
  
  enum LedState {
    LED_OFF = 0x00,
    LED_ON = 0x01,
    LED_BLINK_FAST = 0x02,
    LED_BLINK_SLOW = 0x03
  };
  
  /// Sets all leds to the given state
  void setAllLeds(LedState state);
  
  /// Sets one led to the given state
  void setOneLed(int x, int y, LedState state);
  
  /// Sets one row of leds to the given state
  void setRow(int y, LedState state);
  
  /// Sets one column to the given state
  void setColumn(int x, LedState state);
  
private:
 
  friend void handle_press(const monome_event_t *e, void *data);

  /// called by handle_press
  void buttonTouched(int x, int y, bool isDown);
  
  // used for blinking the LEDs: I have a 40D and I am not sure that it
  struct {
    int bits;
    int log2Bits;
  } blinkingSpeeds [2];
  
  void updateGrid();                       // constantly called by mMonomeThread
  void resetBlinkingBits(int x, int y);    // don't call directly
  void handleLedBlinkLinear(int x, int y); // don't call directly
  
  monome_t *mMonome;
  unsigned int mWidth;
  unsigned int mHeight;
  
  TPCircularBuffer mCommandsBuffer; // buffer of commands, lock-free
  std::thread mMonomeThread; // this thread holds updateGrid()
  
  std::function<void(int, int, ButtonState)> mButtonsCb;
  std::function<void(void)> mRefreshCb;
  
  typedef std::chrono::system_clock::time_point monome_time_t;
  
  typedef struct MonomeCell {
    MonomeCell() : ledState(LED_OFF), lastLedState(LED_OFF), buttonState(TOUCH_UP) {}
    int ledState;
    int lastLedState;
    ButtonState buttonState;
    monome_time_t buttonDownTime;
  } MonomeCell;
  
  MonomeCell** mGrid;
};

#endif /* defined(__MonomeGrid__) */
