
#ifdef Encoder_PWM2

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
ICACHE_RAM_ATTR
#endif

void rotaryInterrupt_PWM2() // TFT PWM Brightness Adjust
{

  /*int rotaryState;
  // Read rotary state (Counter clockwise) -2, -1, 0, 1, 2 (Clockwise)
  rotaryState = rotary.read();
  // Count up or down by using rotary speed
  if (rotaryState == 0) {
    // No change
    return;

  } else if (abs(rotaryState) >= 2) {
    brightness_count += rotaryState * 4;

  } else {
    brightness_count += rotaryState;
  }*/
  /*if(digitalRead(encoderOutA)==LOW){
    brightness_count++;
  }
  if(digitalRead(encoderOutB)==LOW){
    brightness_count--;
  }

  // Limit count to a minimum and maximum value
  if (brightness_count > 250) {
    brightness_count   = 250;

  }
  if (brightness_count  < 0) {
    brightness_count    = 0;
  }*/

  int bri_buttonVal = digitalRead(encoderOutA);
  if (bri_buttonVal == LOW)
  {
    delay(debounceEncButton); // Debounce Button
    bri_Button_counter ++;

    /* Reset count if over max mode number, */
    if (bri_Button_counter == 10) // Number of  bright levels available when button pressed
    {
      bri_Button_counter = 0;
    }
  }
  else
    /* Change Mode */
    switch (bri_Button_counter) {
      case 0: 
        brightness_count   = 250;
        break;

      case 1: 
        brightness_count   = 225;
        break;

      case 2: 
        brightness_count   = 200;
        break;

      case 3: 
        brightness_count   = 175;
        break;

      case 4: 
        brightness_count   = 150;
        break;

      case 5: 
        brightness_count   = 125;
        break;
       
      case 6: 
        brightness_count   = 100;
        break;

      case 7: 
        brightness_count   = 75;
        break;

      case 8: 
        brightness_count   = 50;
        break;

      case 9: 
        brightness_count   = 25;
        break;
    }  
    if (brightness_countLast != brightness_count) {
    brightness_countLast = brightness_count;
    analogWrite(TFT_backlight_PIN , brightness_count);

#ifdef Neo_BrightnessAuto // Adjust NeoPixel brightness with screen brightness

    pixels.setBrightness(brightness_count / Neo_DivideBy); 

#endif

  }
}
#endif
