/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_GamepadDriver,
    0xde813776,0x46f1,0x49f2,0xb1,0xae,0x3d,0x4b,0x19,0xf3,0xe0,0xf8);
// {de813776-46f1-49f2-b1ae-3d4b19f3e0f8}
