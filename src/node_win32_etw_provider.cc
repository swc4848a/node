// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "node_dtrace.h"
#include "node_win32_etw_provider.h"
#include "node_etw_provider.h"
#include "node_win32_etw_provider-inl.h"

namespace node {

using namespace v8;

HMODULE advapi;
REGHANDLE node_provider;
EventRegisterFunc event_register;
EventUnregisterFunc event_unregister;
EventWriteFunc event_write;
int events_enabled;

// callback from V8 module passes symbol and address info for stack walk resolution
bool CodeAddressNotification(int operation,
                             const void* addr1,
                             int len,
                             const char* symbol,
                             int tag,
                             const void* addr2,
                             int line) {
  if (NODE_V8SYMBOL_ENABLED()) {
    switch (operation) {
    case 1:
      NODE_V8SYMBOL_ADD(symbol, addr1, len, tag, (ULONGLONG)addr2, line);
      break;
    case 2:
      NODE_V8SYMBOL_REMOVE(addr1, addr2);
      break;
    case 3:
      NODE_V8SYMBOL_MOVE(addr1, addr2);
      break;
    case 4:
      NODE_V8SYMBOL_RESET();
      break;
    case 5:
      NODE_V8SYMBOL_SOURCEADD(symbol, addr1, (ULONGLONG)addr2, tag);
      break;
    default:
      return false;
    }
  }
  return true;
}


// This callback is called by ETW when consumers of our provider
// are enabled or disabled.
// The callback is dispatched on ETW thread.
void NTAPI etw_events_enable_callback(
  LPCGUID SourceId,
  ULONG IsEnabled,
  UCHAR Level,
  ULONGLONG MatchAnyKeyword,
  ULONGLONG MatchAllKeywords,
  PEVENT_FILTER_DESCRIPTOR FilterData,
  PVOID CallbackContext) {
  if (IsEnabled) {
    events_enabled++;
    if (events_enabled == 1) {
      V8::SetCodeAddressEventCallback(CodeAddressNotification, true);
    }
  } else {
    if (events_enabled == 1) {
      V8::SetCodeAddressEventCallback(NULL, false);
    }
    events_enabled--;
  }
}


void init_etw() {
  events_enabled = 0;

  advapi = LoadLibrary("advapi32.dll");
  if (advapi) {
    event_register = (EventRegisterFunc)
      GetProcAddress(advapi, "EventRegister");
    event_unregister = (EventUnregisterFunc)
      GetProcAddress(advapi, "EventUnregister");
    event_write = (EventWriteFunc)GetProcAddress(advapi, "EventWrite");

    if (event_register) {    
      DWORD status = event_register(&NODE_ETW_PROVIDER,
                                    etw_events_enable_callback,
                                    NULL,
                                    &node_provider);
      assert(status == ERROR_SUCCESS);
    }
  }
}


void shutdown_etw() {
  if (advapi && event_unregister && node_provider) {
    event_unregister(node_provider);
    node_provider = 0;
  }

  events_enabled = 0;

  if (advapi) {
    FreeLibrary(advapi);
    advapi = NULL;
  }
}

}