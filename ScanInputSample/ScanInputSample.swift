//
//  ScanInputSample.swift
//  ScratchKeyboardIO
//
//  Created by 조수환 on 2023/07/04.
//  Copyright © 2023 Apple. All rights reserved.
//

import Foundation
import IOKit
import IOKit.hid

@main
struct EntryPoint {
    static func main() throws {
        let hidManager = IOHIDManagerCreate(nil, 0)

        let matches = [
            [
                kIOHIDDeviceUsagePageKey: 1,
                kIOHIDDeviceUsageKey: 6
            ], // keyboard
            [
                kIOHIDDeviceUsagePageKey: 1,
                kIOHIDDeviceUsageKey: 7
            ] // keypad
        ] as CFArray

        IOHIDManagerSetDeviceMatchingMultiple(hidManager, matches)
        IOHIDManagerRegisterInputValueCallback(hidManager, ioCallback(_:_:_:_:), nil)

        IOHIDManagerScheduleWithRunLoop(hidManager,
                                        RunLoop.main.getCFRunLoop(),
                                        RunLoop.Mode.default.rawValue as CFString)

        IOHIDManagerOpen(hidManager, 0)

        RunLoop.main.run()
    }
}

func ioCallback(_ context: UnsafeMutableRawPointer?,
                _ result: IOReturn,
                _ sender: UnsafeMutableRawPointer?,
                _ value: IOHIDValue) {
    let element = IOHIDValueGetElement(value)

    guard IOHIDElementGetUsagePage(element) == 0x07 else { return }

    let scancode = IOHIDElementGetUsage(element)

    guard scancode >= 4, scancode <= 231 else { return }

    let pressed = IOHIDValueGetIntegerValue(value)

    print("scancode \(scancode), pressed: \(pressed)")
}
