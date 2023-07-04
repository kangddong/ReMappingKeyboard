 /*
See LICENSE folder for this sample’s licensing information.

Abstract:
Command line tool that demonstrates how to use IOKitLib to find all serial ports on macOS.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/serial/ioss.h>
#include <IOKit/IOBSD.h>

#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hid/IOHIDManager.h>

// Default to local echo being on. If your modem has local echo disabled, undefine the following macro.
#define LOCAL_ECHO

// Find the first device that matches the callout device path MATCH_PATH.
// If this is undefined, return the first device found.
// #define MATCH_PATH "/dev/cu.usbmodemfa131"

#define kATCommandString	"AT\r"

#ifdef LOCAL_ECHO
#define kOKResponseString	"AT\r\r\nOK\r\n"
#else
#define kOKResponseString	"\r\nOK\r\n"
#endif

const int kNumRetries = 3;

// Hold the original termios attributes so we can reset them
static struct termios gOriginalTTYAttrs;

// Function prototypes
//static int openSerialPort(const char *bsdPath);

static kern_return_t findMasterPort(mach_port_t *masterPort)
{
    kern_return_t            kernResult;
    
    // Get an iterator across all matching devices.
    kernResult = IOMasterPort(MACH_PORT_NULL, masterPort);
    if (KERN_SUCCESS != kernResult) {
        printf("IOServiceGetMatchingServices returned %d\n", kernResult);
        goto exit;
    }
    
exit:
    return kernResult;
}
void myHIDKeyboardCallback( void* context,  IOReturn result,  void* sender,  IOHIDValueRef value )
{
    IOHIDElementRef elem = IOHIDValueGetElement( value );

    if (IOHIDElementGetUsagePage(elem) != 0x07)
        return;

    uint32_t scancode = IOHIDElementGetUsage( elem );

    if (scancode < 4 || scancode > 231)
        return;

    long pressed = IOHIDValueGetIntegerValue( value );

    printf( "scancode: %d, pressed: %ld\n", scancode, pressed );
}


CFMutableDictionaryRef myCreateDeviceMatchingDictionary( UInt32 usagePage,  UInt32 usage )
{
    CFMutableDictionaryRef dict = CFDictionaryCreateMutable(
                                                            kCFAllocatorDefault, 0
                                                        , & kCFTypeDictionaryKeyCallBacks
                                                        , & kCFTypeDictionaryValueCallBacks );
    if ( ! dict )
        return NULL;

    CFNumberRef pageNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, & usagePage );
    if ( ! pageNumberRef ) {
        CFRelease( dict );
        return NULL;
    }

    CFDictionarySetValue( dict, CFSTR(kIOHIDDeviceUsagePageKey), pageNumberRef );
    CFRelease( pageNumberRef );

    CFNumberRef usageNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, & usage );

    if ( ! usageNumberRef ) {
        CFRelease( dict );
        return NULL;
    }

    CFDictionarySetValue( dict, CFSTR(kIOHIDDeviceUsageKey), usageNumberRef );
    CFRelease( usageNumberRef );

    return dict;
}

int main(int argc, const char * argv[])
{
    mach_port_t     myMasterPort;
    int             fileDescriptor;
    kern_return_t	kernResult;
    io_iterator_t	serialPortIterator;
    char            bsdPath[MAXPATHLEN];
    
    kernResult = findMasterPort(&myMasterPort);
    if (KERN_SUCCESS != kernResult) {
        printf("No masterport were found.\n");
    }
    
    IOHIDManagerRef hidManager = IOHIDManagerCreate( kCFAllocatorDefault, kIOHIDOptionsTypeNone );

        CFArrayRef matches;
        {
            CFMutableDictionaryRef keyboard = myCreateDeviceMatchingDictionary( 0x01, 6 );
            CFMutableDictionaryRef keypad   = myCreateDeviceMatchingDictionary( 0x01, 7 );

            CFMutableDictionaryRef matchesList[] = { keyboard, keypad };

            matches = CFArrayCreate( kCFAllocatorDefault, (const void **)matchesList, 2, NULL );
        }

        IOHIDManagerSetDeviceMatchingMultiple( hidManager, matches );

        IOHIDManagerRegisterInputValueCallback( hidManager, myHIDKeyboardCallback, NULL );

        IOHIDManagerScheduleWithRunLoop( hidManager, CFRunLoopGetMain(), kCFRunLoopDefaultMode );

        IOHIDManagerOpen( hidManager, kIOHIDOptionsTypeNone );

        CFRunLoopRun(); // spins
    
    return EX_OK;
}
