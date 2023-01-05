#include "BluetoothSerial.h"

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED) && defined(CONFIG_BT_SPP_ENABLED)
#define DEBUG_OVER_BLUETOOTH
#endif

boolean confirmRequestPending = false;

void BTConfirmRequestCallback(uint32_t numVal)
{
  confirmRequestPending = true;
  Serial.println(numVal);
}

void BTAuthCompleteCallback(boolean success)
{
  confirmRequestPending = false;
  if (success)
  {
    Serial.println("Pairing success!!");
  }
  else
  {
    Serial.println("Pairing failed, rejected by user!!");
  }
}

// Modified from esp32/Print.cpp.  
static int debug_vprintf(const char *format, va_list ap) {

    char loc_buf[64];
    char * temp = loc_buf;
    int len = vsnprintf(temp, sizeof(loc_buf), format, ap);

    if(len < 0) {
        return 0;
    };
    if(len >= (int)sizeof(loc_buf)){  
        temp = (char*) malloc(len+1);
        if(temp == NULL) {
            return 0;
        }
        len = vsnprintf(temp, len+1, format, ap);
    }
    len = DebugConsole.write((uint8_t*)temp, len);
    if(temp != loc_buf){
        free(temp);
    }
    return len;
}
