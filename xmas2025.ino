// Xmas 2025 Ornament
// This code released into the public domain by Aaron St. John

#include <SoftWire.h>

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define adc_disable() (ADCSRA &= ~(1<<ADEN)) // disable ADC (before power-off)
#define adc_enable()  (ADCSRA |=  (1<<ADEN)) // re-enable ADC

#define SDA_PORT PB0
#define SCL_PORT PB1
#define SDB_PORT PB2
#define NEXT_BUTTON_PIN PB2 // Yes, same as SDB.  That's why the lights shut off when you press the button

const uint8_t I2C_ADDR = 0b0110100;

uint8_t swTxBuffer[16];
uint8_t swRxBuffer[16];

#define NUM_ROWS 8
#define NUM_COLS 4

#define NUM_ANIMATIONS 7

// I really should define all of these as constants, but I only got to a few
#define FALLING_STAR_SPEED 0x28
#define BACK_AND_FORTH_FADE_SPEED 0x30

uint8_t matrix[NUM_ROWS][NUM_COLS];

SoftWire i2c(SDA_PORT, SCL_PORT);

byte saveADCSRA; // variable to save the content of the ADC for later. if needed.

void setup() {
  // Set up the watchdog timer
  adc_disable();
  resetWatchDog();

  // Button pin to input
  pinMode(NEXT_BUTTON_PIN, INPUT);

  // Reset the LED driver
  pinMode(SDB_PORT, OUTPUT);
  digitalWrite(SDB_PORT, LOW);

  // Set up the I2C library
  i2c.setTxBuffer(swTxBuffer, sizeof(swTxBuffer));
  i2c.setRxBuffer(swRxBuffer, sizeof(swRxBuffer));
  i2c.setDelay_us(0);
  i2c.setTimeout(1000);
  i2c.begin();
  
  // Row scale register
  for(int indexRegister = 0x90; indexRegister <= 0x9F; indexRegister++) {
      writeLedMatrixRegister(indexRegister, 0x80);
  }

  writeLedMatrixRegister(0xA0, 0b00010001); // Configuration register  
  writeLedMatrixRegister(0xA1, 0x01); // Global current register

  // Turn the LED driver on
  digitalWrite(SDB_PORT, HIGH);

  setupAnimation();  
}

// Writes a register to the LED matrix
void writeLedMatrixRegister(uint8_t reg, uint8_t data) {
  i2c.beginTransmission(I2C_ADDR);
  i2c.write(reg);
  i2c.write(data);
  i2c.endTransmission();
}

int frame = 0;
int animation = 0;
bool lastNextButton = false;

// the loop function runs over and over again forever
void loop() {
  // Count frames to time some of the animations
  frame++; 

  // The button is held high, and pressing the button ties it to ground
  bool thisNextButton = !digitalRead(NEXT_BUTTON_PIN);

  // On release, switch animations
  // (should probably do it on press so the switchover is less noticible, but here we are...)
  if(lastNextButton & ! thisNextButton) {
    animation = (animation + 1) % NUM_ANIMATIONS;
    setupAnimation();
  }

  runAnimation();

  // Write the LED matrix to the LED controller
  for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
      for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
        writeLedMatrixRegister((NUM_COLS - indexCol - 1) * 0x10 + indexRow + 1, matrix[indexRow][indexCol]);
      }
    }

  // update the button state
  lastNextButton = thisNextButton;

  sleepNow ();
}

void setupAnimation() {
  // Set up the animation
  switch(animation) {
    // falling stars
    case 0:
      for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
        for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
          matrix[indexRow][indexCol] = 0;
        }
      }
      break;

    // up and down
    case 1:
      for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
        for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
          matrix[indexRow][indexCol] = random(0xff);
        }
      }
      break;

    // random stars
    case 2:
      for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
        for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
          matrix[indexRow][indexCol] = 0x00;
        }
      }
      break;

    // chasers
    case 3:
      for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
        for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
          matrix[indexRow][indexCol] = (indexRow + indexCol) % 4 * (0xFF/4);
        }
      }
      break;

    // chasers 2
    case 4:
      for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
        for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
          matrix[indexRow][indexCol] = (indexRow + indexCol) % 4 * (0xFF/4);
        }
      }
      break;

    // back and forth
    case 5:
      for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
        for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
          matrix[indexRow][indexCol] = 0;
        }
      }
      break;

       // on
    case 6:
      for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
        for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
          matrix[indexRow][indexCol] = 0xFF;
        }
      }
      break;
  }
}

void runAnimation() {
  // Do the animation
  switch(animation) {
    // falling stars
    case 0:
      for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {        
        if(random() % 10 == 0) {
          matrix[0][indexCol] = 0xFF;
        } else {
          if(matrix[0][indexCol] > FALLING_STAR_SPEED) {
            matrix[0][indexCol] = matrix[0][indexCol] - FALLING_STAR_SPEED;
          }
        }
      }

      for(int indexRow = NUM_ROWS - 1; indexRow >= 1; indexRow--) {
        for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
          matrix[indexRow][indexCol] = matrix[indexRow - 1][indexCol];
        }
      }        
      
      break;

    case 1:
      for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
        for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
          if(matrix[indexRow][indexCol] % 2) {
            if(matrix[indexRow][indexCol] > 0x04) {
              matrix[indexRow][indexCol] -= 0x04;
            } else {
              matrix[indexRow][indexCol] = 0x00;
            }
          } else {
            if(matrix[indexRow][indexCol] < 0xFF - 0x04) {
              matrix[indexRow][indexCol] += 0x04;
            } else {
              matrix[indexRow][indexCol] = 0xFF;
            }
          }  
        }
      }
      break;

    // random stars
    case 2:
      for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
        for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
          if(random(25) == 0) {
            matrix[indexRow][indexCol] = 0xFF;
          } else if (matrix[indexRow][indexCol] > 0x04)
            matrix[indexRow][indexCol] -= 0x10;
        }
      }
      break;

    // chasers
    case 3:
      for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
        for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
          matrix[indexRow][indexCol] -= 0x28;
        }
      }
      break;

    // chaser 2
    case 4:
      for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
        for(int indexCol = 0; indexCol < NUM_COLS; indexCol++) {
          if(indexRow % 2) {
            matrix[indexRow][indexCol] -= 0x28;
          } else {
            matrix[indexRow][indexCol] += 0x28;
          }
        }
      }
      break;

    // back and forth
    case 5:
      // even rows
      for(int indexRow = 0; indexRow < NUM_ROWS; indexRow += 2) {        
        if(frame * 11 % 17 == indexRow) {
          matrix[indexRow][0] = 0xFF;
        } else {
          if(matrix[indexRow][0] > BACK_AND_FORTH_FADE_SPEED) {
            matrix[indexRow][0] = matrix[indexRow][0] - BACK_AND_FORTH_FADE_SPEED;
          }
        }
      }

      // odd rows
      for(int indexRow = 1; indexRow < NUM_ROWS; indexRow += 2) {        
        if(frame * 11 % 19 == indexRow) {
          matrix[indexRow][NUM_COLS - 1] = 0xFF;
        } else {
          if(matrix[indexRow][NUM_COLS - 1] > BACK_AND_FORTH_FADE_SPEED) {
            matrix[indexRow][NUM_COLS - 1] = matrix[indexRow][NUM_COLS - 1] - BACK_AND_FORTH_FADE_SPEED;
          }
        }
      }

      if(frame % 1 == 0) {
        for(int indexRow = 0; indexRow < NUM_ROWS; indexRow++) {
          if(indexRow % 2 == 0) {
            // even rows
            for(int indexCol = NUM_COLS - 1; indexCol >= 1; indexCol--) {
              matrix[indexRow][indexCol] = matrix[indexRow][indexCol - 1];
            }
          } else {
            // odd rows
            for(int indexCol = 0; indexCol < NUM_COLS - 1; indexCol++) {
              matrix[indexRow][indexCol] = matrix[indexRow][indexCol + 1];
            }
          }
        }        
      }
      break;
  }  

  // For the last animation, don't do anything.  Just leave the lights on
}

// Shamelessly stolen from the internet
void sleepNow ()
{
  set_sleep_mode ( SLEEP_MODE_PWR_DOWN ); // set sleep mode Power Down
  saveADCSRA = ADCSRA;                    // save the state of the ADC. We can either restore it or leave it turned off.
  ADCSRA = 0;                             // turn off the ADC
  power_all_disable ();                   // turn power off to ADC, TIMER 1 and 2, Serial Interface
  
  noInterrupts ();                        // turn off interrupts as a precaution
  resetWatchDog ();                       // reset the WatchDog before beddy bies
  sleep_enable ();                        // allows the system to be commanded to sleep
  interrupts ();                          // turn on interrupts
  
  sleep_cpu ();                           // send the system to sleep, night night!

  sleep_disable ();                       // after ISR fires, return to here and disable sleep
  power_all_enable ();                    // turn on power to ADC, TIMER1 and 2, Serial Interface
  
  // ADCSRA = saveADCSRA;                 // turn on and restore the ADC if needed. Commented out, not needed.
  
} // end of sleepNow ()

void resetWatchDog ()
{
  MCUSR = 0;
  WDTCR = bit ( WDCE ) | bit ( WDE ) | bit ( WDIF ); // allow changes, disable reset, clear existing interrupt
  WDTCR = bit ( WDIE ) /*| bit ( WDP2 )| bit ( WDP1 )*/; // set WDIE ( Interrupt only, no Reset ) and 15 millisecond TimeOut
                                                    
  wdt_reset ();                            // reset WDog to parameters
  
} // end of resetWatchDog ()

ISR ( WDT_vect )
{
  wdt_disable ();                           // until next time....
} // end of ISR