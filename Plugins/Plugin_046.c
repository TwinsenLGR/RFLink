//#######################################################################################################
//##                    This Plugin is only for use with the RFLink software package                   ##
//##                                       Plugin-46 Auriol                                            ##
//#######################################################################################################
/*********************************************************************************************\
 * This plugin takes care of decoding the Auriol protocol for sensor type Z31055A-TX
 * 
 * Author             : StuntTeam
 * Support            : www.nodo-domotica.nl
 * Date               : 9-02-2015
 * Version            : 1.0
 * Compatibility      : RFLink 1.0
 * License            : This code is free for use in any open source project when this header is included.
 *                      Usage of any parts of this code in a commercial application is prohibited!
 *********************************************************************************************
 * Changelog: v1.0 initial release
 *********************************************************************************************
 * Technical Information:
 * Decodes signals from a Auriol Weatherstation outdoor unit, (36 bits, 433 MHz).
 * Auriol Message Format: 
 * 1010 0100 1000 0000 1110 0110 1111 0000 0000  Temp = 23.60
 * AAAA AAAA BCCC DDDD DDDD DDDD EEEE FFFF FFFF
 *
 * A = Rolling Code, no change during normal operation. (Device 'Session' ID) (Might also be 4 bits RC and 4 bits for channel number)
 * B = Battery status indicator, 1=OK 0=LOW
 * C = Always 000
 * D = Temperature (21.5 degrees is shown as decimal value 215, minus values have the high bit set and need to be subtracted from a base value of 4096)
 * E = Always 1111 ?
 * F = Always 0000 0000 ?
 *
 * Sample:
 * 20;1F;DEBUG;Pulses=74;Pulses(uSec)=550,1575,525,675,525,1625,500,700,475,725,500,1675,500,700,500,725,475,1675,475,750,450,750,475,725,450,750,450,750,475,750,450,750,475,1675,450,1700,425,1700,450,750,450,750,450,1700,450,1700,450,775,450,1700,450,1700,450,1700,425,1700,425,775,450,775,450,775,425,775,425,775,425,775,450,775,425,775,425,
 \*********************************************************************************************/
#define PLUGIN_ID 46
#define PLUGIN_NAME "AuriolV2"

#define AURIOL_PULSECOUNT 74

boolean Plugin_046(byte function, struct NodoEventStruct *event, char *string)
{
  boolean success=false;

  switch(function)
  {
#ifdef PLUGIN_046_CORE
  case PLUGIN_RAWSIGNAL_IN:
    {
      if (RawSignal.Number != AURIOL_PULSECOUNT) return false;

      unsigned long bitstream1=0;
      unsigned long bitstream2=0;
      byte rc=0;
      byte bat=0;
      int temperature=0;
	  int bitcounter=0;
      char buffer[14]=""; 
      //==================================================================================
      // get all the bits we need (36 bits)
      for(int x=2;x<RawSignal.Number;x+=2) {
         if (RawSignal.Pulses[x]*RawSignal.Multiply > 1400) {
            if (bitcounter < 24) {
               bitstream1 = (bitstream1 << 1) | 0x1;
               bitcounter++;                     // only need to count the first 10 bits
            } else {
               bitstream2 = (bitstream2 << 1) | 0x1;
            }
         } else {
            if (RawSignal.Pulses[x]*RawSignal.Multiply > 900) return false;
            if (RawSignal.Pulses[x]*RawSignal.Multiply < 500) return false;
            if (bitcounter < 24) {
               bitstream1 = (bitstream1 << 1); 
               bitcounter++;                     // only need to count the first 10 bits
            } else {
               bitstream2 = (bitstream2 << 1); 
            }
         }
      }
      //==================================================================================
      // First perform sanity checks
      if (bitstream1 == 0) return false;
      if ((bitstream2 & 0xfff) != 0xF00) { 
         Serial.println("CRC Error");
         return false;
      }
      rc = (bitstream1 >> 12) & 0x07;            // get 3 bits, should always be 000
      if (rc != 0) return false; 
      //==================================================================================
      bat= (bitstream1 >> 15) & 0x01;            // get battery strength indicator
      rc = (bitstream1 >> 16) & 0xff;            // get rolling code
      temperature = (bitstream1) & 0xfff;        // get 12 temperature bits
      if (temperature > 3000) {
         temperature=4096-temperature;           // fix for minus temperatures
         temperature=temperature | 0x8000;       // turn highest bit on for minus values
      }      
      //==================================================================================
      // Output
      // ----------------------------------
      sprintf(buffer, "20;%02X;", PKSequenceNumber++); // Node and packet number 
      Serial.print( buffer );
      // ----------------------------------
      Serial.print("Auriol V2;");                // Label
      sprintf(buffer, "ID=00%02x;", rc);         // ID    
      Serial.print( buffer );
      sprintf(buffer, "TEMP=%04x;", temperature);     
      Serial.print( buffer );
      if (bat==0) {                              // battery status
         Serial.print("BAT=LOW;");
      } else {
         Serial.print("BAT=OK;");
      }
      Serial.println();
      //==================================================================================
      RawSignal.Number=0;
      success = true;
      break;
    }
#endif // PLUGIN_046_CORE
  }      
  return success;
}