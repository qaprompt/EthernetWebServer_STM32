/****************************************************************************************************************************
  cencoder.c - c source to a base64 decoding algorithm implementation
  For STM32 with built-in Ethernet LAN8742A (Nucleo-144, DISCOVERY, etc) or W5x00/ENC28J60 shield/module

  EthernetWebServer_STM32 is a library for the STM32 running Ethernet WebServer

  Based on and modified from ESP8266 https://github.com/esp8266/Arduino/releases
  Built by Khoi Hoang https://github.com/khoih-prog/EthernetWebServer_STM32
  Licensed under MIT license

  Original author:
  @file       Esp8266WebServer.h
  @author     Ivan Grokhotkov

  Version: 1.3.3

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0   K Hoang      26/02/2020 Initial coding for STM32 with built-in Ethernet (Nucleo-144, DISCOVERY, etc) and ENC28J60
  1.0.1   K Hoang      28/02/2020 Add W5x00 Ethernet shields using Ethernet library
  1.0.2   K Hoang      05/03/2020 Remove dependency on functional-vlpp
  1.0.3   K Hoang      22/07/2020 Fix bug not closing client and releasing socket. Add features.
  1.0.4   K Hoang      23/07/2020 Add support to all STM32 boards (STM32F/L/H/G/WB/MP1) with 32K+ Flash.
  1.0.5   K Hoang      16/09/2020 Add support to Ethernet2, Ethernet3, Ethernet Large for W5x00
                                  Add support to new EthernetENC library for ENC28J60. Add debug feature.
  1.0.6   K Hoang      24/09/2020 Add support to PROGMEM-related commands, such as sendContent_P() and send_P()
  1.1.0   K Hoang      17/11/2020 Add basic HTTP and WebSockets Client by merging ArduinoHttpClient
  1.1.1   K Hoang      26/12/2020 Suppress all possible compiler warnings. Add Version String
  1.2.0   K Hoang      11/04/2021 Add support to LAN8720 using STM32F4 or STM32F7
  1.2.1   K Hoang      04/10/2021 Change option for PIO `lib_compat_mode` from default `soft` to `strict`. Update Packages Patches
  1.3.0   K Hoang      20/12/2021 Reduce usage of Arduino String with std::string. Use reference passing instead of value-passing
  1.3.1   K Hoang      25/12/2021 Fix bug
  1.3.2   K Hoang      28/12/2021 Fix wrong http status header bug and authenticate issue caused by libb64
  1.3.3   K Hoang      11/01/2022 Fix libb64 fallthrough compile warning
 *****************************************************************************************************************************/
 
#include "cencode.h"

const int CHARS_PER_LINE = 72;

void base64_init_encodestate(base64_encodestate* state_in)
{
  state_in->step = step_A;
  state_in->result = 0;
  state_in->stepcount = 0;
}

char base64_encode_value(char value_in)
{
  static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  if (value_in > 63)
    return '=';

  return encoding[(unsigned int)value_in];
}

int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in)
{
  const char* plainchar = plaintext_in;
  const char* const plaintextend = plaintext_in + length_in;
  char* codechar = code_out;
  char result;
  char fragment;

  result = state_in->result;

  switch (state_in->step)
  {
      while (1)
      {
      case step_A:
        if (plainchar == plaintextend)
        {
          state_in->result = result;
          state_in->step = step_A;
          return codechar - code_out;
        }

        fragment = *plainchar++;
        result = (fragment & 0x0fc) >> 2;
        *codechar++ = base64_encode_value(result);
        result = (fragment & 0x003) << 4;
        
        // fall through

      case step_B:
        if (plainchar == plaintextend)
        {
          state_in->result = result;
          state_in->step = step_B;
          return codechar - code_out;
        }

        fragment = *plainchar++;
        result |= (fragment & 0x0f0) >> 4;
        *codechar++ = base64_encode_value(result);
        result = (fragment & 0x00f) << 2;
        
        // fall through
        
      case step_C:
        if (plainchar == plaintextend)
        {
          state_in->result = result;
          state_in->step = step_C;
          return codechar - code_out;
        }

        fragment = *plainchar++;
        result |= (fragment & 0x0c0) >> 6;
        *codechar++ = base64_encode_value(result);
        result  = (fragment & 0x03f) >> 0;
        *codechar++ = base64_encode_value(result);

        ++(state_in->stepcount);

        if (state_in->stepcount == CHARS_PER_LINE / 4)
        {
          *codechar++ = '\n';
          state_in->stepcount = 0;
        }
        
        // fall through
      }
  }

  /* control should not reach here */
  return codechar - code_out;
}

int base64_encode_blockend(char* code_out, base64_encodestate* state_in)
{
  char* codechar = code_out;

  switch (state_in->step)
  {
    case step_B:
      *codechar++ = base64_encode_value(state_in->result);
      *codechar++ = '=';
      *codechar++ = '=';
      break;
    case step_C:
      *codechar++ = base64_encode_value(state_in->result);
      *codechar++ = '=';
      break;
    case step_A:
      break;
  }

  *codechar = 0x00;

  return codechar - code_out;
}

int base64_encode_chars(const char* plaintext_in, int length_in, char* code_out)
{
  base64_encodestate _state;
  base64_init_encodestate(&_state);
  int len = base64_encode_block(plaintext_in, length_in, code_out, &_state);

  return len + base64_encode_blockend((code_out + len), &_state);
}
