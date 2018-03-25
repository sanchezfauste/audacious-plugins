/*
     File: AudioDevice.h 
 Adapted from the CAPlayThough example
  Version: 1.2.2 
  
 Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple 
 Inc. ("Apple") in consideration of your agreement to the following 
 terms, and your use, installation, modification or redistribution of 
 this Apple software constitutes acceptance of these terms.  If you do 
 not agree with these terms, please do not use, install, modify or 
 redistribute this Apple software. 
  
 In consideration of your agreement to abide by the following terms, and 
 subject to these terms, Apple grants you a personal, non-exclusive 
 license, under Apple's copyrights in this original Apple software (the 
 "Apple Software"), to use, reproduce, modify and redistribute the Apple 
 Software, with or without modifications, in source and/or binary forms; 
 provided that if you redistribute the Apple Software in its entirety and 
 without modifications, you must retain this notice and the following 
 text and disclaimers in all such redistributions of the Apple Software. 
 Neither the name, trademarks, service marks or logos of Apple Inc. may 
 be used to endorse or promote products derived from the Apple Software 
 without specific prior written permission from Apple.  Except as 
 expressly stated in this notice, no other rights or licenses, express or 
 implied, are granted by Apple herein, including but not limited to any 
 patent rights that may be infringed by your derivative works or by other 
 works in which the Apple Software may be incorporated. 
  
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE 
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION 
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS 
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND 
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS. 
  
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL 
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, 
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED 
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), 
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE 
 POSSIBILITY OF SUCH DAMAGE. 
  
 Copyright (C) 2013 Apple Inc. All Rights Reserved.
 Copyright (C) 2017 René J.V. Bertin All Rights Reserved.
  
*/

#ifndef __AudioDevice_h__
#define __AudioDevice_h__

#include <CoreServices/CoreServices.h>
#include <CoreAudio/CoreAudio.h>
#include <AvailabilityMacros.h>

#ifndef DEPRECATED_LISTENER_API
#   if !(defined(MAC_OS_X_VERSION_10_11) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_11)
#       define DEPRECATED_LISTENER_API 1
#       warning "Using the deprecated PropertyListener API; at least it works"
#   endif
#endif

#if DEPRECATED_LISTENER_API
using AudioPropertyListenerProc = AudioDevicePropertyListenerProc;
#else
using AudioPropertyListenerProc = AudioObjectPropertyListenerProc;
#endif

class AudioDeviceList;

class AudioDevice {
public:

    typedef void (DefaultDeviceChangeHandler)(AudioObjectID newID, void *data);

    AudioDevice();
    AudioDevice(AudioObjectID devid, bool isInput=false);
    AudioDevice(AudioObjectID devid, bool quick, bool isInput);
    AudioDevice(AudioObjectID devid, AudioPropertyListenerProc lProc, bool isInput=false);
    ~AudioDevice();

    void Init();
    void Init(AudioPropertyListenerProc lProc);

    bool Valid() { return mID != kAudioDeviceUnknown; }

    void SetBufferSize(UInt32 size);
    OSStatus NominalSampleRate(Float64 &sampleRate);
    inline Float64 ClosestNominalSampleRate(Float64 sampleRate);
    OSStatus SetNominalSampleRate(Float64 sampleRate, bool force=false);
    OSStatus ResetNominalSampleRate(bool force=false);
    OSStatus SetStreamBasicDescription(AudioStreamBasicDescription *desc);
    int CountChannels();
    char *GetName(char *buf=NULL, UInt32 maxlen=0);

    void SetInitialNominalSampleRate(Float64 sampleRate)
    {
        mInitialFormat.mSampleRate = sampleRate;
    }

    Float64 CurrentNominalSampleRate()
    {
        return currentNominalSR;
    }

    AudioObjectID ID()
    {
        return mID;
    }

    bool isDefaultDevice()
    {
        return mDefaultDevice;
    }
    void setDefaultDevice(bool isDefault)
    {
        mDefaultDevice = isDefault;
    }

    void installDefaultDeviceChangeHandler(DefaultDeviceChangeHandler *handler, void *data);
    void callDefaultDeviceChangeHandler(AudioObjectID newID);

    static AudioDevice *GetDefaultDevice(bool forInput, OSStatus &err, AudioDevice *dev=NULL);
    static AudioDevice *GetDevice(AudioObjectID devId, bool forInput, AudioDevice *dev=NULL, bool quick=false);

protected:
    AudioStreamBasicDescription mInitialFormat;
    AudioPropertyListenerProc listenerProc;
    OSStatus GetPropertyDataSize( AudioObjectPropertySelector property, UInt32 *size, AudioObjectPropertyAddress *propertyAddress=NULL );
    Float64 currentNominalSR;
    Float64 minNominalSR, maxNominalSR;
    UInt32 nominalSampleRates;
    Float64 *nominalSampleRateList = NULL;
    bool discreteSampleRateList;
    const AudioObjectID mID;
    const bool mForInput;
    UInt32 mSafetyOffset;
    UInt32 mBufferSizeFrames;
    AudioStreamBasicDescription mFormat;
    char mDevName[256] = "";

    bool mInitialised = false;
    bool mDefaultDevice = false;

    DefaultDeviceChangeHandler *defDeviceChangeHandler = nullptr;
    void *defDeviceChangeHanderData = nullptr;

private:
    bool gettingDevName = false;

friend class AudioDeviceList;

public:
    UInt32 listenerSilentFor;
};


#endif // __AudioDevice_h__
