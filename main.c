#include "helpers.c"
#include <CoreAudio/CoreAudio.h>
#include <stdio.h>
#include <string.h>

const char *TARGET_DEVICE;
const char *FALLBACK_DEVICE;

void print_all_devices() {
  AudioObjectPropertyAddress propertyAddress = {
      kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain};
  UInt32 dataSize = 0;
  OSStatus status = AudioObjectGetPropertyDataSize(
      kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize);

  if (status != kAudioHardwareNoError) {
    fprintf(stderr, "Error getting audio devices size\n");
    return;
  }

  UInt32 deviceCount = dataSize / sizeof(AudioDeviceID);
  AudioDeviceID *audioDevices = (AudioDeviceID *)malloc(dataSize);
  if (audioDevices == NULL) {
    panic("Memory allocation failure\n");
    return;
  }

  status =
      AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0,
                                 NULL, &dataSize, audioDevices);
  if (status != kAudioHardwareNoError) {
    fprintf(stderr, "Error getting audio devices\n");
    free(audioDevices);
    return;
  }

  for (UInt32 i = 0; i < deviceCount; ++i) {
    char deviceName[256];
    dataSize = sizeof(deviceName);
    propertyAddress.mSelector = kAudioDevicePropertyDeviceName;

    status = AudioObjectGetPropertyData(audioDevices[i], &propertyAddress, 0,
                                        NULL, &dataSize, deviceName);
    if (status == kAudioHardwareNoError) {
      printf("%s\n", deviceName);
    }
  }

  free(audioDevices);
}

void device_name(AudioDeviceID device, char *name, unsigned long name_size) {
  UInt32 nameSize = name_size;
  AudioObjectPropertyAddress propertyAddress = {
      kAudioDevicePropertyDeviceName, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain};
  AudioObjectGetPropertyData(device, &propertyAddress, 0, NULL, &nameSize,
                             name);
}

AudioDeviceID current_device(AudioObjectPropertySelector selector) {
  AudioDeviceID device;
  UInt32 propertySize = sizeof(device);
  AudioObjectPropertyAddress propertyAddress = {
      selector, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain};
  AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0,
                             NULL, &propertySize, &device);
  return device;
}

AudioDeviceID find_device_by_name(char *required_name) {
  UInt32 size;
  AudioDeviceID devices[64];
  AudioObjectPropertyAddress propertyAddress = {
      kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain};
  OSStatus status = AudioObjectGetPropertyDataSize(
      kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size);
  if (status != kAudioHardwareNoError) {
    warn("Error getting audio devices size\n");
    return 0;
  }

  OSStatus status2 = AudioObjectGetPropertyData(
      kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size, devices);
  if (status2 != kAudioHardwareNoError) {
    warn("Error getting audio devices\n");
    return 0;
  }

  for (int i = 0; i < size / sizeof(AudioDeviceID); i++) {
    char name[256];
    device_name(devices[i], name, sizeof(name));
    if (strcmp(name, required_name) == 0) {
      return devices[i];
    }
  }
  return 0;
}

void restore_balance(AudioDeviceID device) {
  Float32 volume;
  UInt32 size = sizeof(volume);
  AudioObjectPropertyAddress propertyAddress = {
      kAudioDevicePropertyVolumeScalar, kAudioDevicePropertyScopeOutput,
      1 // Channel 1 for left, change to 2 for right
  };

  // Assuming same size for left and right channels
  AudioObjectGetPropertyData(device, &propertyAddress, 0, NULL, &size, &volume);
  propertyAddress.mElement = 2; // Change to channel 2 for right
  AudioObjectGetPropertyData(device, &propertyAddress, 0, NULL, &size, &volume);

  // Set volume for both channels
  propertyAddress.mElement = 1; // Left channel
  AudioObjectSetPropertyData(device, &propertyAddress, 0, NULL, size, &volume);
  propertyAddress.mElement = 2; // Right channel
  AudioObjectSetPropertyData(device, &propertyAddress, 0, NULL, size, &volume);

  warn("Restored balance for output\n");
}

OSStatus changed_output(AudioObjectID inObjectID, UInt32 inNumberAddresses,
                        const AudioObjectPropertyAddress inAddresses[],
                        void *inClientData) {
  AudioDeviceID changed_device_id;
  for (UInt32 i = 0; i < inNumberAddresses; i++) {
    // Check if the default output device has changed.
    if (inAddresses[i].mSelector == kAudioHardwarePropertyDefaultOutputDevice) {
      UInt32 dataSize = sizeof(changed_device_id);
      AudioObjectPropertyAddress propertyAddress = {
          kAudioHardwarePropertyDefaultOutputDevice,
          kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};

      // Get the new default output device ID.
      OSStatus status =
          AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress,
                                     0, NULL, &dataSize, &changed_device_id);
      if (status != kAudioHardwareNoError) {
        warn("Failed to get new default output device ID\n");
        return kAudioHardwareNoError;
      }
    }
  }

  char name[256];
  device_name(changed_device_id, name, sizeof(name));
  warn("Changed output device to %s\n", name);
  if (strcmp(TARGET_DEVICE, name) == 0) {
    warn("Target device changed to one we have to change\n");
    AudioDeviceID fallback_device = find_device_by_name(FALLBACK_DEVICE);
    if (fallback_device != 0) {
      warn("Setting fallback device to %s\n", FALLBACK_DEVICE);
      usleep(600000); // Sleep for 100ms to allow the device to be ready
      AudioObjectPropertyAddress propertyAddress = {
          kAudioHardwarePropertyDefaultInputDevice,
          kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
      OSStatus status = AudioObjectSetPropertyData(
          kAudioObjectSystemObject, &propertyAddress, 0, NULL,
          sizeof(fallback_device), &fallback_device);
      if (status != noErr) {
        warn("Error setting fallback device %s\n", FALLBACK_DEVICE);
      }
    } else {
      warn("Fallback device not found\n");
    }
  }

  return noErr;
}

OSStatus changed_input(AudioObjectID inObjectID, UInt32 inNumberAddresses,
                       const AudioObjectPropertyAddress inAddresses[],
                       void *inClientData) {
  warn("input changed\n");
  // Logic to handle changes. This function now acts as a generic change
  // listener. You can inspect inAddresses to determine which property changed
  // if needed. Example: Switch input or restore balance based on the property
  // changed.

  return noErr;
}

int main(int argc, const char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--devices") == 0) {
      print_all_devices();
      return 0;
    }
  }

  if (argc != 3) {
    printf("Use: %s <target_device> <fallback_device>\n", argv[0]);
    return 1;
  }

  if (getenv("DEBUG") != NULL) {
    is_debug = true;
  }

  TARGET_DEVICE = argv[1];
  FALLBACK_DEVICE = argv[2];

  AudioObjectPropertyAddress inputDeviceAddress = {
      kAudioHardwarePropertyDefaultInputDevice,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain,
  };

  AudioObjectPropertyAddress outputDeviceAddress = {
      kAudioHardwarePropertyDefaultOutputDevice,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain,
  };

  // Register listener for input device changes
  // OSStatus status = AudioObjectAddPropertyListener(
  //     kAudioObjectSystemObject, &inputDeviceAddress, changed_input, NULL);
  // if (status != noErr) {
  //   printf("Error adding in listener\n");
  //   return 1;
  // }

  // Register listener for output device changes
  OSStatus status2 = AudioObjectAddPropertyListener(
      kAudioObjectSystemObject, &outputDeviceAddress, changed_output, NULL);
  if (status2 != noErr) {
    panic("Error adding out listener\n");
  }

  // Keep the application running to listen for changes
  CFRunLoopRun();
  return 0;
}
